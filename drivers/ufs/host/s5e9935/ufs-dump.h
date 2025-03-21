// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2013-2016, Linux Foundation. All rights reserved.
 */

#ifndef _UFS_DUMP_H_
#define _UFS_DUMP_H_

enum {
	LOG_STD_HCI_SFR = 0xFFFFFFF0,
	LOG_VS_HCI_SFR,
	LOG_FMP_SFR,
	LOG_UNIPRO_SFR,
	LOG_PMA_SFR,
	LOG_MCQ_SFR,
	LOG_SQCQ_SFR,
	LOG_INVALID,
};

enum {
	DBG_ATTR_UNIPRO = 0xEFFFFFF0,
	DBG_ATTR_PCS_CMN,
	DBG_ATTR_PCS_TX,
	DBG_ATTR_PCS_RX,
	DBG_ATTR_DIRECT_PCS_TX,
	DBG_ATTR_DIRECT_PCS_RX,
	DBG_ATTR_PCS_START,
	DBG_ATTR_PCS_END,
};

enum {
	DBG_ATTR_CPORT_TX = 0xDFFFFFF0,
	DBG_ATTR_CPORT_RX,
	DBG_ATTR_CPORT_PTR,
	DBG_ATTR_CPORT_UTRL,
	DBG_ATTR_CPORT_UCD,
	DBG_ATTR_CPORT_UTMRL,
	DBG_ATTR_CPORT_REMAP,
	DBG_ATTR_CPORT_END,
};

enum {
	SFR_VAL_H_0_FIRST = 0,
	SFR_VAL_H_0,
	SFR_VAL_NUM,
};

enum {
	ATTR_VAL_H_0_L_0_FIRST = 0,
	ATTR_VAL_H_0_L_1_FIRST,
	ATTR_VAL_H_0_L_0,
	ATTR_VAL_H_0_L_1,
	ATTR_VAL_NUM,
};

struct exynos_ufs_sfr_log {
	const char* name;
	const u32 offset;
	u32 val[SFR_VAL_NUM];
};

struct exynos_ufs_attr_log {
	const u32 mib;
	const u32 offset;
	/* 0: lane0, 1: lane1 */
	u32 val[ATTR_VAL_NUM];
};

struct exynos_ufs_cport_log {
	const u32 mib;
	const char *name;
	const u32 offset;
	const u32 size;
};

static struct exynos_ufs_cport_log ufs_cport_sfr[] = {
	{DBG_ATTR_CPORT_TX,	"TX BUF",	0x0,	0x100},
	{DBG_ATTR_CPORT_RX,	"RX BUF",	0x100,	0x100},
	{DBG_ATTR_CPORT_PTR,	"CPORT LOG PTR",	0x200,	0x100},
	{DBG_ATTR_CPORT_UTRL,	"UTRL",	0x300,	0x400},
	{DBG_ATTR_CPORT_UCD,	"UCD",	0x700, 0x400},
	{DBG_ATTR_CPORT_UTMRL,	"UTMRL",	0xb00, 0xc0},
	{DBG_ATTR_CPORT_END,	0,	0},
};

static struct exynos_ufs_sfr_log ufs_log_sfr[] = {
	{"STD HCI SFR",	LOG_STD_HCI_SFR,	{0}},

	{"AHIT",	0x18,	{0}},
	{"INTERRUPT STATUS",	0x20,	{0}},
	{"INTERRUPT ENABLE",	0x24,	{0}},
	{"CONTROLLER STATUS",	0x30,	{0}},
	{"CONTROLLER ENABLE",	0x34,	{0}},
	{"UTP TRANSF REQ INT AGG CNTRL",	0x4C,	{0}},
	{"UTP TRANSF REQ LIST BASE L",	0x50,	{0}},
	{"UTP TRANSF REQ LIST BASE H",	0x54,	{0}},
	{"UTP TRANSF REQ DOOR BELL",	0x58,	{0}},
	{"UTP TRANSF REQ LIST CLEAR",	0x5C,	{0}},
	{"UTP TRANSF REQ LIST RUN STOP",	0x60,	{0}},
	{"UTP TRANSF REQ LIST CNR",	0x64,	{0}},
	{"UTP TASK REQ LIST BASE L",	0x70,	{0}},
	{"UTP TASK REQ LIST BASE H",	0x74,	{0}},
	{"UTP TASK REQ DOOR BELL",	0x78,	{0}},
	{"UTP TASK REQ LIST CLEAR",	0x7C,	{0}},
	{"UTP TASK REQ LIST RUN STOP",	0x80,	{0}},
	{"UIC COMMAND",	0x90,	{0}},
	{"UIC COMMAND ARG1",	0x94,	{0}},
	{"UIC COMMAND ARG2",	0x98,	{0}},
	{"UIC COMMAND ARG3",	0x9C,	{0}},
	{"MH_IS",	0xd0,	{0}},
	{"MH_IE",	0xd4,	{0}},
	{"DBR_DUPLICATION_INFO",	0xe8,	{0}},
	{"SMU_CDB_INVALID_INFO",	0xec,	{0}},
	{"SMU_UNMAP_INVALID_INFO",	0xf4,	{0}},
	{"CCAP",	0x100,	{0}},
	{"CFG",		0x180,	{0}},
	{"MCQCFG",	0x300,	{0}},

	{"VS HCI SFR",	LOG_VS_HCI_SFR,	{0}},

	{"TXPRDT ENTRY SIZE",	0x00,	{0}},
	{"RXPRDT ENTRY SIZE",	0x04,	{0}},
	{"TO CNT DIV VAL",	0x08,	{0}},
	{"1US TO CNT VAL",	0x0C,	{0}},
	{"INVALID UPIU CTRL",	0x10,	{0}},
	{"INVALID UPIU BADDR",	0x14,	{0}},
	{"INVALID UPIU UBADDR",	0x18,	{0}},
	{"INVALID UTMR OFFSET ADDR",	0x1C,	{0}},
	{"INVALID UTR OFFSET ADDR",	0x20,	{0}},
	{"INVALID DIN OFFSET ADDR",	0x24,	{0}},
	{"VENDOR SPECIFIC IS",	0x38,	{0}},
	{"VENDOR SPECIFIC IE",	0x3C,	{0}},
	{"UTRL NEXUS TYPE",	0x40,	{0}},
	{"UTMRL NEXUS TYPE",	0x44,	{0}},
	{"SW RST",	0x50,	{0}},
	{"RX UPIU MATCH ERROR CODE",	0x5C,	{0}},
	{"DATA REORDER",	0x60,	{0}},
	{"AXIDMA RWDATA BURST LEN",	0x6C,	{0}},
	{"WRITE DMA CTRL",	0x74,	{0}},
	{"V2P1 CTRL",	0x8C,	{0}},
	{"CLKSTOP CTRL",	0xB0,	{0}},
	{"FORCE HCS",	0xB4,	{0}},
	{"FSM MONITOR",	0xC0,	{0}},
	{"DMA0 MONITOR STATE",	0xC8,	{0}},
	{"DMA0 MONITOR CNT",	0xCC,	{0}},
	{"DMA1 MONITOR STATE",	0xD0,	{0}},
	{"DMA1 MONITOR CNT",	0xD4,	{0}},
	{"DMA0 DOORBELL DEBUG",	0xD8,	{0}},
	{"DMA1 DOORBELL DEBUG",	0xDC,	{0}},

	{"AXI DMA IF CTRL",	0xF8,	{0}},
	{"UFS ACG DISABLE",	0xFC,	{0}},
	{"MPHY REFCLK SEL",	0x108,	{0}},

	{"SMU RD ABORT MATCH INFO",	0x118,	{0}},
	{"SMU WR ABORT MATCH INFO",	0x11C,	{0}},

	{"INVALID PRDT CTRL",	0x130,	{0}},
	{"UFS_PH_AH8_H8T",	0x504,	{0}},
	{"UFS_PH_AH8_CLKGATE_CONFIG",	0x508,	{0}},
	{"UFS_PH_AH8_STATE",	0x50c,	{0}},
	{"UFS_PH_AH8_ERR_CONTROL",	0x514,	{0}},
	{"UFS_PH_AH8_INT_CONTROL",	0x518,	{0}},
	{"UFS_PH_AH8_PROC_MPHY",	0x51c,	{0}},
	{"UFS_PH_AH8_PROC_CONFIG",	0x520,	{0}},
	{"UFS_PH_AH8_TIMEOUT",	0x524,	{0}},

	{"FMP SFR",	LOG_FMP_SFR,	{0}},

	{"UFSPRCTRL",	0x0000,	{0}},
	{"UFSPRSTAT0",	0x0008,	{0}},
	{"UFSPRSTAT1",	0x000C,	{0}},
	{"UFSPRSECURITY0",	0x0010,	{0}},
	{"UFSPRSECURITY1",	0x0014,	{0}},
	{"UFSPRSECURITY2",	0x0018,	{0}},
	{"UFSPRVERSION",	0x001C,	{0}},
	{"UFSPWCTRL",	0x0100,	{0}},
	{"UFSPWSTAT0",	0x0108,	{0}},
	{"UFSPWSTAT1",	0x010C,	{0}},
	{"UFSPSBEGIN0",	0x2000,	{0}},
	{"UFSPSEND0",	0x2004,	{0}},
	{"UFSPSLUN0",	0x2008,	{0}},
	{"UFSPSCTRL0",	0x200C,	{0}},
	{"UFSPSBEGIN1",	0x2010,	{0}},
	{"UFSPSEND1",	0x2014,	{0}},
	{"UFSPSLUN1",	0x2018,	{0}},
	{"UFSPSCTRL1",	0x201C,	{0}},
	{"UFSPSBEGIN2",	0x2020,	{0}},
	{"UFSPSEND2",	0x2024,	{0}},
	{"UFSPSLUN2",	0x2028,	{0}},
	{"UFSPSCTRL2",	0x202C,	{0}},
	{"UFSPSBEGIN3",	0x2030,	{0}},
	{"UFSPSEND3",	0x2034,	{0}},
	{"UFSPSLUN3",	0x2038,	{0}},
	{"UFSPSCTRL3",	0x203C,	{0}},
	{"UFSPSBEGIN4",	0x2040,	{0}},
	{"UFSPSEND4",	0x2044,	{0}},
	{"UFSPSLUN4",	0x2048,	{0}},
	{"UFSPSCTRL4",	0x204C,	{0}},
	{"UFSPSBEGIN5",	0x2050,	{0}},
	{"UFSPSEND5",	0x2054,	{0}},
	{"UFSPSLUN5",	0x2058,	{0}},
	{"UFSPSCTRL5",	0x205C,	{0}},
	{"UFSPSBEGIN6",	0x2060,	{0}},
	{"UFSPSEND6",	0x2064,	{0}},
	{"UFSPSLUN6",	0x2068,	{0}},
	{"UFSPSCTRL6",	0x206C,	{0}},
	{"UFSPSBEGIN7",	0x2070,	{0}},
	{"UFSPSEND7",	0x2074,	{0}},
	{"UFSPSLUN7",	0x2078,	{0}},
	{"UFSPSCTRL7",	0x207C,	{0}},

	{"UNIPRO SFR",	LOG_UNIPRO_SFR,	{0}},

	{"DME_LINKSTARTUP_CNF_RESULT",		(0x7854),	{0}},
	{"DME_HIBERNATE_ENTER_CNF_RESULT",		(0x7864),	{0}},
	{"DME_HIBERNATE_ENTER_IND_RESULT",		(0x7868),	{0}},
	{"DME_HIBERNATE_EXIT_CNF_RESULT",		(0x7874),	{0}},
	{"DME_HIBERNATE_EXIT_IND_RESULT",		(0x7878),	{0}},
	{"DME_POWERMODE_IND_RESULT",		(0x78EC),	{0}},
	{"DME_INTR_STATUS_LSB",		(0x7B00),	{0}},
	{"DME_INTR_STATUS_MSB",		(0x7B04),	{0}},
	{"DME_INTR_ERROR_CODE",		(0x7B20),	{0}},
	{"DME_DBG_OPTION_SUITE",		(0x7C00),	{0}},
	{"DME_DBG_CTRL_FSM",		(0x7D00),	{0}},
	{"DME_DBG_FLAG_STATUS",		(0x7D14),	{0}},
	{"DME_DBG_LINKCFG_FSM",		(0x7D18),	{0}},
	{"DME_DBG_ERROR_HISTORY_COUNTER",		(0x7E10),	{0}},
	{"DME_DBG_ERROR_HISTORY_POINTER",		(0x7E14),	{0}},
	{"DME_DBG_ERROR_HISTORY_0",		(0x7E20),	{0}},
	{"DME_DBG_ERROR_HISTORY_1",		(0x7E24),	{0}},
	{"DME_DBG_ERROR_HISTORY_2",		(0x7E28),	{0}},
	{"DME_DBG_ERROR_HISTORY_3",		(0x7E2C),	{0}},
	{"DME_DBG_ERROR_HISTORY_4",		(0x7E30),	{0}},
	{"DME_DBG_ERROR_HISTORY_5",		(0x7E34),	{0}},
	{"DME_DBG_ERROR_HISTORY_6",		(0x7E38),	{0}},
	{"DME_DBG_ERROR_HISTORY_7",		(0x7E3C),	{0}},

	{"PMA SFR",	LOG_PMA_SFR,	{0}},

	{"CMN_REG380",	(0X0E00),	{0}},
	{"CMN_REG381",	(0X0E04),	{0}},
	{"CMN_REG382",	(0X0E08),	{0}},
	{"CMN_REG383",	(0X0E0C),	{0}},
	{"CMN_REG384",	(0X0E10),	{0}},
	{"CMN_REG385",	(0X0E14),	{0}},
	{"TRSV_REG39E",	(0X1E78),	{0}},
	{"TRSV_REG39F",	(0X1E7C),	{0}},
	{"TRSV_REG3A0",	(0X1E80),	{0}},
	{"TRSV_REG3A1",	(0X1E84),	{0}},
	{"TRSV_REG3A2",	(0X1E88),	{0}},
	{"TRSV_REG3A3",	(0X1E8C),	{0}},
	{"TRSV_REG3A4",	(0X1E90),	{0}},
	{"TRSV_REG3A5",	(0X1E94),	{0}},
	{"TRSV_REG3A6",	(0X1E98),	{0}},
	{"TRSV_REG3A7",	(0X1E9C),	{0}},
	{"TRSV_REG3A8",	(0X1EA0),	{0}},
	{"TRSV_REG3A9",	(0X1EA4),	{0}},
	{"TRSV_REG3AA",	(0X1EA8),	{0}},
	{"TRSV_REG3AB",	(0X1EAC),	{0}},
	{"TRSV_REG3AC",	(0X1EB0),	{0}},
	{"TRSV_REG3AD",	(0X1EB4),	{0}},
	{"TRSV_REG3AE",	(0X1EB8),	{0}},
	{"TRSV_REG3AF",	(0X1EBC),	{0}},
	{"TRSV_REG3B0",	(0X1EC0),	{0}},
	{"TRSV_REG3B1",	(0X1EC4),	{0}},
	{"TRSV_REG3D7",	(0X1F5C),	{0}},
	{"TRSV_REG3D8",	(0X1F60),	{0}},
	{"TRSV_REG3D9",	(0X1F64),	{0}},
	{"TRSV_REG3DA",	(0X1F68),	{0}},
	{"TRSV_REG79E",	(0X2E78),	{0}},
	{"TRSV_REG79F",	(0X2E7C),	{0}},
	{"TRSV_REG7A0",	(0X2E80),	{0}},
	{"TRSV_REG7A1",	(0X2E84),	{0}},
	{"TRSV_REG7A2",	(0X2E88),	{0}},
	{"TRSV_REG7A3",	(0X2E8C),	{0}},
	{"TRSV_REG7A4",	(0X2E90),	{0}},
	{"TRSV_REG7A5",	(0X2E94),	{0}},
	{"TRSV_REG7A6",	(0X2E98),	{0}},
	{"TRSV_REG7A7",	(0X2E9C),	{0}},
	{"TRSV_REG7A8",	(0X2EA0),	{0}},
	{"TRSV_REG7A9",	(0X2EA4),	{0}},
	{"TRSV_REG7AA",	(0X2EA8),	{0}},
	{"TRSV_REG7AB",	(0X2EAC),	{0}},
	{"TRSV_REG7AC",	(0X2EB0),	{0}},
	{"TRSV_REG7AD",	(0X2EB4),	{0}},
	{"TRSV_REG7AE",	(0X2EB8),	{0}},
	{"TRSV_REG7AF",	(0X2EBC),	{0}},
	{"TRSV_REG7B0",	(0X2EC0),	{0}},
	{"TRSV_REG7B1",	(0X2EC4),	{0}},
	{"TRSV_REG7D7",	(0X2F5C),	{0}},
	{"TRSV_REG7D8",	(0X2F60),	{0}},
	{"TRSV_REG7D9",	(0X2F64),	{0}},
	{"TRSV_REG7DA",	(0X2F68),	{0}},


	{"MCQ SFR",	LOG_MCQ_SFR,	{0}},

	{"SQATTR0",	(0X0000),	{0}},
	{"SQISAO0",	(0X0010),	{0}},
	{"CQATTR0",	(0X0020),	{0}},
	{"CQISAO0",	(0X0030),	{0}},
	{"SQATTR1",	(0X0040),	{0}},
	{"SQISAO1",	(0X0050),	{0}},
	{"CQATTR1",	(0X0060),	{0}},
	{"CQISAO1",	(0X0070),	{0}},
	{"SQATTR2",	(0X0080),	{0}},
	{"SQISAO2",	(0X0090),	{0}},
	{"CQATTR2",	(0X00A0),	{0}},
	{"CQISAO2",	(0X00B0),	{0}},
	{"SQATTR3",	(0X00C0),	{0}},
	{"SQISAO3",	(0X00D0),	{0}},
	{"CQATTR3",	(0X00E0),	{0}},
	{"CQISAO3",	(0X00F0),	{0}},
	{"SQATTR4",	(0X0100),	{0}},
	{"SQISAO4",	(0X0110),	{0}},
	{"CQATTR4",	(0X0120),	{0}},
	{"CQISAO4",	(0X0130),	{0}},
	{"SQATTR5",	(0X0140),	{0}},
	{"SQISAO5",	(0X0150),	{0}},
	{"CQATTR5",	(0X0160),	{0}},
	{"CQISAO5",	(0X0170),	{0}},
	{"SQATTR6",	(0X0180),	{0}},
	{"SQISAO6",	(0X0190),	{0}},
	{"CQATTR6",	(0X01A0),	{0}},
	{"CQISAO6",	(0X01B0),	{0}},
	{"SQATTR7",	(0X01C0),	{0}},
	{"SQISAO7",	(0X01D0),	{0}},
	{"CQATTR7",	(0X01E0),	{0}},
	{"CQISAO7",	(0X01F0),	{0}},

	{"MCQ SQ/CQ SFR",	LOG_SQCQ_SFR,	{0}},
	{"SQHP0",	(0X0000),	{0}},
	{"SQTP0",	(0X0004),	{0}},
	{"SQIS0",	(0X0010),	{0}},
	{"SQIE0",	(0X0014),	{0}},
	{"CQHP0",	(0X0020),	{0}},
	{"CQTP0",	(0X0024),	{0}},
	{"CQIS0",	(0X0030),	{0}},
	{"CQIE0",	(0X0034),	{0}},
	{"SQHP1",	(0X1000),	{0}},
	{"SQTP1",	(0X1004),	{0}},
	{"SQIS1",	(0X1010),	{0}},
	{"SQIE1",	(0X1014),	{0}},
	{"CQHP1",	(0X1020),	{0}},
	{"CQTP1",	(0X1024),	{0}},
	{"CQIS1",	(0X1030),	{0}},
	{"CQIE1",	(0X1034),	{0}},
	{"SQHP2",	(0X2000),	{0}},
	{"SQTP2",	(0X2004),	{0}},
	{"SQIS2",	(0X2010),	{0}},
	{"SQIE2",	(0X2014),	{0}},
	{"CQHP2",	(0X2020),	{0}},
	{"CQTP2",	(0X2024),	{0}},
	{"CQIS2",	(0X2030),	{0}},
	{"CQIE2",	(0X2034),	{0}},
	{"SQHP3",	(0X3000),	{0}},
	{"SQTP3",	(0X3004),	{0}},
	{"SQIS3",	(0X3010),	{0}},
	{"SQIE3",	(0X3014),	{0}},
	{"CQHP3",	(0X3020),	{0}},
	{"CQTP3",	(0X3024),	{0}},
	{"CQIS3",	(0X3030),	{0}},
	{"CQIE3",	(0X3034),	{0}},
	{"SQHP4",	(0X4000),	{0}},
	{"SQTP4",	(0X4004),	{0}},
	{"SQIS4",	(0X4010),	{0}},
	{"SQIE4",	(0X4014),	{0}},
	{"CQHP4",	(0X4020),	{0}},
	{"CQTP4",	(0X4024),	{0}},
	{"CQIS4",	(0X4030),	{0}},
	{"CQIE4",	(0X4034),	{0}},
	{"SQHP5",	(0X5000),	{0}},
	{"SQTP5",	(0X5004),	{0}},
	{"SQIS5",	(0X5010),	{0}},
	{"SQIE5",	(0X5014),	{0}},
	{"CQHP5",	(0X5020),	{0}},
	{"CQTP5",	(0X5024),	{0}},
	{"CQIS5",	(0X5030),	{0}},
	{"CQIE5",	(0X5034),	{0}},
	{"SQHP6",	(0X6000),	{0}},
	{"SQTP6",	(0X6004),	{0}},
	{"SQIS6",	(0X6010),	{0}},
	{"SQIE6",	(0X6014),	{0}},
	{"CQHP6",	(0X6020),	{0}},
	{"CQTP6",	(0X6024),	{0}},
	{"CQIS6",	(0X6030),	{0}},
	{"CQIE6",	(0X6034),	{0}},
	{"SQHP7",	(0X7000),	{0}},
	{"SQTP7",	(0X7004),	{0}},
	{"SQIS7",	(0X7010),	{0}},
	{"SQIE7",	(0X7014),	{0}},
	{"CQHP7",	(0X7020),	{0}},
	{"CQTP7",	(0X7024),	{0}},
	{"CQIS7",	(0X7030),	{0}},
	{"CQIE7",	(0X7034),	{0}},

	{0, 0,	{0}},
};

static struct exynos_ufs_sfr_log ufs_log_sfr_evt1[] = {
};

static struct exynos_ufs_attr_log ufs_log_attr[] = {
	{DBG_ATTR_UNIPRO, 0,	{0, 0}},
	/* PA Standard */
	{0x1520,	0x3080,	{0, 0}},
	{0x1540,	0x3100,	{0, 0}},
	{0x1543,	0x310C,	{0, 0}},
	{0x155C,	0x3170,	{0, 0}},
	{0x155D,	0x3174,	{0, 0}},
	{0x155F,	0x317C,	{0, 0}},
	{0x1560,	0x3180,	{0, 0}},
	{0x1561,	0x3184,	{0, 0}},
	{0x1564,	0x3190,	{0, 0}},
	{0x1567,	0x319C,	{0, 0}},
	{0x1568,	0x31A0,	{0, 0}},
	{0x1569,	0x31A4,	{0, 0}},
	{0x156A,	0x31A8,	{0, 0}},
	{0x1571,	0x31C4,	{0, 0}},
	{0x1580,	0x3200,	{0, 0}},
	{0x1581,	0x3204,	{0, 0}},
	{0x1582,	0x3208,	{0, 0}},
	{0x1583,	0x320C,	{0, 0}},
	{0x1584,	0x3210,	{0, 0}},
	{0x1585,	0x3214,	{0, 0}},
	{0x1590,	0x3240,	{0, 0}},
	{0x1591,	0x3244,	{0, 0}},
	{0x15A1,	0x3284,	{0, 0}},
	{0x15A2,	0x3288,	{0, 0}},
	{0x15A3,	0x328C,	{0, 0}},
	{0x15A4,	0x3290,	{0, 0}},
	{0x15A7,	0x329C,	{0, 0}},
	{0x15A8,	0x32A0,	{0, 0}},
	{0x15A9,	0x32A4,	{0, 0}},
	{0x15C0,	0x3300,	{0, 0}},
	{0x15C1,	0x3304,	{0, 0}},
	{0x15D2,	0x3348,	{0, 0}},
	{0x15D3,	0x334C,	{0, 0}},
	{0x15D4,	0x3350,	{0, 0}},
	{0x15D5,	0x3354,	{0, 0}},
	/* PA Debug */
	{0x9500,	0x3800,	{0, 0}},
	{0x9501,	0x3804,	{0, 0}},
	{0x9502,	0x3808,	{0, 0}},
	{0x9504,	0x3810,	{0, 0}},
	{0x9564,	0x3990,	{0, 0}},
	{0x956A,	0x39A8,	{0, 0}},
	{0x956D,	0x39B4,	{0, 0}},
	{0x9570,	0x39C0,	{0, 0}},
	{0x9595,	0x3A54,	{0, 0}},
	{0x9596,	0x3A58,	{0, 0}},
	{0x9597,	0x3A5C,	{0, 0}},
	{0x95C0,	0x3B00,	{0, 0}},
	{0x95C1,	0x3B04,	{0, 0}},
	{0x0044,	0x0044,	{0, 0}},
	/* DL Standard */
	{0x2047,	0x411C, {0, 0}},
	{0x2067,	0x419C, {0, 0}},
	/* DL Debug */
	{0xA000,	0x4800, {0, 0}},
	{0xA005,	0x4814, {0, 0}},
	{0xA007,	0x481C, {0, 0}},
	{0xA010,	0x4840, {0, 0}},
	{0xA015,	0x4854, {0, 0}},
	{0xA020,	0x4880, {0, 0}},
	{0xA021,	0x4884, {0, 0}},
	{0xA022,	0x4888, {0, 0}},
	{0xA023,	0x488C, {0, 0}},
	{0xA024,	0x4890, {0, 0}},
	{0xA025,	0x4894, {0, 0}},
	{0xA026,	0x4898, {0, 0}},
	{0xA027,	0x489C, {0, 0}},
	{0xA100,	0x4C00, {0, 0}},
	{0xA101,	0x4C04, {0, 0}},
	{0xA102,	0x4C08, {0, 0}},
	{0xA103,	0x4C0C, {0, 0}},
	{0xA114,	0x4C50, {0, 0}},
	{0xA115,	0x4C54, {0, 0}},
	{0xA116,	0x4C58, {0, 0}},
	{0xA120,	0x4C80, {0, 0}},
	{0xA121,	0x4C84, {0, 0}},
	{0xA122,	0x4C88, {0, 0}},
	/* NL Standard */
	/* NL Debug */
	{0xB011, 0x5844,	{0, 0}},
	/* TL Standard */
	{0x4020, 0x6080,	{0, 0}},
	/* TL Debug */
	{0xC001, 0x6804,	{0, 0}},
	{0xC024, 0x6890,	{0, 0}},
	{0xC026, 0x6898,	{0, 0}},
	{DBG_ATTR_PCS_START, 0, {0, 0}},
	{0x0001, 0x4184,	{0, 0}},
	{0x0001, 0x5184,	{0, 0}},
	{0x0001, 0x8184,	{0, 0}},
	{0x0001, 0x9184,	{0, 0}},
	{DBG_ATTR_DIRECT_PCS_TX, 0, {0, 0}},
	/* MPHY PCS TX0 */
	{0x0021, 0x4084,	{0, 0}},
	{0x0022, 0x4088,	{0, 0}},
	{0x0023, 0x408C,	{0, 0}},
	{0x002B, 0x40AC,	{0, 0}},
	{0x0033, 0x40CC,	{0, 0}},
	{0x0041, 0x4104,	{0, 0}},
	{0x0148, 0x4520,	{0, 0}},
	/* MPHY PCS TX1 */
	{0x0021, 0x5084,	{0, 0}},
	{0x0022, 0x5088,	{0, 0}},
	{0x0023, 0x508C,	{0, 0}},
	{0x002B, 0x50AC,	{0, 0}},
	{0x0033, 0x50CC,	{0, 0}},
	{0x0041, 0x5104,	{0, 0}},
	{0x0148, 0x5520,	{0, 0}},
	{DBG_ATTR_DIRECT_PCS_RX, 0, {0, 0}},
	/* MPHY PCS RX0 */
	{0x008B, 0x822C,	{0, 0}},
	{0x00A1, 0x8284,	{0, 0}},
	{0x00A2, 0x8288,	{0, 0}},
	{0x00A3, 0x828C,	{0, 0}},
	{0x00A7, 0x829C,	{0, 0}},
	{0x00AA, 0x82A8,	{0, 0}},
	{0x00C1, 0x8304,	{0, 0}},
	{0x0135, 0x84D4,	{0, 0}},
	{0x0136, 0x84D8,	{0, 0}},
	/* MPHY PCS RX1 */
	{0x008B, 0x922C,	{0, 0}},
	{0x00A1, 0x9284,	{0, 0}},
	{0x00A2, 0x9288,	{0, 0}},
	{0x00A3, 0x928C,	{0, 0}},
	{0x00A7, 0x929C,	{0, 0}},
	{0x00AA, 0x92A8,	{0, 0}},
	{0x00C1, 0x9304,	{0, 0}},
	{0x0135, 0x94D4,	{0, 0}},
	{0x0136, 0x94D8,	{0, 0}},
	{DBG_ATTR_PCS_END, 0,	{0, 0}},
	{0x0000, 0x4184,	{0, 0}},
	{0x0000, 0x5184,	{0, 0}},
	{0x0000, 0x8184,	{0, 0}},
	{0x0000, 0x9184,	{0, 0}},
	{0, 0,	{0, 0}},
};

#endif	/* _UFS_DUMP_H_ */
