/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2023 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_CIS_IMX616_H
#define IS_CIS_IMX616_H

#include "is-cis.h"
#define DEBUG	1 /* Don't forget to turn me off later */

/* INTEGRATION TIME Value:
 * INE_INTEG_TIME is a fixed value (0x0200: 16bit - read value is 0x134c)
 */
#define SENSOR_IMX616_FINE_INTEG_TIME_VALUE			(4940)

/* For short name
 * : difficult to comply with kernel coding rule because of too long name
 */
#define REG(name)	SENSOR_IMX616_##name##_ADDR
#define VAL(name)	SENSOR_IMX616_##name##_VALUE

/****
 **  Register Address
 **  : address name format: SENSOR_IMX616_XX...XX_ADDR
 ****/
#define IMX616_ABS_GAIN_GR_SET_ADDR			(0x0B8E)
#define SENSOR_IMX616_ABS_GAIN_R_SET_ADDR		(0x0B90)
#define SENSOR_IMX616_ABS_GAIN_B_SET_ADDR		(0x0B92)
#define SENSOR_IMX616_ABS_GAIN_GB_SET_ADDR		(0x0B94)

/**
 * Extra register for 3hdr
 **/
#define SENSOR_IMX616_MID_COARSE_INTEG_TIME_ADDR	(0x3FE0)
#define SENSOR_IMX616_ST_COARSE_INTEG_TIME_ADDR		(0x0224)
#define SENSOR_IMX616_MID_AGAIN_ADDR			(0x3FE2)
#define SENSOR_IMX616_ST_AGAIN_ADDR			(0x0216)
#define SENSOR_IMX616_MID_DGAIN_ADDR			(0x3FE4)
#define SENSOR_IMX616_ST_DGAIN_ADDR			(0x0218)

#define SENSOR_IMX616_SHD_CORR_EN_ADDR			(0x0B00)
#define SENSOR_IMX616_LSC_APP_RATE_ADDR			(0x380C)
#define SENSOR_IMX616_LSC_APP_RATE_Y_ADDR		(0x380D)
#define SENSOR_IMX616_CALC_MODE_ADDR			(0x3804)
#define SENSOR_IMX616_TABLE_SEL_ADDR			(0x3805)
#define SENSOR_IMX616_KNOT_TAB_GR_ADDR			(0x9C04)
#define SENSOR_IMX616_KNOT_TAB_GB_ADDR			(0x9D08)

#define SENSOR_IMX616_AEHIST1_ADDR			(0x37E0)
#define SENSOR_IMX616_AEHIST2_ADDR			(0x3DA0)
#define SENSOR_IMX616_FLK_AREA_ADDR			(0x37F0)

#define SENSOR_IMX616_EBD_CONTROL_ADDR		(0xBCF1)

/* Blend for TC */
#define SENSOR_IMX616_BLD1_GMTC2_EN_ADDR		(0x3DB6)
#define SENSOR_IMX616_BLD1_GMTC2_RATIO_ADDR		(0x3DB0)
#define SENSOR_IMX616_BLD1_GMTC_NR_TRANSIT_FRM_ADDR	(0xF602)
#define SENSOR_IMX616_BLD2_TC_RATIO_1_ADDR		(0xF4B6)
#define SENSOR_IMX616_BLD2_TC_RATIO_2_ADDR		(0xF4B9)
#define SENSOR_IMX616_BLD2_TC_RATIO_3_ADDR		(0xF4BC)
#define SENSOR_IMX616_BLD2_TC_RATIO_4_ADDR		(0xF4BF)
#define SENSOR_IMX616_BLD2_TC_RATIO_5_ADDR		(0xF4C2)
#define SENSOR_IMX616_BLD3_LTC_RATIO_1_ADDR		(0x3C0C)
#define SENSOR_IMX616_BLD3_LTC_RATIO_6_ADDR		(0x3DAE) /* RATIO 6 */
#define SENSOR_IMX616_BLD3_LTC_RATIO_START_ADDR		SENSOR_IMX616_BLD3_LTC_RATIO_1_ADDR /* RATIO 1 ~ 5 */
#define SENSOR_IMX616_BLD4_HDR_TC_RATIO1_UP_ADDR	(0x3C15)
#define SENSOR_IMX616_BLD4_HDR_TC_RATIO1_LO_ADDR	(0x3C16)
#define SENSOR_IMX616_BLD4_HDR_TC_RATIO_START_ADDR	SENSOR_IMX616_BLD4_HDR_TC_RATIO1_UP_ADDR

#define SENSOR_IMX616_EVC_PGAIN_ADDR			(0x3DB2)
#define SENSOR_IMX616_EVC_NGAIN_ADDR			(0x3DB3)

/* AEHIST Thresh */
#define SENSOR_IMX616_AEHIST_LN_AUTO_THRETH_ADDR	(0x3C00)
#define SENSOR_IMX616_AEHIST_LN_LOWER_TH_MSB_ADDR	(0x3C01)
#define SENSOR_IMX616_AEHIST_LN_THRETH_START_ADDR	SENSOR_IMX616_AEHIST_LN_LOWER_TH_MSB_ADDR
#define SENSOR_IMX616_AEHIST_LOG_AUTO_THRETH_ADDR	(0x3C05)
#define SENSOR_IMX616_AEHIST_LOG_LOWER_TH_MSB_ADDR	(0x3C06)
#define SENSOR_IMX616_AEHIST_LOG_THRETH_START_ADDR	SENSOR_IMX616_AEHIST_LOG_LOWER_TH_MSB_ADDR


/****
 **  Constant Value
 **  : value name format: SENSOR_IMX616_XX...XX_VALUE
 ***/
/**
 * Extra Value for 3hdr
 **/
#define SENSOR_IMX616_QBCHDR_MIN_AGAIN_VALUE		(112)
#define SENSOR_IMX616_QBCHDR_MAX_AGAIN_VALUE		(960)
#define SENSOR_IMX616_QBCHDR_MIN_DGAIN_VALUE		(0x0100)
#define SENSOR_IMX616_QBCHDR_MAX_DGAIN_VALUE		(0x0FFF)
#define SENSOR_IMX616_LSC_MAX_GAIN_VALUE		(0x03FF)
#define SENSOR_IMX616_LSC_APP_RATE_VALUE		(0x00)
#define SENSOR_IMX616_LSC_APP_RATE_Y_VALUE		(0x80)
#define SENSOR_IMX616_CALC_MODE_MANUAL_VALUE		(0x02)
#define SENSOR_IMX616_TABLE_SEL_1_VALUE			(0x00)

/* AEHIST Threth: ((lower_16bit << 16) | upper_16bit) */
#define SENSOR_IMX616_AEHIST_LINEAR_THRETH_VALUE	(0x000003FF)
#define SENSOR_IMX616_AEHIST_LOG_THRETH_VALUE		(0x0000FFFF)
#define SENSOR_IMX616_AEHIST_LINEAR_LO_THRETH_VALUE	(0x0000)
#define SENSOR_IMX616_AEHIST_LINEAR_UP_THRETH_VALUE	(0x03FF)
#define SENSOR_IMX616_AEHIST_LOG_LO_THRETH_VALUE	(0x0000)
#define SENSOR_IMX616_AEHIST_LOG_UP_THRETH_VALUE	(0xFFFF)


/****
 **  Others
 ***/

/* Related Function Option */
#define CIS_CALIBRATION 1
#define SENSOR_IMX616_EBD_LINE_DISABLE		(1)

#if IS_ENABLED(CIS_CALIBRATION)
#define IMX616_QSC_ADDR		(0xC500)
#endif

/* 3HDR */
#define NR_FLKER_BLK_W		(4)
#define NR_FLKER_BLK_H		(96)
#define RAMTABLE_H		13
#define RAMTABLE_V		10
#define RAMTABLE_LEN		(RAMTABLE_H * RAMTABLE_V)

#define NR_ROI_AREAS		(2)

/*
 * [Mode Information]
 *
 * Reference File : IMX616-AAJH5_SAM-DPHY_26MHz_RegisterSetting_ver14.00-20.01_b1_191023.xlsx
 *
 * -. Global Setting -
 *
 * -. 2x2 BIN For Single Still Preview / Capture / Video -
 *    [ 0 ] REG_H : 2x2 Binning 3264x2448 30fps    : Single Still Preview (4:3)     ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2054
 *    [ 1 ] REG_I : 2x2 Binning 3264x1836 30fps    : Single Still Preview (16:9)    ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2054
 *    [ 2 ] REG_T : 2x2 Binning 3264x1836 60fps    : Single Video (16:9)            ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2054
 *    [ 3 ] REG_I_2 : 2x2 Binning 3264x1836 120fps : Single Video (16:9)            ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2054
 *    [ 4 ] REG_J : 2x2 Binning 3264x1504 30fps    : Single Still Preview (19.5:9)  ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2054
 *    [ 5 ] REG_Q : 2X2 Binning 3264x1472 30fps    : Single Still Preview (20:9)    ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2054
 *    [ 6 ] REG_N : 2x2 Binning 2448x2448 30fps    : Single Still Preview (1:1)     ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2054
 *    [ 7 ] REG_Y : 2x2 Binning 2144x1200 120fps   : Single Video (16:9)            ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2054
 *
 * -. 68 deg 2x2 BIN For Single Still Preview / Capture -
 *    [ 8 ] REG_K : 2x2 Binning 2640x1980 30fps    : Single Still Preview (4:3)     ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2054
 *    [ 9 ] REG_L : 2x2 Binning 2640x1488 30fps    : Single Still Preview (16:9)    ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2054
 *    [ 10 ] REG_M : 2x2 Binning 2640x1216 30fps   : Single Still Preview (19.5:9)  ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2054
 *    [ 11 ] REG_O_2 : 2x2 Binning 1968x1968 30fps : Single Still Preview (1:1)     ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2054
 *
 * -. 2x2 BIN H2V2 For FastAE
 *    [ 12 ] REG_R_2 : 2x2 Binning 1632x1224 120fps   : FAST AE (4:3)               ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2054
 *
 * -. FULL Remosaic For Single Still Remosaic Capture -
 *    [ 13 ] REG_F  : Full Remosaic 6528x4896 24fps   : Single Still Remosaic Capture (4:3) , MIPI lane: 4, MIPI data rate(Mbps/lane): 2218
 *
 * -. 68 deg FULL Remosaic For Single Still Remosaic Capture -
 *    [ 14 ] REG_G : Full Remosaic 5264x3948 24fps    : Single Still Remosaic Capture (4:3) , MIPI lane: 4, MIPI data rate(Mbps/lane): 2218
 *
 * -. QBC HDR
 *    [ 15 ] REG_S : QBCHDR Seamless 3264x2448 24fps  : QBCHDR seamless (4:3)        ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2218
 *    [ 16 ] REG_W : QBCHDR Seamless 3264x1836 30fps  : QBCHDR seamless (16:9)       ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2218
 *    [ 17 ] REG_X : 2x2Bin Seamless 3264x1836 30fps  : 2X2Bin seamless (4:3)        ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2218
 *    [ 18 ] REG_Z : 2x2Bin Seamless 3264x2448 24fps  : 2X2Bin seamless (16:9)       ,  MIPI lane: 4, MIPI data rate(Mbps/lane): 2218
 *
 */


enum sensor_imx616_mode_enum {
	SENSOR_IMX616_MODE_2X2BIN_3264x2448_30FPS = 0,		/* 0 */
	SENSOR_IMX616_MODE_2X2BIN_3264x1836_30FPS,
	SENSOR_IMX616_MODE_2X2BIN_2640x1980_30FPS,
	SENSOR_IMX616_MODE_2X2BIN_2640x1448_30FPS,
	SENSOR_IMX616_MODE_2X2BIN_3264x1836_60FPS,
	SENSOR_IMX616_MODE_2X2BIN_3264x1836_120FPS,
	SENSOR_IMX616_MODE_H2V2_1632x1224_120FPS,
	/* FULL Remosaic */
	IMX616_MODE_REMOSAIC_START,
	SENSOR_IMX616_MODE_REMOSAIC_6528x4896_24FPS = IMX616_MODE_REMOSAIC_START,
	SENSOR_IMX616_MODE_REMOSAIC_6528x3672_24FPS,
	SENSOR_IMX616_MODE_REMOSAIC_6528x3012_24FPS,
	SENSOR_IMX616_MODE_REMOSAIC_5264x3948_24FPS,
	IMX616_MODE_REMOSAIC_END = SENSOR_IMX616_MODE_REMOSAIC_5264x3948_24FPS,
	SENSOR_IMX616_MODE_2X2BIN_2880x1980_30FPS,
	/* ++ QBC 3HDR
	 * : consist of QBCHDR mode and QBCHDR-TRANSITION-CAPABLE mode
	 */
	IMX616_MODE_QBCHDR_START ,		/* QBCHDR mode start*/
	SENSOR_IMX616_MODE_QBCHDR_SEAMLESS_3264x2448_24FPS = IMX616_MODE_QBCHDR_START,
	SENSOR_IMX616_MODE_QBCHDR_SEAMLESS_3264x1836_30FPS,
	IMX616_MODE_QBCHDR_CAPABLE_START,	/* QBCHDR transition-capable mode start*/
	SENSOR_IMX616_MODE_2X2BIN_SEAMLESS_3264x1836_30FPS = IMX616_MODE_QBCHDR_CAPABLE_START,
	SENSOR_IMX616_MODE_2X2BIN_SEAMLESS_3264x2448_24FPS,
	IMX616_MODE_QBCHDR_CAPABLE_END = SENSOR_IMX616_MODE_2X2BIN_SEAMLESS_3264x2448_24FPS,
	IMX616_MODE_QBCHDR_END = IMX616_MODE_QBCHDR_CAPABLE_END,
	/* -- QBC 3HDR */

	/* ++ Seamless Transition Mode, only used internally in CIS driver */
	SENSOR_IMX616_MODE_TRANSITION_2X2BIN_to_QBCHDR,
	SENSOR_IMX616_MODE_TRANSITION_QBCHDR_to_2X2BIN,
	/* -- Seamless Transition Mode */
	SENSOR_IMX616_MODE_MAX
};

struct sensor_imx616_private_data {
	const struct sensor_regs global;
};

static const struct sensor_reg_addr sensor_imx616_reg_addr = {
	.fll = 0x0340,
	.cit = 0x0202,
	.cit_shifter = 0x3100,
	.again = 0x0204,
	.dgain = 0x020E,
	.group_param_hold = 0x0104,
};

/* IS_REMOSAIC(u32 mode): check if mode is remosaic */
#define IS_REMOSAIC(mode) ({						\
	typecheck(u32, mode) && (mode >= IMX616_MODE_REMOSAIC_START) &&	\
	(mode <= IMX616_MODE_REMOSAIC_END);				\
})

/* IS_REMOSAIC_MODE(struct is_cis *cis) */
#define IS_REMOSAIC_MODE(cis) ({					\
	u32 m;								\
	typecheck(struct is_cis *, cis);				\
	m = cis->cis_data->sens_config_index_cur;			\
	(m >= IMX616_MODE_REMOSAIC_START) && (m <= IMX616_MODE_REMOSAIC_END); \
})

/* IS_3HDR(struct is_cis *cis, u32 mode): check if mode is 3dhdr */
#define IS_3HDR(cis, mode) ({						\
	typecheck(u32, mode);						\
	typecheck(struct is_cis *, cis);				\
	(cis->use_3hdr) && (mode >= IMX616_MODE_QBCHDR_START) &&	\
	(mode <= IMX616_MODE_QBCHDR_END);				\
})

/* IS_3HDR_MODE(struct is_cis *cis) */
#define IS_3HDR_MODE(cis) ({						\
	u32 m;								\
	typecheck(struct is_cis *, cis);				\
	m = cis->cis_data->sens_config_index_cur;			\
	(cis->use_3hdr) && (m >= IMX616_MODE_QBCHDR_START) &&		\
	(m <= IMX616_MODE_QBCHDR_END);					\
})

/**
 * @brief IS_3HDR_TRANSITION_CAPABLE
 *  : check if 'mode' is 3hdr transition-capable mode
 * @return
 *  : true or false
 */
#define IS_3HDR_SEAMLESS(cis, mode) ({			\
	typecheck(u32, mode);						\
	typecheck(struct is_cis *, cis);				\
	(cis->use_3hdr) && (mode >= IMX616_MODE_QBCHDR_CAPABLE_START)	\
	&& (mode <= IMX616_MODE_QBCHDR_CAPABLE_END);			\
})

#define IS_3HDR_SEAMLESS_MODE(cis) ({				\
	u32 m;								\
	typecheck(struct is_cis *, cis);				\
	m = cis->cis_data->sens_config_index_cur;			\
	(cis->use_3hdr) && (m >= IMX616_MODE_QBCHDR_CAPABLE_START) 	\
	&& (m <= IMX616_MODE_QBCHDR_CAPABLE_END);			\
})

#if defined(DEBUG) && (DEBUG >= 2)
#define ASSERT(x, fmt, args...)						\
	do {								\
		if (!(x)) {						\
			err("[ASSERT FAILURE] "fmt, ##args);		\
			panic("fimc-is2:imx616:%d\n" "[ASSERT] " fmt,	\
				__LINE__, ##args);			\
		}							\
	} while (0)
#elif defined(DEBUG) && (DEBUG >= 1)
#define ASSERT(x, fmt, args...)						\
	do {								\
		if (!(x)) {						\
			err("[ASSERT FAILURE] "fmt, ##args);		\
		}							\
	} while (0)
#else
#define ASSERT(x, format...) do { } while (0)
#endif

#define CHECK_GOTO(conditon, label)		\
	do {					\
		if (unlikely(conditon))		\
			goto label;		\
	} while (0)

#define CHECK_RET(conditon, ret)		\
	do {					\
		if (unlikely(conditon))		\
			return ret;		\
	} while (0)

#define CHECK_ERR_GOTO(conditon, label, fmt, args...)	\
	do {						\
		if (unlikely(conditon)) {		\
			err(fmt, ##args);		\
			goto label;			\
		}					\
	} while (0)

#define CHECK_ERR_RET(conditon, ret,  fmt, args...)	\
	do {						\
		if (unlikely(conditon)) {		\
			err(fmt, ##args);		\
			return ret;			\
		}					\
	} while (0)


#endif

