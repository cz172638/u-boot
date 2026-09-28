/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2018-2020 Rockchip Electronics Co., Ltd
 * Author: Zhihuan He <huan.he@rock-chips.com> (uPCTL/PHY part)
 */

#ifndef _ASM_ARCH_SDRAM_RK3308_H
#define _ASM_ARCH_SDRAM_RK3308_H

#include <ram.h>
#include <asm/arch-rockchip/cru_rk3308.h>
#include <asm/arch-rockchip/grf_rk3308.h>
#include <asm/arch-rockchip/pmu_rk3308.h>
#include <asm/arch-rockchip/sdram_common.h>
#include <linux/types.h>

/* DesignWare uPCTL and Innosilicon DDR PHY */
struct ddr_pctl {
	u32 scfg;
	u32 sctl;
	u32 stat;
	u32 intrstat;
	u32 reserved0[(0x40 - 0x10) / 4];
	u32 mcmd;
	u32 powctl;
	u32 powstat;
	u32 cmdtstat;
	u32 cmdtstaten;
	u32 reserved1[(0x60 - 0x54) / 4];
	u32 mrrcfg0;
	u32 mrrstat0;
	u32 mrrstat1;
	u32 reserved2[(0x7c - 0x6c) / 4];
	u32 mcfg1;
	u32 mcfg;
	u32 ppcfg;
	u32 mstat;
	u32 lpddr2zqcfg;
	u32 reserved3;
	u32 dtupdes;
	u32 dtuna;
	u32 dtune;
	u32 dtuprd0;
	u32 dtuprd1;
	u32 dtuprd2;
	u32 dtuprd3;
	u32 dtuawdt;
	u32 reserved4[(0xc0 - 0xb4) / 4];
	u32 togcnt1u;
	u32 tinit;
	u32 trsth;
	u32 togcnt100n;
	u32 trefi;
	u32 tmrd;
	u32 trfc;
	u32 trp;
	u32 trtw;
	u32 tal;
	u32 tcl;
	u32 tcwl;
	u32 tras;
	u32 trc;
	u32 trcd;
	u32 trrd;
	u32 trtp;
	u32 twr;
	u32 twtr;
	u32 texsr;
	u32 txp;
	u32 txpdll;
	u32 tzqcs;
	u32 tzqcsi;
	u32 tdqs;
	u32 tcksre;
	u32 tcksrx;
	u32 tcke;
	u32 tmod;
	u32 trstl;
	u32 tzqcl;
	u32 tmrr;
	u32 tckesr;
	u32 tdpd;
	u32 trefi_mem_ddr3;
	u32 reserved5[(0x180 - 0x14c) / 4];
	u32 ecccfg;
	u32 ecctst;
	u32 eccclr;
	u32 ecclog;
	u32 reserved6[(0x200 - 0x190) / 4];
	u32 dtuwactl;
	u32 dturactl;
	u32 dtucfg;
	u32 dtuectl;
	u32 dtuwd0;
	u32 dtuwd1;
	u32 dtuwd2;
	u32 dtuwd3;
	u32 dtuwdm;
	u32 dturd0;
	u32 dturd1;
	u32 dturd2;
	u32 dturd3;
	u32 dtulfsrwd;
	u32 dtulfsrrd;
	u32 dtueaf;
	u32 dfitctrldelay;
	u32 dfiodtcfg;
	u32 dfiodtcfg1;
	u32 dfiodtrankmap;
	u32 dfitphywrdata;
	u32 dfitphywrlat;
	u32 dfitphywrdatalat;
	u32 reserved7;
	u32 dfitrddataen;
	u32 dfitphyrdlat;
	u32 reserved8[(0x270 - 0x268) / 4];
	u32 dfitphyupdtype0;
	u32 dfitphyupdtype1;
	u32 dfitphyupdtype2;
	u32 dfitphyupdtype3;
	u32 dfitctrlupdmin;
	u32 dfitctrlupdmax;
	u32 dfitctrlupddly;
	u32 reserved9;
	u32 dfiupdcfg;
	u32 dfitrefmski;
	u32 dfitctrlupdi;
	u32 reserved10[(0x2ac - 0x29c) / 4];
	u32 dfitrcfg0;
	u32 dfitrstat0;
	u32 dfitrwrlvlen;
	u32 dfitrrdlvlen;
	u32 dfitrrdlvlgateen;
	u32 dfiststat0;
	u32 dfistcfg0;
	u32 dfistcfg1;
	u32 reserved11;
	u32 dfitdramclken;
	u32 dfitdramclkdis;
	u32 dfistcfg2;
	u32 dfistparclr;
	u32 dfistparlog;
	u32 reserved12[(0x2f0 - 0x2e4) / 4];
	u32 dfilpcfg0;
	u32 reserved13[(0x300 - 0x2f4) / 4];
	u32 dfitrwrlvlresp0;
	u32 dfitrwrlvlresp1;
	u32 dfitrwrlvlresp2;
	u32 dfitrrdlvlresp0;
	u32 dfitrrdlvlresp1;
	u32 dfitrrdlvlresp2;
	u32 dfitrwrlvldelay0;
	u32 dfitrwrlvldelay1;
	u32 dfitrwrlvldelay2;
	u32 dfitrrdlvldelay0;
	u32 dfitrrdlvldelay1;
	u32 dfitrrdlvldelay2;
	u32 dfitrrdlvlgatedelay0;
	u32 dfitrrdlvlgatedelay1;
	u32 dfitrrdlvlgatedelay2;
	u32 dfitrcmd;
	u32 reserved14[(0x3f8 - 0x340) / 4];
	u32 ipvr;
	u32 iptr;
};
check_member(ddr_pctl, iptr, 0x03fc);

struct ddr_phy {
	u32 phy_reg0;
	u32 phy_reg1;
	u32 phy_reg2;
	u32 phy_reg3;
	u32 reserved0;
	u32 phy_reg5;
	u32 phy_reg6;
	u32 reserveds1[(0x24 - 0x1c) / 4];
	u32 phy_reg9;
	u32 reserveds2[(0x2c - 0x28) / 4];
	u32 phy_regb;
	u32 phy_regc;
	u32 reserveds3[(0x44 - 0x34) / 4];
	u32 phy_reg11;
	u32 phy_reg12;
	u32 phy_reg13;
	u32 phy_reg14;
	u32 reserved4;
	u32 phy_reg16;
	u32 phy_reg17;
	u32 phy_reg18;
	u32 reserveds5[(0x80 - 0x64) / 4];
	u32 phy_reg20;
	u32 phy_reg21;
	u32 reserveds6[(0x98 - 0x88) / 4];
	u32 phy_reg26;
	u32 phy_reg27;
	u32 phy_reg28;
	u32 reserveds7[(0xac - 0xa4) / 4];
	u32 phy_reg2b;
	u32 phy_reg2c;
	u32 reserveds8[(0xb8 - 0xb4) / 4];
	u32 phy_reg2e;
	u32 phy_reg2f;
	u32 phy_reg30;
	u32 phy_reg31;
	u32 reserveds9[(0xd8 - 0xc8) / 4];
	u32 phy_reg36;
	u32 phy_reg37;
	u32 phy_reg38;
	u32 reserveds10[(0xec - 0xe4) / 4];
	u32 phy_reg3b;
	u32 phy_reg3c;
	u32 reserveds11[(0xf8 - 0xf4) / 4];
	u32 phy_reg3e;
	u32 phy_reg3f;
	u32 reserveds12[(0x1c0 - 0x100) / 4];
	u32 phy_reg_skew_cs0data[(0x218 - 0x1c0) / 4];
	u32 reserveds13[(0x28c - 0x218) / 4];
	u32 phy_vref;
	/*dll bypass switch reg,0x290*/
	u32 phy_regdll;
	u32 reserveds14[(0x2c0 - 0x294) / 4];
	u32 phy_reg_ca_skew[(0x2f8 - 0x2c0) / 4];
	u32 reserveds15[(0x300 - 0x2f8) / 4];
	u32 phy_reg_skew_cs1data[(0x358 - 0x300) / 4];
	u32 reserveds16[(0x3c0 - 0x358) / 4];
	u32 phy_regf0;
	u32 phy_regf1;
	u32 reserveds17[(0x3e4 - 0x3c8) / 4];
	u32 phy_regf9;
	u32 phy_regfa;
	u32 phy_regfb;
	u32 phy_regfc;
	u32 reserved18;
	u32 reserved19;
	u32 phy_regff;
};
check_member(ddr_phy, phy_regff, 0x03fc);

union noc_timing_t {
	u32 d32;
	struct {
	unsigned acttoact : 6;
	unsigned rdtomiss : 6;
	unsigned wrtomiss : 6;
	unsigned burstlen : 3;
	unsigned rdtowr : 5;
	unsigned wrtord : 5;
	unsigned bwratio : 1;
	} b;
};

union noc_activate_t {
	u32 d32;
	struct {
	unsigned rrd : 4;
	unsigned faw : 6;
	unsigned fawbank : 1;
	unsigned reserved : 21;
	} b;
};

struct ddr_timing {
	u32 freq;
	struct pctl_timing {
		u32 togcnt1u;
		u32 tinit;
		u32 trsth;
		u32 togcnt100n;
		u32 trefi;
		u32 tmrd;
		u32 trfc;
		u32 trp;
		u32 trtw;
		u32 tal;
		u32 tcl;
		u32 tcwl;
		u32 tras;
		u32 trc;
		u32 trcd;
		u32 trrd;
		u32 trtp;
		u32 twr;
		u32 twtr;
		u32 texsr;
		u32 txp;
		u32 txpdll;
		u32 tzqcs;
		u32 tzqcsi;
		u32 tdqs;
		u32 tcksre;
		u32 tcksrx;
		u32 tcke;
		u32 tmod;
		u32 trstl;
		u32 tzqcl;
		u32 tmrr;
		u32 tckesr;
		u32 tdpd;
		u32 trefi_mem_ddr3;
	} pctl_timing;
	struct phy_timing {
		u32 mr[4];
		u32 bl;
		u32 cl_al;
	} phy_timing;
	union noc_timing_t noc_timing;
	u32 readlatency;
	union noc_activate_t activate;
	u32 devtodev;
};

struct ddr_config {
	/*
	 * 000: lpddr
	 * 001: ddr
	 * 010: ddr2
	 * 011: ddr3
	 * 100: lpddr2-s2
	 * 101: lpddr2-s4
	 * 110: lpddr3
	 */
	u32 ddr_type;
	u32 chn_cnt;
	u32 rank;
	u32 cs0_row;
	u32 cs1_row;

	/* 2: 4bank, 3: 8bank */
	u32 bank;
	u32 col;
	/* die buswidth, 2:32bit, 1:16bit, 0:8bit */
	u32 dbw;
	/* bw(0: 8bit, 1: 16bit, 2: 32bit) */
	u32 bw;
};

struct ddr_schedule {
	u32 col;
	u32 bank;
	u32 row;
};

enum {
	PHY_LOW_SPEED_MHZ		= 400,
	/* PHY_REG0 */
	CHN_ENABLE_SHIFT		= 4,
	DQ_16BIT_EN_MASK		= 3 << 4,
	DQ_16BIT_EN			= 3 << 4,
	DQ_32BIT_EN_MASK		= 0xf << 4,
	DQ_32BIT_EN			= 0xf << 4,
	RESET_DIGITAL_CORE_SHIFT	= 3,
	RESET_DIGITAL_CORE_MASK		= 1 << RESET_DIGITAL_CORE_SHIFT,
	RESET_DIGITAL_CORE_ACT		= 0,
	RESET_DIGITAL_CORE_DIS		= 1,
	RESET_ANALOG_LOGIC_SHIFT	= 2,
	RESET_ANALOG_LOGIC_MASK		= 1 << RESET_ANALOG_LOGIC_SHIFT,
	RESET_ANALOG_LOGIC_ACT		= 0,
	RESET_ANALOG_LOGIC_DIS		= 1,

	/* PHY_REG1 */
	MEMORY_SELECT_DDR3		= 0,
	MEMORY_SELECT_DDR2		= 1,
	MEMORY_SELECT_LPDDR2		= 2,
	PHY_BL_8			= 1 << 2,
	PHY_BL_4			= 0 << 2,

	/* PHY_REG2 */
	DQS_GATE_TRAINING_SEL_CS0	= 1 << 5,
	DQS_GATE_TRAINING_ACT		= 1,
	DQS_GATE_TRAINING_DIS		= 0,

	/* PHY_REG12 */
	CMD_PRCOMP_SHIFT		= 3,
	CMD_PRCOMP_MASK			= 0x1f << CMD_PRCOMP_SHIFT,

	/* DDRPHY_REG13 */
	CMD_DLL_BYPASS_SHIFT		= 4,
	CMD_DLL_BYPASS			= 1,
	CMD_DLL_BYPASS_MASK		= 1,
	CMD_DLL_BYPASS_DISABLE		= 0,

	/* DDRPHY_REG14 */
	CK_DLL_BYPASS_SHIFT		= 3,
	CK_DLL_BYPASS			= 1,
	CK_DLL_BYPASS_DISABLE		= 0,

	/* DDRPHY_REG26 */
	LEFT_CHN_A_DQ_DLL_SHIFT		= 4,
	LEFT_CHN_A_DQ_DLL_BYPASS	= 1,
	LEFT_CHN_A_DQ_DLL_BYPASS_MASK	= 1,
	LEFT_CHN_A_DQ_DLL_BYPASS_DIS	= 0,

	/* DDRPHY_REG27 */
	LEFT_CHN_A_DQS_DLL_SHIFT	= 3,
	LEFT_CHN_A_DQS_DLL_BYPASS	= 1,
	LEFT_CHN_A_DQS_DLL_BYPASS_DIS	= 0,

	/* DDRPHY_REG28 */
	LEFT_CHN_A_READ_DQS_22_5_DELAY	= 1,
	LEFT_CHN_A_READ_DQS_45_DELAY	= 2,

	/* DDRPHY_REG36 */
	RIGHT_CHN_A_DQ_DLL_SHIFT	= 4,
	RIGHT_CHN_A_DQ_DLL_BYPASS	= 1,
	RIGHT_CHN_A_DQ_DLL_BYPASS_MASK	= 1,
	RIGHT_CHN_A_DQ_DLL_BYPASS_DIS	= 0,

	/* DDRPHY_REG37 */
	RIGHT_CHN_A_DQS_DLL_SHIFT	= 3,
	RIGHT_CHN_A_DQS_DLL_BYPASS	= 1,
	RIGHT_CHN_A_DQS_DLL_BYPASS_DIS	= 0,

	/* DDRPHY_REG38 */
	RIGHT_CHN_A_READ_DQS_22_5_DELAY	= 1,
	RIGHT_CHN_A_READ_DQS_45_DELAY	= 2,

	/* PHY_REGDLL */
	RIGHT_CHN_A_TX_DQ_BYPASS_SHIFT	= 2,
	RIGHT_CHN_A_TX_DQ_BYPASS_SET	= 1,
	RIGHT_CHN_A_TX_DQ_BYPASS_DIS	= 0,
	LEFT_CHN_A_TX_DQ_BYPASS_SHIFT	= 1,
	LEFT_CHN_A_TX_DQ_BYPASS_SET	= 1,
	LEFT_CHN_A_TX_DQ_BYPASS_DIS	= 0,
	CMD_CK_DLL_BYPASS_SHIFT		= 0,
	CMD_CK_DLL_BYPASS_SET		= 1,
	CMD_CK_DLL_BYPASS_DIS		= 0,

	/* PHY_REGFF */
	CHN_A_TRAINING_DONE_MASK	= 3,
	CHN_A_HIGH_8BIT_TRAINING_DONE	= 1 << 1,
	CHN_A_LOW_8BIT_TRAINING_DONE	= 1,
};

/* DDRCTL */
enum {
	/* PCTL_SCTL */
	INIT_STATE				= 0,
	CFG_STATE				= 1,
	GO_STATE				= 2,
	SLEEP_STATE				= 3,
	WAKEUP_STATE				= 4,

	/* PCTL_STAT*/
	PCTL_CTL_STAT_MASK			= 0x7,
	INIT_MEM				= 0,
	CONFIG					= 1,
	CONFIG_REQ				= 2,
	ACCESS					= 3,
	ACCESS_REQ				= 4,
	LOW_POWER				= 5,
	LOW_POWER_ENTRY_REQ			= 6,
	LOW_POWER_EXIT_REQ			= 7,

	/* PCTL_MCMD */
	START_CMD				= 0x80000000,
	RANK_SEL_SHIFT				= 20,
	RANK_SEL_CS0				= 1,
	RANK_SEL_CS1				= 2,
	RANK_SEL_CS0_CS1			= 3,
	BANK_ADDR_SHIFT				= 17,
	BANK_ADDR_MASK				= 0x7,
	CMD_ADDR_SHIFT				= 4,
	CMD_ADDR_MASK				= 0x1fff,
	LPDDR23_MA_SHIFT			= 4,
	LPDDR23_MA_MASK				= 0xff,
	LPDDR23_OP_SHIFT			= 12,
	LPDDR23_OP_MASK				= 0xff,
	DDR3_DLL_RESET				= 1 << 8,
	DESELECT_CMD				= 0x0,
	PREA_CMD				= 0x1,
	REF_CMD					= 0x2,
	MRS_CMD					= 0x3,
	ZQCS_CMD				= 0x4,
	ZQCL_CMD				= 0x5,
	RSTL_CMD				= 0x6,
	MPR_CMD					= 0x8,
	DFICTRLUPD_CMD				= 0xa,
	MR0					= 0x0,
	MR1					= 0x1,
	MR2					= 0x2,
	MR3					= 0x3,

	/* PCTL_POWCTL */
	POWER_UP_START				= 1,
	POWER_UP_START_MASK			= 1,

	/* PCTL_POWSTAT */
	POWER_UP_DONE				= 1,

	/*PCTL_PPCFG*/
	PPMEM_EN_MASK				= 1,
	PPMEM_EN				= 1,
	PPMEM_DIS				= 0,
	/* PCTL_TREFI */
	UPD_REF					= 0x80000000,

	/* PCTL_DFISTCFG0 */
	DFI_DATA_BYTE_DISABLE_EN_SHIFT		= 2,
	DFI_DATA_BYTE_DISABLE_EN		= 1,
	DFI_FREQ_RATIO_EN_SHIFT			= 1,
	DFI_FREQ_RATIO_EN			= 1,
	DFI_INIT_START_SHIFT			= 0,
	DFI_INIT_START_EN			= 1,

	/* PCTL_DFISTCFG1 */
	DFI_DRAM_CLK_DISABLE_EN_DPD_SHIFT	= 1,
	DFI_DRAM_CLK_DISABLE_EN_DPD		= 1,
	DFI_DRAM_CLK_DISABLE_EN_SHIFT		= 0,
	DFI_DRAM_CLK_DISABLE_EN			= 1,

	/* PCTL_DFISTCFG2 */
	PARITY_EN_SHIFT				= 1,
	PARITY_EN				= 1,
	PARITY_INTR_EN_SHIFT			= 0,
	PARITY_INTR_EN				= 1,

	/* PCTL_DFILPCFG0 */
	DFI_LP_EN_PD				= 1,
	DFI_LP_WAKEUP_PD_SHIFT			= 4,
	DFI_LP_WAKEUP_PD_32_CYCLES		= 1,
	DFI_LP_EN_SR_SHIFT			= 8,
	DFI_LP_EN_SR				= 1,
	DFI_LP_WAKEUP_SR_SHIFT			= 12,
	DFI_LP_WAKEUP_SR_32_CYCLES		= 1,
	DFI_TLP_RESP_SHIFT			= 16,
	DFI_TLP_RESP				= 5,

	/* PCTL_DFITPHYUPDTYPE0 */
	TPHYUPD_TYPE0				= 1,

	/* PCTL_DFITPHYRDLAT */
	TPHY_RDLAT				= 0xd,

	/* PCTL_DFITPHYWRDATA */
	TPHY_WRDATA				= 0x0,

	/* PCTL_DFIUPDCFG */
	DFI_PHYUPD_DISABLE			= 0 << 1,
	DFI_CTRLUPD_DISABLE			= 0,

	/* PCTL_DFIODTCFG */
	RANK0_ODT_WRITE_SEL_SHIFT		= 3,
	RANK0_ODT_WRITE_SEL			= 1,
	RANK0_ODT_WRITE_DIS			= 0,
	RANK1_ODT_WRITE_SEL_SHIFT		= 11,
	RANK1_ODT_WRITE_SEL			= 1,
	RANK1_ODT_WRITE_DIS			= 0,

	/* PCTL_DFIODTCFG1 */
	ODT_LEN_BL8_W_SHIFT			= 16,
	ODT_LEN_BL8_W				= 7,
	ODT_LEN_BL8_W_0				= 0,

	/* PCTL_MCFG */
	MDDR_LPDDR23_CLOCK_STOP_IDLE_DIS	= 0 << 24,
	LPDDR2_EN				= 3 << 22,
	DDR3_EN					= 1 << 5,
	DDR2_EN					= 0 << 5,
	LPDDR2_S4				= 1 << 6,
	MEM_BL_8				= 1,
	MEM_BL_4				= 0,
	MDDR_LPDDR2_BL_4			= 1 << 20,
	MDDR_LPDDR2_BL_8			= 2 << 20,
	TFAW_CFG_5_TDDR				= 1 << 18,
	TFAW_CFG_6_TDDR				= 2 << 18,
	PD_EXIT_SLOW_EXIT_MODE			= 0 << 17,
	PD_EXIT_FAST_EXIT_MODE			= 1 << 17,
	PD_TYPE_ACT_PD				= 1 << 16,
	PD_IDLE_DISABLE				= 0 << 8,
	PD_IDLE_MASK				= 0xff << 8,
	PD_IDLE_SHIFT				= 8,
	TWO_T_SHIFT				= 3,

	/* PCTL_MCFG1 */
	SR_IDLE_MASK				= 0xff,
	HW_EXIT_IDLE_EN_SHIFT			= 31,
	HW_EXIT_IDLE_EN_MASK			= 1 << HW_EXIT_IDLE_EN_SHIFT,
	HW_EXIT_IDLE_EN				= 1 << HW_EXIT_IDLE_EN_SHIFT,

	/* PCTL_SCFG */
	HW_LOW_POWER_EN				= 1,
};

enum {
	/* PHY_DDR3_RON_RTT */
	PHY_RON_RTT_DISABLE			= 0,
	PHY_RON_RTT_451OHM			= 1,
	PHY_RON_RTT_225OHM			= 2,
	PHY_RON_RTT_150OHM			= 3,
	PHY_RON_RTT_112OHM			= 4,
	PHY_RON_RTT_90OHM			= 5,
	PHY_RON_RTT_75OHM			= 6,
	PHY_RON_RTT_64OHM			= 7,

	PHY_RON_RTT_56OHM			= 16,
	PHY_RON_RTT_50OHM			= 17,
	PHY_RON_RTT_45OHM			= 18,
	PHY_RON_RTT_41OHM			= 19,
	PHY_RON_RTT_37OHM			= 20,
	PHY_RON_RTT_34OHM			= 21,
	PHY_RON_RTT_33OHM			= 22,
	PHY_RON_RTT_30OHM			= 23,

	PHY_RON_RTT_28OHM			= 24,
	PHY_RON_RTT_26OHM			= 25,
	PHY_RON_RTT_25OHM			= 26,
	PHY_RON_RTT_23OHM			= 27,
	PHY_RON_RTT_22OHM			= 28,
	PHY_RON_RTT_21OHM			= 29,
	PHY_RON_RTT_20OHM			= 30,
	PHY_RON_RTT_19OHM			= 31,
};


/* CRU bits used by the DRAM init (from the downstream headers) */
enum {
	PLLPD0_POWER_DOWN	= 1,
	PLLPD0_NO_POWER_DOWN	= 0,
	POSTDIV1_SHIFT			= 12,
	POSTDIV1_MASK			= 0x7 << POSTDIV1_SHIFT,
	FBDIV_SHIFT			= 0,
	FBDIV_MASK			= 0xfff << FBDIV_SHIFT,
	PLLPD0_SHIFT			= 13,
	PLLPD0_MASK			= 1 << PLLPD0_SHIFT,
	DSMPD_SHIFT			= 12,
	DSMPD_MASK			= 1 << DSMPD_SHIFT,
	PLL_LOCK_SHIFT			= 10,
	POSTDIV2_SHIFT			= 6,
	POSTDIV2_MASK			= 0x7 << POSTDIV2_SHIFT,
	REFDIV_SHIFT			= 0,
	REFDIV_MASK			= 0x3f << REFDIV_SHIFT,
	FRACDIV_SHIFT			= 0,
	VPLL1_CLK_SEL_SHIFT		= 13,
	VPLL1_CLK_SEL_MASK		= 0x1 << VPLL1_CLK_SEL_SHIFT,
	VPLL1_CLK_SEL_WITHOUT_LVL_SHIFT	= 1,
	VPLL0_CLK_SEL_SHIFT		= 12,
	VPLL0_CLK_SEL_MASK		= 0x1 << VPLL0_CLK_SEL_SHIFT,
	VPLL0_CLK_SEL_WITHOUT_LVL_SHIFT	= 1,
	DPLL_CLK_SEL_SHIFT		= 11,
	DPLL_CLK_SEL_MASK		= 0x1 << DPLL_CLK_SEL_SHIFT,
	DPLL_CLK_SEL_WITHOUT_LVL_SHIFT	= 1,
	VPLL1_WORK_MODE_SHIFT		= 6,
	VPLL1_WORK_MODE_MASK		= 0x3 << VPLL1_WORK_MODE_SHIFT,
	VPLL1_WORK_MODE_XIN_OSC0	= 0,
	VPLL1_WORK_MODE_PLL		= 1,
	VPLL0_WORK_MODE_SHIFT		= 4,
	VPLL0_WORK_MODE_MASK		= 0x3 << VPLL0_WORK_MODE_SHIFT,
	VPLL0_WORK_MODE_XIN_OSC0	= 0,
	VPLL0_WORK_MODE_PLL		= 1,
	DPLL_WORK_MODE_SHIFT		= 2,
	DPLL_WORK_MODE_MASK		= 0x3 << DPLL_WORK_MODE_SHIFT,
	DPLL_WORK_MODE_XIN_OSC0		= 0,
	DPLL_WORK_MODE_PLL		= 1,
	DDRPHY4X_PLL_CLK_SEL_SHIFT	= 6,
	DDRPHY4X_PLL_CLK_SEL_MASK	= 0x3 << DDRPHY4X_PLL_CLK_SEL_SHIFT,
	DDRPHY4X_DIV_CON_SIHFT		= 0,
	DDRPHY4X_DIV_CON_MASK		= 0x7 << DDRPHY4X_DIV_CON_SIHFT,
	A_H_PCLK_BUS_PLL_SEL_SHIFT	= 6,
	A_H_PCLK_BUS_PLL_SEL_MASK	= 0x3 << A_H_PCLK_BUS_PLL_SEL_SHIFT,
	A_H_PCLK_BUS_PLL_SEL_DPLL	= 0,
	ACLK_BUS_DIV_CON_SHIFT		= 0,
	ACLK_BUS_DIV_CON_MASK		= 0x1f << ACLK_BUS_DIV_CON_SHIFT,
	ACLK_BUS_DIV_CON_5		= 5,
	PCLK_BUS_DIV_CON_SHIFT		= 8,
	PCLK_BUS_DIV_CON_MASK		= 0x1f << PCLK_BUS_DIV_CON_SHIFT,
	PCLK_BUS_DIV_CON_25		= 25,
	HCLK_BUS_DIV_CON_SHIFT		= 0,
	HCLK_BUS_DIV_CON_MASK		= 0x1f << HCLK_BUS_DIV_CON_SHIFT,
	HCLK_BUS_DIV_CON_13		= 13,
	CLK_CRYPTO_APK_SEL_SHIFT	= 14,
	CLK_CRYPTO_APK_SEL_MASK		= 0x3 << CLK_CRYPTO_APK_SEL_SHIFT,
	CLK_CRYPTO_APK_SEL_DPLL		= 0,
	CLK_CRYPTO_APK_DIV_SHIFT	= 8,
	CLK_CRYPTO_APK_DIV_MASK		= 0x1f << CLK_CRYPTO_APK_DIV_SHIFT,
	CLK_CRYPTO_APK_DIV_13		= 13,
	CLK_CRYPTO_PLL_SEL_SHIFT	= 6,
	CLK_CRYPTO_PLL_SEL_MASK		= 0x3 << CLK_CRYPTO_PLL_SEL_SHIFT,
	CLK_CRYPTO_PLL_SEL_DPLL		= 0,
	CLK_CRYPTO_DIV_CON_SHIFT	= 0,
	CLK_CRYPTO_DIV_CON_MASK		= 0x1f << CLK_CRYPTO_DIV_CON_SHIFT,
	CLK_CRYPTO_DIV_CON_13		= 13,
	CLK_UART0_DIV_CON_SHIFT		= 0,
	CLK_UART0_DIV_CON_MASK		= 0x1f << CLK_UART0_DIV_CON_SHIFT,
	CLK_UART1_DIV_CON_SHIFT		= 0,
	CLK_UART1_DIV_CON_MASK		= 0x1f << CLK_UART1_DIV_CON_SHIFT,
	CLK_UART2_DIV_CON_SHIFT		= 0,
	CLK_UART2_DIV_CON_MASK		= 0x1f << CLK_UART2_DIV_CON_SHIFT,
	CLK_UART3_DIV_CON_SHIFT		= 0,
	CLK_UART3_DIV_CON_MASK		= 0x1f << CLK_UART3_DIV_CON_SHIFT,
	CLK_UART4_DIV_CON_SHIFT		= 0,
	CLK_UART4_DIV_CON_MASK		= 0x1f << CLK_UART4_DIV_CON_SHIFT,
	A_H_P_PERI_PLL_SEL_SHIFT	= 6,
	A_H_P_PERI_PLL_SEL_MASK		= 0x3 << A_H_P_PERI_PLL_SEL_SHIFT,
	A_H_P_PERI_PLL_SEL_DPLL		= 0,
	ACLK_PERI_DIV_CON_SHIFT		= 0,
	ACLK_PERI_DIV_CON_MASK		= 0x1f << ACLK_PERI_DIV_CON_SHIFT,
	ACLK_PERI_DIV_CON_5		= 5,
	PCLK_PERI_DIV_CON_SHIFT		= 8,
	PCLK_PERI_DIV_CON_MASK		= 0x1f << PCLK_PERI_DIV_CON_SHIFT,
	PCLK_PERI_DIV_CON_27		= 27,
	HCLK_PERI_DIV_CON_SHIFT		= 0,
	HCLK_PERI_DIV_CON_MASK		= 0x1f << HCLK_PERI_DIV_CON_SHIFT,
	HCLK_PERI_DIV_CON_13		= 13,
	CLK_NANDC_PLL_SEL_SHIFT		= 6,
	CLK_NANDC_PLL_SEL_MASK		= 0x3 << CLK_NANDC_PLL_SEL_SHIFT,
	CLK_NANDC_PLL_SEL_DPLL		= 0,
	CLK_NANDC_DIV_CON_SHIFT		= 0,
	CLK_NANDC_DIV_CON_MASK		= 0x1f << CLK_NANDC_DIV_CON_SHIFT,
	CLK_NANDC_DIV_CON_13		= 13,
	CLK_SDMMC_PLL_SEL_SHIFT		= 8,
	CLK_SDMMC_PLL_SEL_MASK		= 0x3 << CLK_SDMMC_PLL_SEL_SHIFT,
	CLK_SDMMC_PLL_SEL_DPLL		= 0,
	CLK_SDMMC_DIV_CON_SHIFT		= 0,
	CLK_SDMMC_DIV_CON_MASK		= 0xff << CLK_SDMMC_DIV_CON_SHIFT,
	CLK_SDMMC_DIV_CON_27		= 27,
	CLK_EMMC_PLL_SEL_SHIFT		= 8,
	CLK_EMMC_PLL_SEL_MASK		= 0x3 << CLK_EMMC_PLL_SEL_SHIFT,
	CLK_EMMC_PLL_SEL_DPLL		= 0,
	CLK_EMMC_DIV_CON_SHIFT		= 0,
	CLK_EMMC_DIV_CON_MASK		= 0xff << CLK_EMMC_DIV_CON_SHIFT,
	CLK_EMMC_DIV_CON_27		= 27,
	CLK_SFC_PLL_SEL_SHIFT		= 14,
	CLK_SFC_PLL_SEL_MASK		= 0x3 << CLK_SFC_PLL_SEL_SHIFT,
	CLK_SFC_PLL_SEL_DPLL		= 0,
	CLK_SFC_DIV_CON_SHIFT		= 0,
	CLK_SFC_DIV_CON_MASK		= 0x7f << CLK_SFC_DIV_CON_SHIFT,
	CLK_SFC_DIV_CON_53		= 53,
	CLK_SPDIFTX_DIV_CON_SHIFT	= 0,
	CLK_SPDIFTX_DIV_CON_MASK	= 0x7f << CLK_SPDIFTX_DIV_CON_SHIFT,
	CLK_SPDIFTX_DIV_CON_15		= 15,
	PRESETN_DDRPHY_REQ_SHIFT	= 14,
	PRESETN_DDRPHY_REQ_MASK		= 0x1 << PRESETN_DDRPHY_REQ_SHIFT,
	PRESETN_DDRPHY_REQ_EN		= 1,
	PRESETN_DDRPHY_REQ_DIS		= 0,
	RESETN_DDRPHYDIV_REQ_SHIFT	= 13,
	RESETN_DDRPHYDIV_REQ_MASK	= 0x1 << RESETN_DDRPHYDIV_REQ_SHIFT,
	RESETN_DDRPHYDIV_REQ_EN		= 1,
	RESETN_DDRPHYDIV_REQ_DIS	= 0,
	RESETN_DDRPHY_REQ_SHIFT		= 12,
	RESETN_DDRPHY_REQ_MASK		= 0x1 << RESETN_DDRPHY_REQ_SHIFT,
	RESETN_DDRPHY_REQ_EN		= 1,
	RESETN_DDRPHY_REQ_DIS		= 0,
	PRESETN_DDRUPCTL_REQ_SHIFT	= 6,
	PRESETN_DDRUPCTL_REQ_MASK	= 0x1 << PRESETN_DDRUPCTL_REQ_SHIFT,
	PRESETN_DDRUPCTL_REQ_EN		= 1,
	PRESETN_DDRUPCTL_REQ_DIS	= 0,
	RESETN_DDRUPCTL_REQ_SHIFT	= 4,
	RESETN_DDRUPCTL_REQ_MASK	= 0x1 << RESETN_DDRUPCTL_REQ_SHIFT,
	RESETN_DDRUPCTL_REQ_EN		= 1,
	RESETN_DDRUPCTL_REQ_DIS		= 0,
};

/* GRF bits used by the DRAM init (from the downstream headers) */
enum {
	CYSYREQ_UPCTL_DDRSTDBY_SHIFT	= 5,
	CYSYREQ_UPCTL_DDRSTDBY_MASK	= 1 << CYSYREQ_UPCTL_DDRSTDBY_SHIFT,
	CYSYREQ_UPCTL_DDRSTDBY_EN	= 1,
	GRF_DDR_16BIT_EN_SHIFT		= 0,
	GRF_DDR_16BIT_EN_MASK		= 1 << GRF_DDR_16BIT_EN_SHIFT,
	GRF_DDR_16BIT_EN		= 1,
	NOC_MSCH_MAINDDR3_SHIFT		= 0,
	NOC_MSCH_MAINDDR3_MASK		= 0x1 << NOC_MSCH_MAINDDR3_SHIFT,
	NOC_MSCH_MAINDDR3_EN		= 1,
	NOC_MSCH_MAINDDR3_DIS		= 0,
};

#define CG_EXIT_TH		(250)

#define PATTERN			(0x5aa5f00f)

struct rk3308_ddr_standby {
	u32 con0;
	u32 con1;
	u32 status0;
};

struct rk3308_service_msch {
	u32 id_coreid;
	u32 id_revisionid;
	u32 ddrconf;
	u32 ddrtiming;
	u32 ddrmode;
	u32 readlatency;
};

enum {
	/* ddr standby */
	IDLE_TH_SHIFT				= 16,
	/* can not gate msch clk */
	MSCH_GATE_CLK_SHIFT			= 7,
	MSCH_GATE_CLK_EN			= 1,

	DDRPHY4X_GATE_SHIFT			= 6,
	DDRPHY4X_GATE_EN			= 1,

	UPCTL_CORE_CLK_GATE_SHIFT		= 5,
	UPCTL_CORE_CLK_GATE_EN			= 1,

	UPCTL_ACLK_GATE_SHIFT			= 4,
	UPCTL_ACLK_GATE_EN			= 1,

	CTL_IDLR_SHIFT				= 1,
	CTL_IDLR_EN				= 1,

	STDBY_EN_SHIFT				= 0,
	STDBY_EN				= 1,

	CG_EXIT_TH_SHIFT			= 16,

	STDBY_STATUS_SHIFT			= 0,
	STDBY_STATUS_MASK			= 0x7f << STDBY_STATUS_SHIFT,
	ST_STDBY				= 0x10,
};

enum {
	/* memory scheduler ddrtiming */
	BWRATIO_HALF_BW				= 0x80000000,
	BWRATIO_HALF_BW_DIS			= 0x0,

	PHY_TX_DE_SKEW_SHIFT			= 3,
	PHY_TX_DE_SKEW_EN			= 1,
};

struct dram_info {
	struct rk3308_cru *cru;
	struct rk3308_grf *grf;
	struct rk3308_sgrf *sgrf;
	struct rk3308_pmu *pmu;
	struct ddr_phy *phy;
	struct ddr_pctl *pctl;
	struct rk3308_ddr_standby *standby;
	struct rk3308_service_msch *service_msch;
	struct ram_info info;
};

struct sdram_params {
	u32 idle_pd;
	u32 idle_sr;
	u32 ddr_2t_en;
	u32 stdby_idle;
	struct ddr_config ddr_config_t;
	struct ddr_timing ddr_timing_t;
};

struct rk3308_ddr_skew {
	u32 a0_a1_skew[14];
	u32 cs0_dm0_skew[22];
};

struct rk3308_ddr_gd {
	struct sdram_head_info_v0 head_info;
	struct rk3308_ddr_skew ddr_skew;
};

int sdram_init(void);

#endif
