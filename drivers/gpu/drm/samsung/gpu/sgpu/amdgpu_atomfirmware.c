/*
 * Copyright 2016 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <drm/sgpu_drm.h>
#include "amdgpu.h"
#include "atomfirmware.h"
#include "amdgpu_atomfirmware.h"
#include "atom.h"
#include "atombios.h"
#include "soc15_hw_ip.h"

bool amdgpu_atomfirmware_gpu_supports_virtualization(struct amdgpu_device *adev)
{
	int index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
						firmwareinfo);
	uint16_t data_offset;

	if (amdgpu_atom_parse_data_header(adev->mode_info.atom_context, index, NULL,
					  NULL, NULL, &data_offset)) {
		struct atom_firmware_info_v3_1 *firmware_info =
			(struct atom_firmware_info_v3_1 *)(adev->mode_info.atom_context->bios +
							   data_offset);

		if (le32_to_cpu(firmware_info->firmware_capability) &
		    ATOM_FIRMWARE_CAP_GPU_VIRTUALIZATION)
			return true;
	}
	return false;
}

void amdgpu_atomfirmware_scratch_regs_init(struct amdgpu_device *adev)
{
	int index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
						firmwareinfo);
	uint16_t data_offset;

	if (amdgpu_atom_parse_data_header(adev->mode_info.atom_context, index, NULL,
					  NULL, NULL, &data_offset)) {
		struct atom_firmware_info_v3_1 *firmware_info =
			(struct atom_firmware_info_v3_1 *)(adev->mode_info.atom_context->bios +
							   data_offset);

		adev->bios_scratch_reg_offset =
			le32_to_cpu(firmware_info->bios_scratch_reg_startaddr);
	}
}

int amdgpu_atomfirmware_allocate_fb_scratch(struct amdgpu_device *adev)
{
	struct atom_context *ctx = adev->mode_info.atom_context;
	int index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
						vram_usagebyfirmware);
	struct vram_usagebyfirmware_v2_1 *	firmware_usage;
	uint32_t start_addr, size;
	uint16_t data_offset;
	int usage_bytes = 0;

	if (amdgpu_atom_parse_data_header(ctx, index, NULL, NULL, NULL, &data_offset)) {
		firmware_usage = (struct vram_usagebyfirmware_v2_1 *)(ctx->bios + data_offset);
		DRM_DEBUG("atom firmware requested %08x %dkb fw %dkb drv\n",
			  le32_to_cpu(firmware_usage->start_address_in_kb),
			  le16_to_cpu(firmware_usage->used_by_firmware_in_kb),
			  le16_to_cpu(firmware_usage->used_by_driver_in_kb));

		start_addr = le32_to_cpu(firmware_usage->start_address_in_kb);
		size = le16_to_cpu(firmware_usage->used_by_firmware_in_kb);

		if ((uint32_t)(start_addr & ATOM_VRAM_OPERATION_FLAGS_MASK) ==
			(uint32_t)(ATOM_VRAM_BLOCK_SRIOV_MSG_SHARE_RESERVATION <<
			ATOM_VRAM_OPERATION_FLAGS_SHIFT)) {
			/* Firmware request VRAM reservation for SR-IOV */
			adev->mman.fw_vram_usage_start_offset = (start_addr &
				(~ATOM_VRAM_OPERATION_FLAGS_MASK)) << 10;
			adev->mman.fw_vram_usage_size = size << 10;
			/* Use the default scratch size */
			usage_bytes = 0;
		} else {
			usage_bytes = le16_to_cpu(firmware_usage->used_by_driver_in_kb) << 10;
		}
	}
	ctx->scratch_size_bytes = 0;
	if (usage_bytes == 0)
		usage_bytes = 20 * 1024;
	/* allocate some scratch memory */
	ctx->scratch = kzalloc(usage_bytes, GFP_KERNEL);
	if (!ctx->scratch)
		return -ENOMEM;
	ctx->scratch_size_bytes = usage_bytes;
	return 0;
}

union igp_info {
	struct atom_integrated_system_info_v1_11 v11;
	struct atom_integrated_system_info_v1_12 v12;
};

union umc_info {
	struct atom_umc_info_v3_1 v31;
};

union vram_info {
	struct atom_vram_info_header_v2_3 v23;
	struct atom_vram_info_header_v2_4 v24;
	struct atom_vram_info_header_v2_5 v25;
};

union vram_module {
	struct atom_vram_module_v9 v9;
	struct atom_vram_module_v10 v10;
	struct atom_vram_module_v11 v11;
};

static int convert_atom_mem_type_to_vram_type(struct amdgpu_device *adev,
					      int atom_mem_type)
{
	int vram_type;

	if (adev->flags & AMD_IS_APU) {
		switch (atom_mem_type) {
		case Ddr2MemType:
		case LpDdr2MemType:
			vram_type = AMDGPU_VRAM_TYPE_DDR2;
			break;
		case Ddr3MemType:
		case LpDdr3MemType:
			vram_type = AMDGPU_VRAM_TYPE_DDR3;
			break;
		case Ddr4MemType:
		case LpDdr4MemType:
			vram_type = AMDGPU_VRAM_TYPE_DDR4;
			break;
		default:
			vram_type = AMDGPU_VRAM_TYPE_UNKNOWN;
			break;
		}
	} else {
		switch (atom_mem_type) {
		case ATOM_DGPU_VRAM_TYPE_GDDR5:
			vram_type = AMDGPU_VRAM_TYPE_GDDR5;
			break;
		case ATOM_DGPU_VRAM_TYPE_HBM2:
			vram_type = AMDGPU_VRAM_TYPE_HBM;
			break;
		case ATOM_DGPU_VRAM_TYPE_GDDR6:
			vram_type = AMDGPU_VRAM_TYPE_GDDR6;
			break;
		default:
			vram_type = AMDGPU_VRAM_TYPE_UNKNOWN;
			break;
		}
	}

	return vram_type;
}


int
amdgpu_atomfirmware_get_vram_info(struct amdgpu_device *adev,
				  int *vram_width, int *vram_type,
				  int *vram_vendor)
{
	struct amdgpu_mode_info *mode_info = &adev->mode_info;
	int index, i = 0;
	u16 data_offset, size;
	union igp_info *igp_info;
	union vram_info *vram_info;
	union vram_module *vram_module;
	u8 frev, crev;
	u8 mem_type;
	u8 mem_vendor;
	u32 mem_channel_number;
	u32 mem_channel_width;
	u32 module_id;

	if (adev->flags & AMD_IS_APU)
		index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
						    integratedsysteminfo);
	else
		index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
						    vram_info);

	if (amdgpu_atom_parse_data_header(mode_info->atom_context,
					  index, &size,
					  &frev, &crev, &data_offset)) {
		if (adev->flags & AMD_IS_APU) {
			igp_info = (union igp_info *)
				(mode_info->atom_context->bios + data_offset);
			switch (crev) {
			case 11:
				mem_channel_number = igp_info->v11.umachannelnumber;
				/* channel width is 64 */
				if (vram_width)
					*vram_width = mem_channel_number * 64;
				mem_type = igp_info->v11.memorytype;
				if (vram_type)
					*vram_type = convert_atom_mem_type_to_vram_type(adev, mem_type);
				break;
			case 12:
				mem_channel_number = igp_info->v12.umachannelnumber;
				/* channel width is 64 */
				if (vram_width)
					*vram_width = mem_channel_number * 64;
				mem_type = igp_info->v12.memorytype;
				if (vram_type)
					*vram_type = convert_atom_mem_type_to_vram_type(adev, mem_type);
				break;
			default:
				return -EINVAL;
			}
		} else {
			vram_info = (union vram_info *)
				(mode_info->atom_context->bios + data_offset);
			module_id = (RREG32(adev->bios_scratch_reg_offset + 4) & 0x00ff0000) >> 16;
			switch (crev) {
			case 3:
				if (module_id > vram_info->v23.vram_module_num)
					module_id = 0;
				vram_module = (union vram_module *)vram_info->v23.vram_module;
				while (i < module_id) {
					vram_module = (union vram_module *)
						((u8 *)vram_module + vram_module->v9.vram_module_size);
					i++;
				}
				mem_type = vram_module->v9.memory_type;
				if (vram_type)
					*vram_type = convert_atom_mem_type_to_vram_type(adev, mem_type);
				mem_channel_number = vram_module->v9.channel_num;
				mem_channel_width = vram_module->v9.channel_width;
				if (vram_width)
					*vram_width = mem_channel_number * (1 << mem_channel_width);
				mem_vendor = (vram_module->v9.vender_rev_id) & 0xF;
				if (vram_vendor)
					*vram_vendor = mem_vendor;
				break;
			case 4:
				if (module_id > vram_info->v24.vram_module_num)
					module_id = 0;
				vram_module = (union vram_module *)vram_info->v24.vram_module;
				while (i < module_id) {
					vram_module = (union vram_module *)
						((u8 *)vram_module + vram_module->v10.vram_module_size);
					i++;
				}
				mem_type = vram_module->v10.memory_type;
				if (vram_type)
					*vram_type = convert_atom_mem_type_to_vram_type(adev, mem_type);
				mem_channel_number = vram_module->v10.channel_num;
				mem_channel_width = vram_module->v10.channel_width;
				if (vram_width)
					*vram_width = mem_channel_number * (1 << mem_channel_width);
				mem_vendor = (vram_module->v10.vender_rev_id) & 0xF;
				if (vram_vendor)
					*vram_vendor = mem_vendor;
				break;
			case 5:
				if (module_id > vram_info->v25.vram_module_num)
					module_id = 0;
				vram_module = (union vram_module *)vram_info->v25.vram_module;
				while (i < module_id) {
					vram_module = (union vram_module *)
						((u8 *)vram_module + vram_module->v11.vram_module_size);
					i++;
				}
				mem_type = vram_module->v11.memory_type;
				if (vram_type)
					*vram_type = convert_atom_mem_type_to_vram_type(adev, mem_type);
				mem_channel_number = vram_module->v11.channel_num;
				mem_channel_width = vram_module->v11.channel_width;
				if (vram_width)
					*vram_width = mem_channel_number * (1 << mem_channel_width);
				mem_vendor = (vram_module->v11.vender_rev_id) & 0xF;
				if (vram_vendor)
					*vram_vendor = mem_vendor;
				break;
			default:
				return -EINVAL;
			}
		}

	}

	return 0;
}

/*
 * Return true if vbios enabled ecc by default, if umc info table is available
 * or false if ecc is not enabled or umc info table is not available
 */
bool amdgpu_atomfirmware_mem_ecc_supported(struct amdgpu_device *adev)
{
	struct amdgpu_mode_info *mode_info = &adev->mode_info;
	int index;
	u16 data_offset, size;
	union umc_info *umc_info;
	u8 frev, crev;
	bool ecc_default_enabled = false;

	index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
			umc_info);

	if (amdgpu_atom_parse_data_header(mode_info->atom_context,
				index, &size, &frev, &crev, &data_offset)) {
		/* support umc_info 3.1+ */
		if ((frev == 3 && crev >= 1) || (frev > 3)) {
			umc_info = (union umc_info *)
				(mode_info->atom_context->bios + data_offset);
			ecc_default_enabled =
				(le32_to_cpu(umc_info->v31.umc_config) &
				 UMC_CONFIG__DEFAULT_MEM_ECC_ENABLE) ? true : false;
		}
	}

	return ecc_default_enabled;
}

union firmware_info {
	struct atom_firmware_info_v3_1 v31;
	struct atom_firmware_info_v3_2 v32;
	struct atom_firmware_info_v3_3 v33;
	struct atom_firmware_info_v3_4 v34;
};

/*
 * Return true if vbios supports sram ecc or false if not
 */
bool amdgpu_atomfirmware_sram_ecc_supported(struct amdgpu_device *adev)
{
	struct amdgpu_mode_info *mode_info = &adev->mode_info;
	int index;
	u16 data_offset, size;
	union firmware_info *firmware_info;
	u8 frev, crev;
	bool sram_ecc_supported = false;

	index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
			firmwareinfo);

	if (amdgpu_atom_parse_data_header(adev->mode_info.atom_context,
				index, &size, &frev, &crev, &data_offset)) {
		/* support firmware_info 3.1 + */
		if ((frev == 3 && crev >=1) || (frev > 3)) {
			firmware_info = (union firmware_info *)
				(mode_info->atom_context->bios + data_offset);
			sram_ecc_supported =
				(le32_to_cpu(firmware_info->v31.firmware_capability) &
				 ATOM_FIRMWARE_CAP_SRAM_ECC) ? true : false;
		}
	}

	return sram_ecc_supported;
}

union smu_info {
	struct atom_smu_info_v3_1 v31;
};

int amdgpu_atomfirmware_get_clock_info(struct amdgpu_device *adev)
{
	struct amdgpu_mode_info *mode_info = &adev->mode_info;
	struct amdgpu_pll *spll = &adev->clock.spll;
	struct amdgpu_pll *mpll = &adev->clock.mpll;
	uint8_t frev, crev;
	uint16_t data_offset;
	int ret = -EINVAL, index;

	index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
					    firmwareinfo);
	if (amdgpu_atom_parse_data_header(mode_info->atom_context, index, NULL,
				   &frev, &crev, &data_offset)) {
		union firmware_info *firmware_info =
			(union firmware_info *)(mode_info->atom_context->bios +
						data_offset);

		adev->clock.default_sclk =
			le32_to_cpu(firmware_info->v31.bootup_sclk_in10khz);
		adev->clock.default_mclk =
			le32_to_cpu(firmware_info->v31.bootup_mclk_in10khz);

		adev->pm.current_sclk = adev->clock.default_sclk;
		adev->pm.current_mclk = adev->clock.default_mclk;

		/* not technically a clock, but... */
		adev->mode_info.firmware_flags =
			le32_to_cpu(firmware_info->v31.firmware_capability);

		ret = 0;
	}

	index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
					    smu_info);
	if (amdgpu_atom_parse_data_header(mode_info->atom_context, index, NULL,
				   &frev, &crev, &data_offset)) {
		union smu_info *smu_info =
			(union smu_info *)(mode_info->atom_context->bios +
					   data_offset);

		/* system clock */
		spll->reference_freq = le32_to_cpu(smu_info->v31.core_refclk_10khz);

		spll->reference_div = 0;
		spll->min_post_div = 1;
		spll->max_post_div = 1;
		spll->min_ref_div = 2;
		spll->max_ref_div = 0xff;
		spll->min_feedback_div = 4;
		spll->max_feedback_div = 0xff;
		spll->best_vco = 0;

		ret = 0;
	}

	index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
					    umc_info);
	if (amdgpu_atom_parse_data_header(mode_info->atom_context, index, NULL,
				   &frev, &crev, &data_offset)) {
		union umc_info *umc_info =
			(union umc_info *)(mode_info->atom_context->bios +
					   data_offset);

		/* memory clock */
		mpll->reference_freq = le32_to_cpu(umc_info->v31.mem_refclk_10khz);

		mpll->reference_div = 0;
		mpll->min_post_div = 1;
		mpll->max_post_div = 1;
		mpll->min_ref_div = 2;
		mpll->max_ref_div = 0xff;
		mpll->min_feedback_div = 4;
		mpll->max_feedback_div = 0xff;
		mpll->best_vco = 0;

		ret = 0;
	}

	return ret;
}

union gfx_info {
	struct  atom_gfx_info_v2_4 v24;
};

int amdgpu_atomfirmware_get_gfx_info(struct amdgpu_device *adev)
{
	struct amdgpu_mode_info *mode_info = &adev->mode_info;
	int index;
	uint8_t frev, crev;
	uint16_t data_offset;

	index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
					    gfx_info);
	if (amdgpu_atom_parse_data_header(mode_info->atom_context, index, NULL,
				   &frev, &crev, &data_offset)) {
		union gfx_info *gfx_info = (union gfx_info *)
			(mode_info->atom_context->bios + data_offset);
		switch (crev) {
		case 4:
			adev->gfx.config.max_shader_engines = gfx_info->v24.max_shader_engines;
			adev->gfx.config.max_cu_per_sh = gfx_info->v24.max_cu_per_sh;
			adev->gfx.config.max_sh_per_se = gfx_info->v24.max_sh_per_se;
			adev->gfx.config.max_backends_per_se = gfx_info->v24.max_backends_per_se;
			adev->gfx.config.max_texture_channel_caches = gfx_info->v24.max_texture_channel_caches;
			adev->gfx.config.max_gprs = le16_to_cpu(gfx_info->v24.gc_num_gprs);
			adev->gfx.config.max_gs_threads = gfx_info->v24.gc_num_max_gs_thds;
			adev->gfx.config.gs_vgt_table_depth = gfx_info->v24.gc_gs_table_depth;
			adev->gfx.config.gs_prim_buffer_depth =
				le16_to_cpu(gfx_info->v24.gc_gsprim_buff_depth);
			adev->gfx.config.double_offchip_lds_buf =
				gfx_info->v24.gc_double_offchip_lds_buffer;
			adev->gfx.cu_info.wave_front_size = le16_to_cpu(gfx_info->v24.gc_wave_size);
			adev->gfx.cu_info.max_waves_per_simd = le16_to_cpu(gfx_info->v24.gc_max_waves_per_simd);
			adev->gfx.cu_info.max_scratch_slots_per_cu = gfx_info->v24.gc_max_scratch_slots_per_cu;
			adev->gfx.cu_info.lds_size = le16_to_cpu(gfx_info->v24.gc_lds_size);
			return 0;
		default:
			return -EINVAL;
		}

	}
	return -EINVAL;
}

/*
 * Check if VBIOS supports GDDR6 training data save/restore
 */
static bool gddr6_mem_train_vbios_support(struct amdgpu_device *adev)
{
	uint16_t data_offset;
	int index;

	index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
					    firmwareinfo);
	if (amdgpu_atom_parse_data_header(adev->mode_info.atom_context, index, NULL,
					  NULL, NULL, &data_offset)) {
		struct atom_firmware_info_v3_1 *firmware_info =
			(struct atom_firmware_info_v3_1 *)(adev->mode_info.atom_context->bios +
							   data_offset);

		DRM_DEBUG("atom firmware capability:0x%08x.\n",
			  le32_to_cpu(firmware_info->firmware_capability));

		if (le32_to_cpu(firmware_info->firmware_capability) &
		    ATOM_FIRMWARE_CAP_ENABLE_2STAGE_BIST_TRAINING)
			return true;
	}

	return false;
}

int amdgpu_mem_train_support(struct amdgpu_device *adev)
{
	int ret;
	uint32_t major, minor, revision, hw_v;

	if (gddr6_mem_train_vbios_support(adev)) {
		amdgpu_discovery_get_ip_version(adev, MP0_HWID, &major, &minor, &revision);
		hw_v = HW_REV(major, minor, revision);
		/*
		 * treat 0 revision as a special case since register for MP0 and MMHUB is missing
		 * for some Navi10 A0, preventing driver from discovering the hwip information since
		 * none of the functions will be initialized, it should not cause any problems
		 */
		switch (hw_v) {
		case HW_REV(11, 0, 0):
		case HW_REV(11, 0, 5):
		case HW_REV(11, 0, 7):
		case HW_REV(11, 0, 11):
			ret = 1;
			break;
		default:
			DRM_ERROR("memory training vbios supports but psp hw(%08x)"
				  " doesn't support!\n", hw_v);
			ret = -1;
			break;
		}
	} else {
		ret = 0;
		hw_v = -1;
	}


	DRM_DEBUG("mp0 hw_v %08x, ret:%d.\n", hw_v, ret);
	return ret;
}

int amdgpu_atomfirmware_get_fw_reserved_fb_size(struct amdgpu_device *adev)
{
	struct atom_context *ctx = adev->mode_info.atom_context;
	union firmware_info *firmware_info;
	int index;
	u16 data_offset, size;
	u8 frev, crev;
	int fw_reserved_fb_size;

	index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
			firmwareinfo);

	if (!amdgpu_atom_parse_data_header(ctx, index, &size,
				&frev, &crev, &data_offset))
		/* fail to parse data_header */
		return 0;

	firmware_info = (union firmware_info *)(ctx->bios + data_offset);

	if (frev !=3)
		return -EINVAL;

	switch (crev) {
	case 4:
		fw_reserved_fb_size =
			(firmware_info->v34.fw_reserved_size_in_kb << 10);
		break;
	default:
		fw_reserved_fb_size = 0;
		break;
	}

	return fw_reserved_fb_size;
}
