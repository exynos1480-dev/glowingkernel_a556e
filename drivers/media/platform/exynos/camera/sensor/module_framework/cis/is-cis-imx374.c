// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Sensor driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/videodev2.h>
#include <videodev2_exynos_camera.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include <exynos-is-sensor.h>
#include "is-hw.h"
#include "is-core.h"
#include "is-param.h"
#include "is-device-sensor.h"
#include "is-device-sensor-peri.h"
#include "is-resourcemgr.h"
#include "is-dt.h"
#include "is-cis-imx374.h"
#include "is-vendor.h"
#include "is-cis-imx374-setA-19p2.h"
#include "is-helper-ixc.h"

#define SENSOR_NAME "IMX374"

static const u32 *sensor_imx374_global;
static u32 sensor_imx374_global_size;
static const u32 **sensor_imx374_setfiles;
static const u32 *sensor_imx374_setfile_sizes;

static const struct sensor_pll_info_compact **sensor_imx374_pllinfos;
static u32 sensor_imx374_max_setfile_num;

/*************************************************
 *  [imx374 Analog gain formular]
 *
 *  m0: [0x008c:0x008d] fixed to 0
 *  m1: [0x0090:0x0091] fixed to -1
 *  c0: [0x008e:0x008f] fixed to 1024
 *  c1: [0x0092:0x0093] fixed to 1024
 *  X : [0x0204:0x0205] Analog gain setting value
 *
 *  Analog Gain = (m0 * X + c0) / (m1 * X + c1)
 *              = 1024 / (1024 - X)
 *
 *  Analog Gain Range = 0 to 896 (0dB to 18dB)
 *
 *************************************************/

u32 sensor_imx374_cis_calc_again_code(u32 permille)
{
	return 1024 - (1024000 / permille);
}

u32 sensor_imx374_cis_calc_again_permile(u32 code)
{
	return 1024000 / (1024 - code);
}

static void sensor_imx374_set_integration_max_margin(u32 mode, cis_shared_data *cis_data)
{
	WARN_ON(!cis_data);

	if (mode < 0 || mode >= SENSOR_IMX374_MODE_MAX)
		err("invalid mode(%d)!!", mode);

	cis_data->max_margin_coarse_integration_time = SENSOR_IMX374_COARSE_INTEGRATION_TIME_MAX_MARGIN;

	dbg_sensor(1, "max_margin_coarse_integration_time(%d)\n", cis_data->max_margin_coarse_integration_time);
}

static void sensor_imx374_cis_data_calculation(const struct sensor_pll_info_compact *pll_info_compact,
	cis_shared_data *cis_data)
{
	u64 vt_pix_clk_hz;
	u32 frame_rate, max_fps, frame_valid_us;

	WARN_ON(!pll_info_compact);

	/* 1. get pclk value from pll info */
	vt_pix_clk_hz = pll_info_compact->pclk;

	dbg_sensor(1, "ext_clock(%d), mipi_datarate(%llu), pclk(%llu)\n",
			pll_info_compact->ext_clk, pll_info_compact->mipi_datarate, pll_info_compact->pclk);

	/* 2. the time of processing one frame calculation (us) */
	cis_data->min_frame_us_time = (((u64)pll_info_compact->frame_length_lines) *
					pll_info_compact->line_length_pck * 1000
					/ (vt_pix_clk_hz / 1000));
	cis_data->cur_frame_us_time = cis_data->min_frame_us_time;
	cis_data->base_min_frame_us_time = cis_data->min_frame_us_time;

	/* 3. FPS calculation */
	frame_rate = vt_pix_clk_hz / (pll_info_compact->frame_length_lines * pll_info_compact->line_length_pck);
	dbg_sensor(1, "frame_rate (%d) = vt_pix_clk_hz(%llu) / "
		KERN_CONT "(pll_info_compact->frame_length_lines(%d) * pll_info_compact->line_length_pck(%d))\n",
		frame_rate, vt_pix_clk_hz, pll_info_compact->frame_length_lines, pll_info_compact->line_length_pck);

	/* calculate max fps */
	max_fps = (vt_pix_clk_hz * 10) / (pll_info_compact->frame_length_lines * pll_info_compact->line_length_pck);
	max_fps = (max_fps % 10 >= 5 ? frame_rate + 1 : frame_rate);

	cis_data->pclk = vt_pix_clk_hz;
	cis_data->max_fps = max_fps;
	cis_data->frame_length_lines = pll_info_compact->frame_length_lines;
	cis_data->line_length_pck = pll_info_compact->line_length_pck;
	cis_data->line_readOut_time = (u64)cis_data->line_length_pck * 1000
					* 1000 * 1000 / cis_data->pclk;
	cis_data->rolling_shutter_skew = (cis_data->cur_height - 1) * cis_data->line_readOut_time;
	cis_data->stream_on = false;

	/* Frame valid time calculation */
	frame_valid_us = (u64)cis_data->cur_height * cis_data->line_length_pck
				* 1000 * 1000 / cis_data->pclk;
	cis_data->frame_valid_us_time = (unsigned int)frame_valid_us;

	dbg_sensor(1, "%s\n", __func__);
	dbg_sensor(1, "Sensor size(%d x %d) setting: SUCCESS!\n",
					cis_data->cur_width, cis_data->cur_height);
	dbg_sensor(1, "Frame Valid(us): %d\n", frame_valid_us);
	dbg_sensor(1, "rolling_shutter_skew: %lld\n", cis_data->rolling_shutter_skew);

	dbg_sensor(1, "Fps: %d, max fps(%d)\n", frame_rate, cis_data->max_fps);
	dbg_sensor(1, "min_frame_time(%d us)\n", cis_data->min_frame_us_time);
	dbg_sensor(1, "Pixel rate(Kbps): %d\n", cis_data->pclk / 1000);

	/* Frame period calculation */
	cis_data->frame_time = (cis_data->line_readOut_time * cis_data->cur_height / 1000);
	cis_data->rolling_shutter_skew = (cis_data->cur_height - 1) * cis_data->line_readOut_time;

	dbg_sensor(1, "[%s] frame_time(%d), rolling_shutter_skew(%lld)\n", __func__,
		cis_data->frame_time, cis_data->rolling_shutter_skew);

	/* Constant values */
	cis_data->min_fine_integration_time = SENSOR_IMX374_FINE_INTEGRATION_TIME_MIN;
	cis_data->max_fine_integration_time = SENSOR_IMX374_FINE_INTEGRATION_TIME_MAX;
	cis_data->min_coarse_integration_time = SENSOR_IMX374_COARSE_INTEGRATION_TIME_MIN;
	cis_data->max_margin_coarse_integration_time = SENSOR_IMX374_COARSE_INTEGRATION_TIME_MAX_MARGIN;
}

void sensor_imx374_cis_data_calc(struct v4l2_subdev *subdev, u32 mode)
{
	struct is_cis *cis = NULL;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	if (mode >= sensor_imx374_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		return;
	}

	sensor_imx374_cis_data_calculation(sensor_imx374_pllinfos[mode], cis->cis_data);
}

/* CIS OPS */
int sensor_imx374_cis_init(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	u32 setfile_index = 0;
	cis_setting_info setinfo;

	setinfo.param = NULL;
	setinfo.return_value = 0;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	WARN_ON(!cis->cis_data);

	cis->cis_data->stream_on = false;
	cis->cis_data->cur_width = SENSOR_IMX374_MAX_WIDTH;
	cis->cis_data->cur_height = SENSOR_IMX374_MAX_HEIGHT;
	cis->cis_data->cur_pattern_mode = SENSOR_TEST_PATTERN_MODE_OFF;
	cis->cis_data->low_expo_start = 33000;
	cis->need_mode_change = false;
#ifdef USE_CAMERA_ADAPTIVE_MIPI
	cis->mipi_clock_index_cur = CAM_MIPI_NOT_INITIALIZED;
	cis->mipi_clock_index_new = CAM_MIPI_NOT_INITIALIZED;
#endif

	sensor_imx374_cis_data_calculation(sensor_imx374_pllinfos[setfile_index], cis->cis_data);
	sensor_imx374_set_integration_max_margin(setfile_index, cis->cis_data);

	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_min_exposure_time, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] min exposure time : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_max_exposure_time, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] max exposure time : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_min_analog_gain, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] min again : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_max_analog_gain, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] max again : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_min_digital_gain, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] min dgain : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_max_digital_gain, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] max dgain : %d\n", __func__, setinfo.return_value);

p_err:
	return ret;
}

static const struct is_cis_log log_imx374[] = {
	{I2C_READ, 16, 0x0000, 0, "model_id"},
	{I2C_READ, 16, 0x00DC, 0, "0x00DC"},
	{I2C_READ, 16, 0x00EA, 0, "0x00EA"},
	{I2C_READ, 8, 0x0002, 0, "revision_number"},
	{I2C_READ, 8, 0x0005, 0, "frame_count"},
	{I2C_READ, 8, 0x0100, 0, "0x0100"},
	{I2C_READ, 16, 0x0202, 0, "0x0202"},
	{I2C_READ, 16, 0x0204, 0, "0x0204"},
	{I2C_READ, 16, 0x0340, 0, "0x0340"},
};

int sensor_imx374_cis_log_status(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	sensor_cis_log_status(cis, log_imx374,
			ARRAY_SIZE(log_imx374), (char *)__func__);
p_err:
	return ret;
}

#if USE_GROUP_PARAM_HOLD
static int sensor_imx374_cis_group_param_hold_func(struct v4l2_subdev *subdev, unsigned int hold)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (hold == cis->cis_data->group_param_hold) {
		pr_debug("already group_param_hold (%d)\n", cis->cis_data->group_param_hold);
		goto p_err;
	}

	ret = cis->ixc_ops->write8(cis->client, 0x0104, hold);
	if (ret < 0)
		goto p_err;

	cis->cis_data->group_param_hold = hold;
	ret = 1;
p_err:
	return ret;
}
#else
static inline int sensor_imx374_cis_group_param_hold_func(struct v4l2_subdev *subdev, unsigned int hold)
{ return 0; }
#endif

/* Input
 *	hold : true - hold, flase - no hold
 * Output
 *      return: 0 - no effect(already hold or no hold)
 *		positive - setted by request
 *		negative - ERROR value
 */
int sensor_imx374_cis_group_param_hold(struct v4l2_subdev *subdev, bool hold)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = sensor_imx374_cis_group_param_hold_func(subdev, hold);
	if (ret < 0)
		goto p_err;

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

int sensor_imx374_cis_set_global_setting(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	/* setfile global setting is at camera entrance */
	info("[%d][%s] global setting enter\n", cis->id, __func__);
	ret = sensor_cis_set_registers(subdev, sensor_imx374_global, sensor_imx374_global_size);

	if (ret < 0) {
		err("[%d]sensor_imx374_set_registers fail!!", cis->id);
		goto p_err;
	}

	info("[%d][%s] global setting done\n", cis->id, __func__);

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx374_cis_mode_change(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct is_cis *cis = NULL;
	struct is_device_sensor *device;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	if (unlikely(!device)) {
		err("device sensor is null");
		return -EINVAL;
	}

	if (mode >= sensor_imx374_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		ret = -EINVAL;
		goto p_err;
	}

	sensor_imx374_set_integration_max_margin(mode, cis->cis_data);

	cis->mipi_clock_index_cur = CAM_MIPI_NOT_INITIALIZED;

	IXC_MUTEX_LOCK(cis->ixc_lock);

	ret = sensor_cis_set_registers(subdev, sensor_imx374_setfiles[mode], sensor_imx374_setfile_sizes[mode]);
	if (ret < 0) {
		err("[%d]sensor_imx374_set_registers fail!!", cis->id);
		goto p_err_i2c_unlock;
	}

	info("[%d][%s] mode changed(%d)\n", cis->id, __func__, mode);

	/* HDR OFF */
	ret |= cis->ixc_ops->write8(cis->client, 0x0220, 0x00);

	/* EMB OFF */
	ret |= cis->ixc_ops->write8(cis->client, 0xBCF0, 0x00);
	ret |= cis->ixc_ops->write8(cis->client, 0xBCF1, 0x00);

p_err_i2c_unlock:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_imx374_cis_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct is_device_sensor *device;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	WARN_ON(!sensor_peri);

	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	WARN_ON(!device);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);

	is_vendor_set_mipi_clock(device);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	ret = sensor_imx374_cis_group_param_hold_func(subdev, 0x01);
	if (ret < 0)
		err("group_param_hold_func failed at stream on");

	/* Sensor stream on */
	info("%s\n", __func__);
	cis->ixc_ops->write8(cis->client, 0x0100, 0x01);

	ret = sensor_imx374_cis_group_param_hold_func(subdev, 0x00);
	if (ret < 0)
		err("group_param_hold_func failed at stream on");

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	cis_data->stream_on = true;

p_err:
	return ret;
}

int sensor_imx374_cis_set_test_pattern(struct v4l2_subdev *subdev, camera2_sensor_ctl_t *sensor_ctl)
{
	int ret = 0;
	struct is_cis *cis;

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	dbg_sensor(1, "[MOD:D:%d] %s, cur_pattern_mode(%d), testPatternMode(%d)\n", cis->id, __func__,
			cis->cis_data->cur_pattern_mode, sensor_ctl->testPatternMode);

	if (cis->cis_data->cur_pattern_mode != sensor_ctl->testPatternMode) {
		info("%s REG : 0xB000 write to 0x00\n", __func__);
		cis->ixc_ops->write8(cis->client, 0xB000, 0x00);

		if (sensor_ctl->testPatternMode == SENSOR_TEST_PATTERN_MODE_OFF) {
			info("%s: set DEFAULT pattern! (testpatternmode : %d)\n", __func__, sensor_ctl->testPatternMode);

			IXC_MUTEX_LOCK(cis->ixc_lock);
			cis->ixc_ops->write16(cis->client, 0x0600, 0x0000);
			IXC_MUTEX_UNLOCK(cis->ixc_lock);

			cis->cis_data->cur_pattern_mode = sensor_ctl->testPatternMode;
		} else if (sensor_ctl->testPatternMode == SENSOR_TEST_PATTERN_MODE_BLACK) {
			info("%s: set BLACK pattern! (testpatternmode :%d), Data : 0x(%x, %x, %x, %x)\n",
				__func__, sensor_ctl->testPatternMode,
				(unsigned short)sensor_ctl->testPatternData[0],
				(unsigned short)sensor_ctl->testPatternData[1],
				(unsigned short)sensor_ctl->testPatternData[2],
				(unsigned short)sensor_ctl->testPatternData[3]);

			IXC_MUTEX_LOCK(cis->ixc_lock);
			cis->ixc_ops->write16(cis->client, 0x0600, 0x0001);
			IXC_MUTEX_UNLOCK(cis->ixc_lock);

			cis->cis_data->cur_pattern_mode = sensor_ctl->testPatternMode;
		}
	}

p_err:
	return ret;
}

int sensor_imx374_cis_stream_off(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u8 cur_frame_count = 0;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = sensor_imx374_cis_group_param_hold_func(subdev, 0x00);
	if (ret < 0)
		err("group_param_hold_func failed at stream off");

	cis->ixc_ops->read8(cis->client, 0x0005, &cur_frame_count);
	info("%s: frame_count(0x%x)\n", __func__, cur_frame_count);

	cis->ixc_ops->write8(cis->client, 0x0100, 0x00);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	cis_data->stream_on = false;

p_err:
	return ret;
}

int sensor_imx374_cis_set_exposure_time(struct v4l2_subdev *subdev, struct ae_param *target_exposure)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u64 vt_pic_clk_freq_khz = 0;
	u16 long_coarse_int = 0;
	u16 short_coarse_int = 0;
	u32 line_length_pck = 0;
	u32 min_fine_int = 0;

	WARN_ON(!subdev);
	WARN_ON(!target_exposure);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if ((target_exposure->long_val <= 0) || (target_exposure->short_val <= 0)) {
		err("[%d] invalid target exposure(%d, %d)", cis->id,
				target_exposure->long_val, target_exposure->short_val);
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), target long(%d), short(%d)\n", cis->id, __func__,
			cis_data->sen_vsync_count, target_exposure->long_val, target_exposure->short_val);

	vt_pic_clk_freq_khz = cis_data->pclk / 1000;
	line_length_pck = cis_data->line_length_pck;
	min_fine_int = cis_data->min_fine_integration_time;

	dbg_sensor(1, "[MOD:D:%d] %s, vt_pic_clk_freq_khz (%d), line_length_pck(%d), min_fine_int (%d)\n",
		cis->id, __func__, vt_pic_clk_freq_khz, line_length_pck, min_fine_int);

	long_coarse_int = ((target_exposure->long_val * vt_pic_clk_freq_khz) / 1000 - min_fine_int) / line_length_pck;
	short_coarse_int = ((target_exposure->short_val * vt_pic_clk_freq_khz) / 1000 - min_fine_int) / line_length_pck;

	if (long_coarse_int > cis_data->max_coarse_integration_time) {
		long_coarse_int = cis_data->max_coarse_integration_time;
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) max\n", cis->id, __func__,
			cis_data->sen_vsync_count, long_coarse_int);
	}

	if (short_coarse_int > cis_data->max_coarse_integration_time) {
		short_coarse_int = cis_data->max_coarse_integration_time;
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), short coarse(%d) max\n", cis->id, __func__,
			cis_data->sen_vsync_count, short_coarse_int);
	}

	if (long_coarse_int < cis_data->min_coarse_integration_time) {
		long_coarse_int = cis_data->min_coarse_integration_time;
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) min\n", cis->id, __func__,
			cis_data->sen_vsync_count, long_coarse_int);
	}

	if (short_coarse_int < cis_data->min_coarse_integration_time) {
		short_coarse_int = cis_data->min_coarse_integration_time;
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), short coarse(%d) min\n", cis->id, __func__,
			cis_data->sen_vsync_count, short_coarse_int);
	}

	if (short_coarse_int > long_coarse_int) {
		dbg_sensor(1, "[MOD:D:%d] %s, long coarse(%d), short coarse(%d) max\n", cis->id, __func__,
			long_coarse_int, short_coarse_int);
		long_coarse_int = short_coarse_int;
	}

	dbg_sensor(1, "[MOD:D:%d] %s, frame_length_lines(%#x), long_coarse_int %#x, short_coarse_int %#x\n",
		cis->id, __func__, cis_data->frame_length_lines, long_coarse_int, short_coarse_int);

	cis_data->cur_long_exposure_coarse = long_coarse_int;
	cis_data->cur_short_exposure_coarse = short_coarse_int;

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx374_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_i2c_unlock;
	}

	/* Short exposure */
	ret = cis->ixc_ops->write16(cis->client, 0x0202, short_coarse_int);
	if (ret < 0)
		goto p_err_i2c_unlock;

p_err_i2c_unlock:
	if (hold > 0) {
		hold = sensor_imx374_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_imx374_cis_get_min_exposure_time(struct v4l2_subdev *subdev, u32 *min_expo)
{
	int ret = 0;
	struct is_cis *cis = NULL;
	cis_shared_data *cis_data = NULL;
	u32 min_integration_time = 0;
	u32 min_coarse = 0;
	u32 min_fine = 0;
	u64 vt_pic_clk_freq_khz = 0;
	u32 line_length_pck = 0;

	WARN_ON(!subdev);
	WARN_ON(!min_expo);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	vt_pic_clk_freq_khz = cis_data->pclk / 1000;
	if (vt_pic_clk_freq_khz == 0) {
		err("[%d] Invalid vt_pic_clk_freq_khz(%d)", cis->id, vt_pic_clk_freq_khz);
		goto p_err;
	}
	line_length_pck = cis_data->line_length_pck;
	min_coarse = cis_data->min_coarse_integration_time;
	min_fine = cis_data->min_fine_integration_time;

	min_integration_time = (u32)((u64)((line_length_pck * min_coarse) + min_fine) * 1000 / vt_pic_clk_freq_khz);
	*min_expo = min_integration_time;

	dbg_sensor(1, "[%s] min integration time %d\n", __func__, min_integration_time);

p_err:
	return ret;
}

int sensor_imx374_cis_get_max_exposure_time(struct v4l2_subdev *subdev, u32 *max_expo)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u32 max_integration_time = 0;
	u32 max_coarse_margin = 0;
	u32 max_fine_margin = 0;
	u32 max_coarse = 0;
	u32 max_fine = 0;
	u64 vt_pic_clk_freq_khz = 0;
	u32 line_length_pck = 0;
	u32 frame_length_lines = 0;

	WARN_ON(!subdev);
	WARN_ON(!max_expo);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	vt_pic_clk_freq_khz = cis_data->pclk / 1000;
	if (vt_pic_clk_freq_khz == 0) {
		err("[MOD:D:%d] Invalid vt_pic_clk_freq_khz(%d)", cis->id, vt_pic_clk_freq_khz);
		goto p_err;
	}
	line_length_pck = cis_data->line_length_pck;
	frame_length_lines = cis_data->frame_length_lines;

	max_coarse_margin = cis_data->max_margin_coarse_integration_time;
	max_fine_margin = line_length_pck - cis_data->min_fine_integration_time;
	max_coarse = frame_length_lines - max_coarse_margin;
	max_fine = cis_data->max_fine_integration_time;

	max_integration_time = (u32)((u64)((line_length_pck * max_coarse) + max_fine) * 1000 / vt_pic_clk_freq_khz);

	*max_expo = max_integration_time;

	/* TODO: Is this values update hear? */
	cis_data->max_margin_fine_integration_time = max_fine_margin;
	cis_data->max_coarse_integration_time = max_coarse;

	dbg_sensor(1, "[%s] max integration time %d, max margin fine integration %d, max coarse integration %d\n",
			__func__, max_integration_time,
			cis_data->max_margin_fine_integration_time,
			cis_data->max_coarse_integration_time);

p_err:
	return ret;
}

int sensor_imx374_cis_adjust_frame_duration(struct v4l2_subdev *subdev,
						u32 input_exposure_time,
						u32 *target_duration)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	u64 vt_pic_clk_freq_khz = 0;
	u32 line_length_pck = 0;
	u32 frame_length_lines = 0;
	u32 frame_duration = 0;

	WARN_ON(!subdev);
	WARN_ON(!target_duration);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	if (input_exposure_time == 0) {
		input_exposure_time  = cis_data->min_frame_us_time;
		info("[%d][%s] Not proper exposure time(0), apply min frame duration to exposure time forcely! (%d)\n",
			cis->id, __func__, cis_data->min_frame_us_time);
	}

	vt_pic_clk_freq_khz = cis_data->pclk / 1000;
	line_length_pck = cis_data->line_length_pck;
	frame_length_lines = (u32)(((vt_pic_clk_freq_khz * input_exposure_time) / 1000
						- cis_data->min_fine_integration_time) / line_length_pck);
	frame_length_lines += cis_data->max_margin_coarse_integration_time;

	frame_duration = (u32)(((u64)frame_length_lines * line_length_pck) * 1000 / vt_pic_clk_freq_khz);

	dbg_sensor(1, "[%s] input exp(%d), adj duration, frame duraion(%d), min_frame_us(%d)\n",
			__func__, input_exposure_time, frame_duration, cis_data->min_frame_us_time);
	dbg_sensor(1, "[%s] adj duration, frame duraion(%d), min_frame_us(%d)\n",
			__func__, frame_duration, cis_data->min_frame_us_time);

	*target_duration = MAX(frame_duration, cis_data->min_frame_us_time);

	return ret;
}

int sensor_imx374_cis_set_frame_duration(struct v4l2_subdev *subdev, u32 frame_duration)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u64 vt_pic_clk_freq_khz = 0;
	u32 line_length_pck = 0;
	u16 frame_length_lines = 0;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	if (frame_duration < cis_data->min_frame_us_time) {
		dbg_sensor(1, "frame duration is less than min(%d)\n", frame_duration);
		frame_duration = cis_data->min_frame_us_time;
	}

	vt_pic_clk_freq_khz = cis_data->pclk / 1000;
	line_length_pck = cis_data->line_length_pck;

	frame_length_lines = (u16)((vt_pic_clk_freq_khz * frame_duration) / (line_length_pck * 1000));

	dbg_sensor(1, "[MOD:D:%d] %s, vt_pic_clk_freq_khz(%#x) frame_duration = %d us,"
			KERN_CONT "(line_length_pck%#x), frame_length_lines(%#x)\n",
			cis->id, __func__, vt_pic_clk_freq_khz, frame_duration,
			line_length_pck, frame_length_lines);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx374_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_i2c_unlock;
	}

	ret = cis->ixc_ops->write16(cis->client, 0x0340, frame_length_lines);
	if (ret < 0)
		goto p_err_i2c_unlock;

	cis_data->cur_frame_us_time = frame_duration;
	cis_data->frame_length_lines = frame_length_lines;
	cis_data->max_coarse_integration_time = cis_data->frame_length_lines -
						cis_data->max_margin_coarse_integration_time;

p_err_i2c_unlock:
	if (hold > 0) {
		hold = sensor_imx374_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_imx374_cis_set_frame_rate(struct v4l2_subdev *subdev, u32 min_fps)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	u32 frame_duration = 0;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	if (min_fps > cis_data->max_fps) {
		err("[%d] request FPS is too high(%d), set to max(%d)", cis->id, min_fps, cis_data->max_fps);
		min_fps = cis_data->max_fps;
	}

	if (min_fps == 0) {
		err("[%d] request FPS is 0, set to min FPS(1)", cis->id);
		min_fps = 1;
	}

	frame_duration = (1 * 1000 * 1000) / min_fps;

	dbg_sensor(1, "[MOD:D:%d] %s, set FPS(%d), frame duration(%d)\n",
			cis->id, __func__, min_fps, frame_duration);

	ret = sensor_imx374_cis_set_frame_duration(subdev, frame_duration);
	if (ret < 0) {
		err("[%d] set frame duration is fail(%d)", cis->id, ret);
		goto p_err;
	}

	cis_data->min_frame_us_time = MAX(frame_duration, cis_data->base_min_frame_us_time);

p_err:

	return ret;
}

int sensor_imx374_cis_adjust_analog_gain(struct v4l2_subdev *subdev, u32 input_again, u32 *target_permile)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	u32 again_code = 0;
	u32 again_permile = 0;

	WARN_ON(!subdev);
	WARN_ON(!target_permile);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	again_code = sensor_imx374_cis_calc_again_code(input_again);

	if (again_code > cis_data->max_analog_gain[0])
		again_code = cis_data->max_analog_gain[0];
	else if (again_code < cis_data->min_analog_gain[0])
		again_code = cis_data->min_analog_gain[0];

	again_permile = sensor_imx374_cis_calc_again_permile(again_code);

	dbg_sensor(1, "[%s] min again(%d), max(%d), input_again(%d), code(%d), permile(%d)\n", __func__,
			cis_data->max_analog_gain[0],
			cis_data->min_analog_gain[0],
			input_again,
			again_code,
			again_permile);

	*target_permile = again_permile;

	return ret;
}

int sensor_imx374_cis_set_analog_gain(struct v4l2_subdev *subdev, struct ae_param *again)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u16 short_gain = 0;

	WARN_ON(!subdev);
	WARN_ON(!again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	short_gain = (u16)sensor_imx374_cis_calc_again_code(again->short_val);

	if (short_gain < cis->cis_data->min_analog_gain[0]) {
		info("[%d][%s] not proper short_gain value, reset to min_analog_gain\n", cis->id, __func__);
		short_gain = cis->cis_data->min_analog_gain[0];
	}
	if (short_gain > cis->cis_data->max_analog_gain[0]) {
		info("[%d][%s] not proper short_gain value, reset to max_analog_gain\n", cis->id, __func__);
		short_gain = cis->cis_data->max_analog_gain[0];
	}

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt = %d),"
		KERN_CONT "input short gain = %d us, "
		KERN_CONT "analog short gain(%#x)\n",
		cis->id, __func__, cis->cis_data->sen_vsync_count,
		again->short_val, short_gain);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx374_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_i2c_unlock;
	}

	/* Short analog gain */
	ret = cis->ixc_ops->write16(cis->client, 0x0204, short_gain);
	if (ret < 0)
		goto p_err_i2c_unlock;

p_err_i2c_unlock:
	if (hold > 0) {
		hold = sensor_imx374_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_imx374_cis_get_analog_gain(struct v4l2_subdev *subdev, u32 *again)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	u16 analog_gain = 0;

	WARN_ON(!subdev);
	WARN_ON(!again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx374_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_i2c_unlock;
	}

	ret = cis->ixc_ops->read16(cis->client, 0x0204, &analog_gain);
	if (ret < 0)
		goto p_err_i2c_unlock;

	*again = sensor_imx374_cis_calc_again_permile(analog_gain);

	dbg_sensor(1, "[MOD:D:%d] %s, cur_again = %d us, analog_gain(%#x)\n",
			cis->id, __func__, *again, analog_gain);

p_err_i2c_unlock:
	if (hold > 0) {
		hold = sensor_imx374_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_imx374_cis_get_min_analog_gain(struct v4l2_subdev *subdev, u32 *min_again)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	WARN_ON(!subdev);
	WARN_ON(!min_again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;
	cis_data->min_analog_gain[0] = 0;
	cis_data->min_analog_gain[1] = sensor_imx374_cis_calc_again_permile(cis_data->min_analog_gain[0]);

	*min_again = cis_data->min_analog_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->min_analog_gain[0], cis_data->min_analog_gain[1]);

	return ret;
}

int sensor_imx374_cis_get_max_analog_gain(struct v4l2_subdev *subdev, u32 *max_again)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	WARN_ON(!subdev);
	WARN_ON(!max_again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;
	cis_data->max_analog_gain[0] = 0x380;
	cis_data->max_analog_gain[1] = sensor_imx374_cis_calc_again_permile(cis_data->max_analog_gain[0]);

	*max_again = cis_data->max_analog_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->max_analog_gain[0], cis_data->max_analog_gain[1]);

	return ret;
}

int sensor_imx374_cis_set_digital_gain(struct v4l2_subdev *subdev, struct ae_param *dgain)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u16 short_gain = 0;

	WARN_ON(!subdev);
	WARN_ON(!dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	short_gain = (u16)sensor_cis_calc_dgain_code(dgain->short_val);

	if (short_gain < cis->cis_data->min_digital_gain[0]) {
		info("[%d][%s] not proper short_gain value, reset to min_digital_gain\n", cis->id, __func__);
		short_gain = cis->cis_data->min_digital_gain[0];
	}
	if (short_gain > cis->cis_data->max_digital_gain[0]) {
		info("[%d][%s] not proper short_gain value, reset to max_digital_gain\n", cis->id, __func__);
		short_gain = cis->cis_data->max_digital_gain[0];
	}

	dbg_sensor(1, "[MOD:D:%d] %s, input_dgain = %d/%d us, short_gain(%#x)\n",
			cis->id, __func__, dgain->long_val, dgain->short_val, short_gain);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx374_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_i2c_unlock;
	}

	/* Short digital gain */
	ret = cis->ixc_ops->write16(cis->client, 0x020E, short_gain);
	if (ret < 0)
		goto p_err_i2c_unlock;

p_err_i2c_unlock:
	if (hold > 0) {
		hold = sensor_imx374_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_imx374_cis_get_digital_gain(struct v4l2_subdev *subdev, u32 *dgain)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	u16 digital_gain = 0;

	WARN_ON(!subdev);
	WARN_ON(!dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx374_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_i2c_unlock;
	}

	ret = cis->ixc_ops->read16(cis->client, 0x020E, &digital_gain);
	if (ret < 0)
		goto p_err_i2c_unlock;

	*dgain = sensor_cis_calc_dgain_permile(digital_gain);

	dbg_sensor(1, "[MOD:D:%d] %s, cur_dgain = %d us, digital_gain(%#x)\n",
			cis->id, __func__, *dgain, digital_gain);

p_err_i2c_unlock:
	if (hold > 0) {
		hold = sensor_imx374_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_imx374_cis_get_min_digital_gain(struct v4l2_subdev *subdev, u32 *min_dgain)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	WARN_ON(!subdev);
	WARN_ON(!min_dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;
	cis_data->min_digital_gain[0] = 0x100;
	cis_data->min_digital_gain[1] = sensor_cis_calc_dgain_permile(cis_data->min_digital_gain[0]);

	*min_dgain = cis_data->min_digital_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->min_digital_gain[0], cis_data->min_digital_gain[1]);

	return ret;
}

int sensor_imx374_cis_get_max_digital_gain(struct v4l2_subdev *subdev, u32 *max_dgain)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	WARN_ON(!subdev);
	WARN_ON(!max_dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;
	cis_data->max_digital_gain[0] = 0xFD9;
	cis_data->max_digital_gain[1] = sensor_cis_calc_dgain_permile(cis_data->max_digital_gain[0]);

	*max_dgain = cis_data->max_digital_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->max_digital_gain[0], cis_data->max_digital_gain[1]);

	return ret;
}

static int sensor_imx374_cis_check_rev_on_init(struct v4l2_subdev *subdev)
{
	struct is_cis *cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	if (cis->ixc_ops->write8(cis->client, 0xA02, 0x5F)) {
		IXC_MUTEX_UNLOCK(cis->ixc_lock);
		return -EAGAIN;
	}

	if (cis->ixc_ops->write8(cis->client, 0xA00, 0x01)) {
		IXC_MUTEX_UNLOCK(cis->ixc_lock);
		return -EAGAIN;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return sensor_cis_check_rev_on_init(subdev);
}

static struct is_cis_ops cis_ops = {
	.cis_init = sensor_imx374_cis_init,
	.cis_log_status = sensor_imx374_cis_log_status,
	.cis_group_param_hold = sensor_imx374_cis_group_param_hold,
	.cis_set_global_setting = sensor_imx374_cis_set_global_setting,
	.cis_mode_change = sensor_imx374_cis_mode_change,
	.cis_stream_on = sensor_imx374_cis_stream_on,
	.cis_stream_off = sensor_imx374_cis_stream_off,
	.cis_set_exposure_time = sensor_imx374_cis_set_exposure_time,
	.cis_get_min_exposure_time = sensor_imx374_cis_get_min_exposure_time,
	.cis_get_max_exposure_time = sensor_imx374_cis_get_max_exposure_time,
	.cis_adjust_frame_duration = sensor_imx374_cis_adjust_frame_duration,
	.cis_set_frame_duration = sensor_imx374_cis_set_frame_duration,
	.cis_set_frame_rate = sensor_imx374_cis_set_frame_rate,
	.cis_adjust_analog_gain = sensor_imx374_cis_adjust_analog_gain,
	.cis_set_analog_gain = sensor_imx374_cis_set_analog_gain,
	.cis_get_analog_gain = sensor_imx374_cis_get_analog_gain,
	.cis_get_min_analog_gain = sensor_imx374_cis_get_min_analog_gain,
	.cis_get_max_analog_gain = sensor_imx374_cis_get_max_analog_gain,
	.cis_set_digital_gain = sensor_imx374_cis_set_digital_gain,
	.cis_get_digital_gain = sensor_imx374_cis_get_digital_gain,
	.cis_get_min_digital_gain = sensor_imx374_cis_get_min_digital_gain,
	.cis_get_max_digital_gain = sensor_imx374_cis_get_max_digital_gain,
	.cis_compensate_gain_for_extremely_br = sensor_cis_compensate_gain_for_extremely_br,
	.cis_wait_streamoff = sensor_cis_wait_streamoff,
	.cis_wait_streamon = sensor_cis_wait_streamon,
	.cis_data_calculation = sensor_imx374_cis_data_calc,
	.cis_check_rev_on_init = sensor_imx374_cis_check_rev_on_init,
	.cis_set_initial_exposure = sensor_cis_set_initial_exposure,
	.cis_set_test_pattern = sensor_imx374_cis_set_test_pattern,
};

static int cis_imx374_probe_i2c(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret;
	u32 mclk_freq_khz;
	struct is_cis *cis;
	struct is_device_sensor_peri *sensor_peri;
	int i;
	char const *setfile;
	struct device_node *dnode = client->dev.of_node;
	int index;
	const int *verify_sensor_mode = NULL;
	int verify_sensor_mode_size = 0;

	ret = sensor_cis_probe(client, &(client->dev), &sensor_peri, I2C_TYPE);
	if (ret) {
		probe_info("%s: sensor_cis_probe ret(%d)\n", __func__, ret);
		return ret;
	}

	cis = &sensor_peri->cis;
	cis->ctrl_delay = N_PLUS_TWO_FRAME;
	cis->cis_ops = &cis_ops;
	/* belows are depend on sensor cis. MUST check sensor spec */
	cis->bayer_order = OTF_INPUT_ORDER_BAYER_GR_BG;

	if (of_property_read_string(dnode, "setfile", &setfile)) {
		err("setfile index read fail(%d), take default setfile!!", ret);
		setfile = "default";
	}

	mclk_freq_khz = sensor_peri->module->pdata->mclk_freq_khz;

	if (mclk_freq_khz == 19200) {
		if (strcmp(setfile, "default") == 0 || strcmp(setfile, "setA") == 0)
			probe_info("%s setfile_A mclk: 19.2MHz\n", __func__);
		else
			err("%s setfile index out of bound, take default (setfile_A mclk: 19.2MHz)", __func__);

		sensor_imx374_global = sensor_imx374_setfile_A_19p2_Global;
		sensor_imx374_global_size = ARRAY_SIZE(sensor_imx374_setfile_A_19p2_Global);
		sensor_imx374_setfiles = sensor_imx374_setfiles_A_19p2;
		sensor_imx374_setfile_sizes = sensor_imx374_setfile_A_19p2_sizes;
		sensor_imx374_pllinfos = sensor_imx374_pllinfos_A_19p2;
		sensor_imx374_max_setfile_num = ARRAY_SIZE(sensor_imx374_setfiles_A_19p2);
		verify_sensor_mode = sensor_imx374_setfile_A_19p2_verify_sensor_mode;
		verify_sensor_mode_size = ARRAY_SIZE(sensor_imx374_setfile_A_19p2_verify_sensor_mode);
	}

	if (cis->vendor_use_adaptive_mipi) {
		for (i = 0; i < verify_sensor_mode_size; i++) {
			index = verify_sensor_mode[i];

			if (index >= cis->mipi_sensor_mode_size || index < 0) {
				panic("wrong mipi_sensor_mode index");
				break;
			}
		}
	}

	probe_info("%s done\n", __func__);

	return ret;
}

static const struct of_device_id sensor_cis_imx374_match[] = {
	{
		.compatible = "samsung,exynos-is-cis-imx374",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_cis_imx374_match);

static const struct i2c_device_id sensor_cis_imx374_idt[] = {
	{ SENSOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_cis_imx374_driver = {
	.probe	= cis_imx374_probe_i2c,
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sensor_cis_imx374_match,
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_cis_imx374_idt
};

#ifdef MODULE
builtin_i2c_driver(sensor_cis_imx374_driver);
#else
static int __init sensor_cis_imx374_init(void)
{
	int ret;

	ret = i2c_add_driver(&sensor_cis_imx374_driver);
	if (ret)
		err("failed to add %s driver: %d",
			sensor_cis_imx374_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_cis_imx374_init);
#endif

MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: fimc-is");
