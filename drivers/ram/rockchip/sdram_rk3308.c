// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Rockchip Electronics Co., Ltd
 *
 * TPL DRAM init for RK3308 (DesignWare uPCTL + Innosilicon DDR PHY),
 * ported from the Rockchip downstream U-Boot (next-dev), where it was
 * split into sdram_rk3308.c and sdram_rv1108_pctl_phy.c.
 */

#include <config.h>
#include <debug_uart.h>
#include <dm.h>
#include <init.h>
#include <log.h>
#include <ram.h>
#include <syscon.h>
#include <asm/io.h>
#include <asm/arch-rockchip/clock.h>
#include <asm/arch-rockchip/grf_rk3308.h>
#include <asm/arch-rockchip/hardware.h>
#include <asm/arch-rockchip/sdram.h>
#include <linux/delay.h>

#ifdef CONFIG_TPL_BUILD
#include <asm/arch-rockchip/sdram_rk3308.h>

#define CRU_BASE		0xff500000
#define GRF_BASE		0xff000000
#define SGRF_BASE		0xff2b0000
#define DDR_PHY_BASE		0xff530000
#define DDR_PCTL_BASE		0xff010000
#define DDR_STANDBY_BASE	0xff030000
#define PMU_BASS_ADDR		0xff520000
#define SERVICE_MSCH_BASE	0xff5c8000

struct rk3308_ddr_gd ddr_gd = {
#include	"sdram-rk3308-ddr-skew.inc"
};

struct sdram_params sdram_configs[] = {
#if defined(CONFIG_RAM_ROCKCHIP_LPDDR2)
#include "sdram-rk3308-lpddr2-detect-451.inc"
#elif defined(CONFIG_RAM_ROCKCHIP_DDR2)
#include "sdram-rk3308-ddr2-detect-451.inc"
#else
#include "sdram-rk3308-ddr3-detect-589.inc"
#endif
};

static void enable_ddr_io_ret(struct dram_info *priv);
static void pll_set(u32 pll_type, struct dram_info *priv,
		    struct rockchip_pll_rate_table *pll_priv);
static int rk3308_debug_uart_id(void);
static void rkdclk_init(struct dram_info *priv,
			struct sdram_params *params_priv);
static void phy_pctrl_reset_cru(struct dram_info *priv);
static void pctl_cfg_grf(struct dram_info *priv,
			 struct sdram_params *params_priv);
static void ddr_msch_cfg(struct dram_info *priv,
			 struct sdram_params *params_priv);
static void ddr_msch_cfg_rbc(struct sdram_params *params_priv,
			     struct dram_info *priv);
static void ddr_phy_skew_cfg(struct dram_info *priv);
static void set_ds_odt(struct dram_info *priv,
		       struct sdram_params *params_priv);
static void ddr_phy_dqs_rx_dll_cfg(struct dram_info *priv, u32 freq);
static void ddr_msch_get_max_col(struct dram_info *priv,
				 struct ddr_schedule *sch_priv);
static void ddr_msch_get_max_row(struct dram_info *priv,
				 struct ddr_schedule *sch_priv);
static void enable_ddr_standby(struct dram_info *priv,
			       struct sdram_params *params_priv);
static void modify_sdram_params(struct dram_info *priv,
				struct sdram_params *params_priv);
static int check_rd_gate(struct dram_info *priv);
static u32 dram_test(u32 i, u32 dqs);
static void modify_data_training(struct dram_info *priv,
				 struct sdram_params *params_priv);
static void enable_low_power(struct dram_info *priv,
			     struct sdram_params *params_priv);
static void copy_to_reg(u32 *dest, const u32 *src, u32 n);
static void phy_pctrl_reset(struct dram_info *priv);
static void phy_dll_bypass_set(struct dram_info *priv, unsigned int freq);
static void send_command(struct dram_info *priv,
			 u32 rank, u32 cmd, u32 arg);
static void memory_init(struct dram_info *priv,
			struct sdram_params *params_priv);
static void move_to_config_state(struct dram_info *priv);
static void move_to_access_state(struct dram_info *priv);
static void pctl_cfg(struct dram_info *priv,
		     struct sdram_params *params_priv);
static void phy_cfg(struct dram_info *priv,
		    struct sdram_params *params_priv);
static void dram_cfg_rbc(struct dram_info *priv,
			 struct sdram_params *params_priv);
static void data_training(struct dram_info *priv);
static int sdram_detect(struct dram_info *priv,
			struct sdram_params *params_priv);
static void sdram_all_config(struct dram_info *priv,
			     struct sdram_params *params_priv);
static int rk3308_dram_init(struct dram_info *sdram_priv,
			    struct sdram_params *params_priv);

#define DDR3_DDR2_ODT_DISABLE_FREQ	(666)

#define DDR2_TRFC_256MBIT	(75)
#define DDR2_TRFC_512MBIT	(105)
#define DDR2_TRFC_1GBIT		(128)
#define DDR2_TRFC_2GBIT		(195)
#define DDR2_TRFC_4GBIT		(328)

#define DDR3_TRFC_512MBIT	(90)
#define DDR3_TRFC_1GBIT		(110)
#define DDR3_TRFC_2GBIT		(160)
#define DDR3_TRFC_4GBIT		(300)
#define DDR3_TRFC_8GBIT		(350)

#define LPDDR2_TRFC_8GBIT		(210) /*ns*/
#define LPDDR2_TRFC_4GBIT		(130) /*ns*/
#define LPDDR2_TREC_512MBIT		(90) /*ns*/

static void enable_ddr_io_ret(struct dram_info *priv)
{
	rk_clrsetreg(&priv->pmu->sft_con_lo, DDR_IO_RET_CFG_MASK,
		     DDR_IO_RET_CFG << DDR_IO_RET_CFG_SHIFT);

	rk_clrsetreg(&priv->grf->upctrl_con0, GRF_DDR_16BIT_EN_MASK,
		     GRF_DDR_16BIT_EN << GRF_DDR_16BIT_EN_SHIFT);
}

static void pll_set(u32 pll_type, struct dram_info *priv,
		    struct rockchip_pll_rate_table *pll_priv)
{
	/* pll power down */
	rk_clrsetreg(&priv->cru->pll[pll_type].con1, PLLPD0_MASK,
		     PLLPD0_POWER_DOWN << PLLPD0_SHIFT);
	rk_clrsetreg(&priv->cru->pll[pll_type].con1,
		     DSMPD_MASK, pll_priv->dsmpd << DSMPD_SHIFT);

	/* set pll freq */
	rk_clrsetreg(&priv->cru->pll[pll_type].con0,
		     FBDIV_MASK | POSTDIV1_MASK,
		     pll_priv->fbdiv << FBDIV_SHIFT |
		     pll_priv->postdiv1 << POSTDIV1_SHIFT);
	rk_clrsetreg(&priv->cru->pll[pll_type].con1,
		     POSTDIV2_MASK | REFDIV_MASK,
		     pll_priv->postdiv2 << POSTDIV2_SHIFT |
		     pll_priv->refdiv << REFDIV_SHIFT);
	writel(pll_priv->frac << FRACDIV_SHIFT,
	       &priv->cru->pll[pll_type].con2);
	/* pll power up */
	rk_clrsetreg(&priv->cru->pll[pll_type].con1, PLLPD0_MASK,
		     PLLPD0_NO_POWER_DOWN << PLLPD0_SHIFT);

	/* wait until pll lock */
	while (!(readl(&priv->cru->pll[pll_type].con1) &
	       (1u << PLL_LOCK_SHIFT)))
		udelay(1);
}

static int rk3308_debug_uart_id(void)
{
#if defined(CONFIG_DEBUG_UART_BASE)
	switch (CONFIG_DEBUG_UART_BASE) {
	case 0xff0a0000:
		return 0;
	case 0xff0b0000:
		return 1;
	case 0xff0c0000:
		return 2;
	case 0xff0d0000:
		return 3;
	case 0xff0e0000:
		return 4;
	}
#endif
	return -1;
}

static void rkdclk_init(struct dram_info *priv,
			struct sdram_params *params_priv)
{
	u32 ddr_pll_sel;
	u32 ddr_phy_div_con;
	u32 uart_div[5] = {15, 15, 15, 15, 15};
	struct rockchip_pll_rate_table rk3308_pll_div;

	/* DPLL VPLL0 VPLL1 mode in 24MHz*/
	rk_clrsetreg(&priv->cru->mode, VPLL1_WORK_MODE_MASK,
		     VPLL1_WORK_MODE_XIN_OSC0 << VPLL1_WORK_MODE_SHIFT);
	rk_clrsetreg(&priv->cru->mode, VPLL0_WORK_MODE_MASK,
		     VPLL0_WORK_MODE_XIN_OSC0 << VPLL0_WORK_MODE_SHIFT);
	rk_clrsetreg(&priv->cru->mode, DPLL_WORK_MODE_MASK,
		     DPLL_WORK_MODE_XIN_OSC0 << DPLL_WORK_MODE_SHIFT);

	/* set PLL without level shift */
	rk_clrsetreg(&priv->cru->mode, VPLL1_CLK_SEL_MASK,
		     VPLL1_CLK_SEL_WITHOUT_LVL_SHIFT << VPLL1_CLK_SEL_SHIFT);
	rk_clrsetreg(&priv->cru->mode, VPLL0_CLK_SEL_MASK,
		     VPLL0_CLK_SEL_WITHOUT_LVL_SHIFT << VPLL0_CLK_SEL_SHIFT);
	rk_clrsetreg(&priv->cru->mode, DPLL_CLK_SEL_MASK,
		     DPLL_CLK_SEL_WITHOUT_LVL_SHIFT << DPLL_CLK_SEL_SHIFT);

	/* set vpll1 in 903.168MHz vco = 1.806GHz */
	rk3308_pll_div.refdiv = 2;
	rk3308_pll_div.fbdiv = 150;
	rk3308_pll_div.postdiv1 = 2;
	rk3308_pll_div.postdiv2 = 1;
	rk3308_pll_div.frac = 0x872B02;
	rk3308_pll_div.dsmpd = 0;
	pll_set(VPLL1, priv, &rk3308_pll_div);

	if (params_priv->ddr_timing_t.freq == 393) {
		/* set vpll0 in 786.432MHz vco = 3.146GHz */
		rk3308_pll_div.refdiv = 2;
		rk3308_pll_div.fbdiv = 262;
		rk3308_pll_div.postdiv1 = 4;
		rk3308_pll_div.postdiv2 = 1;
		rk3308_pll_div.frac = 0x24DD2F;
		rk3308_pll_div.dsmpd = 0;
	} else {
		/* set vpll0 in 1179.648MHz, vco = 2.359GHz*/
		rk3308_pll_div.refdiv = 2;
		rk3308_pll_div.fbdiv = 196;
		rk3308_pll_div.postdiv1 = 2;
		rk3308_pll_div.postdiv2 = 1;
		rk3308_pll_div.frac = 0x9BA5E3;
		rk3308_pll_div.dsmpd = 0;
	}
	pll_set(VPLL0, priv, &rk3308_pll_div);

	if (params_priv->ddr_timing_t.freq == 800) {
		ddr_pll_sel = 0;
		ddr_phy_div_con = 0;
	} else if (params_priv->ddr_timing_t.freq == 589) {
		ddr_pll_sel = 1;
		ddr_phy_div_con = 0;
	} else if (params_priv->ddr_timing_t.freq == 451) {
		ddr_pll_sel = 2;
		ddr_phy_div_con = 0;
	} else if (params_priv->ddr_timing_t.freq == 393) {
		ddr_pll_sel = 1;
		ddr_phy_div_con = 0;
	} else if (params_priv->ddr_timing_t.freq == 294) {
		ddr_pll_sel = 1;
		ddr_phy_div_con = 1;
	} else if (params_priv->ddr_timing_t.freq == 225) {
		ddr_pll_sel = 2;
		ddr_phy_div_con = 1;
	} else {
		printascii("err\n");
		while (1)
			;
	}

	/* dpll default set in 1300MHz */
	if (params_priv->ddr_timing_t.freq == 800) {
		/* set dpll in 1584 MHz ,vco=3.168G*/
		rk3308_pll_div.refdiv = 1;
		rk3308_pll_div.fbdiv = 132;
		rk3308_pll_div.postdiv1 = 2;
		rk3308_pll_div.postdiv2 = 1;
		rk3308_pll_div.frac = 0;
		rk3308_pll_div.dsmpd = 1;
	} else {
		/* 1300000000,vco = 1.3GHz */
		rk3308_pll_div.refdiv = 6;
		rk3308_pll_div.fbdiv = 325;
		rk3308_pll_div.postdiv1 = 1;
		rk3308_pll_div.postdiv2 = 1;
		rk3308_pll_div.frac = 0;
		rk3308_pll_div.dsmpd = 1;
	}

	pll_set(DPLL, priv, &rk3308_pll_div);

	/* set ddrphy freq */
	rk_clrsetreg(&priv->cru->clksel_con[1],
		     DDRPHY4X_PLL_CLK_SEL_MASK | DDRPHY4X_DIV_CON_MASK,
		     ddr_pll_sel << DDRPHY4X_PLL_CLK_SEL_SHIFT |
		     ddr_phy_div_con << DDRPHY4X_DIV_CON_SIHFT);

	/* set aclk_bus 216.7MHz */
	rk_clrsetreg(&priv->cru->clksel_con[5],
		     A_H_PCLK_BUS_PLL_SEL_MASK | ACLK_BUS_DIV_CON_MASK,
		     A_H_PCLK_BUS_PLL_SEL_DPLL << A_H_PCLK_BUS_PLL_SEL_SHIFT |
		     ACLK_BUS_DIV_CON_5 << ACLK_BUS_DIV_CON_SHIFT);
	/* set pclk_bus 50MHz,hclk_bus 92.857MHz */
	rk_clrsetreg(&priv->cru->clksel_con[6],
		     PCLK_BUS_DIV_CON_MASK | HCLK_BUS_DIV_CON_MASK,
		     PCLK_BUS_DIV_CON_25 << PCLK_BUS_DIV_CON_SHIFT |
		     HCLK_BUS_DIV_CON_13 << HCLK_BUS_DIV_CON_SHIFT);
	/* set crypto 92.857MHz,crypto_apk 92.857MHz */
	rk_clrsetreg(&priv->cru->clksel_con[7],
		     CLK_CRYPTO_APK_SEL_MASK | CLK_CRYPTO_APK_DIV_MASK |
		     CLK_CRYPTO_PLL_SEL_MASK | CLK_CRYPTO_DIV_CON_MASK,
		     CLK_CRYPTO_APK_SEL_DPLL << CLK_CRYPTO_APK_SEL_SHIFT |
		     CLK_CRYPTO_APK_DIV_13 << CLK_CRYPTO_APK_DIV_SHIFT |
		     CLK_CRYPTO_PLL_SEL_DPLL << CLK_CRYPTO_PLL_SEL_SHIFT |
		     CLK_CRYPTO_DIV_CON_13 << CLK_CRYPTO_DIV_CON_SHIFT);
	/* set aclk_peri 216.7MHz */
	rk_clrsetreg(&priv->cru->clksel_con[36],
		     A_H_P_PERI_PLL_SEL_MASK | ACLK_PERI_DIV_CON_MASK,
		     A_H_P_PERI_PLL_SEL_DPLL << A_H_P_PERI_PLL_SEL_SHIFT |
		     ACLK_PERI_DIV_CON_5 << ACLK_PERI_DIV_CON_SHIFT);
	/* set hclk_peri 92.857MHz,pclk_peri 46.428MHz */
	rk_clrsetreg(&priv->cru->clksel_con[37],
		     PCLK_PERI_DIV_CON_MASK | HCLK_PERI_DIV_CON_MASK,
		     PCLK_PERI_DIV_CON_27 << PCLK_PERI_DIV_CON_SHIFT |
		     HCLK_PERI_DIV_CON_13 << HCLK_PERI_DIV_CON_SHIFT);
	/* set NANDC 92.857MHz */
	rk_clrsetreg(&priv->cru->clksel_con[38],
		     CLK_NANDC_PLL_SEL_MASK |
		     CLK_NANDC_DIV_CON_MASK,
		     CLK_NANDC_PLL_SEL_DPLL << CLK_NANDC_PLL_SEL_SHIFT |
		     CLK_NANDC_DIV_CON_13 << CLK_NANDC_DIV_CON_SHIFT);
	/* set SDMMC 46.4/(internal freq_div 2)=23.2MHz */
	rk_clrsetreg(&priv->cru->clksel_con[39],
		     CLK_SDMMC_PLL_SEL_MASK |
		     CLK_SDMMC_DIV_CON_MASK,
		     CLK_SDMMC_PLL_SEL_DPLL << CLK_SDMMC_PLL_SEL_SHIFT |
		     CLK_SDMMC_DIV_CON_27 << CLK_SDMMC_DIV_CON_SHIFT);
	/* set emmc 46.4/(internal freq_div 2)=23.2MHz */
	rk_clrsetreg(&priv->cru->clksel_con[41],
		     CLK_EMMC_PLL_SEL_MASK |
		     CLK_EMMC_DIV_CON_MASK,
		     CLK_EMMC_PLL_SEL_DPLL << CLK_EMMC_PLL_SEL_SHIFT |
		     CLK_EMMC_DIV_CON_27 << CLK_EMMC_DIV_CON_SHIFT);
	/* set SFC 24.07/(internal freq_div 2)=12.0MHz */
	rk_clrsetreg(&priv->cru->clksel_con[42],
		     CLK_SFC_PLL_SEL_MASK | CLK_SFC_DIV_CON_MASK,
		     CLK_SFC_PLL_SEL_DPLL << CLK_SFC_PLL_SEL_SHIFT |
		     CLK_SFC_DIV_CON_53 << CLK_SFC_DIV_CON_SHIFT);
	/* set spdif tx lower than 100Mhz */
	rk_clrsetreg(&priv->cru->clksel_con[48],
		     CLK_SPDIFTX_DIV_CON_MASK,
		     CLK_SPDIFTX_DIV_CON_15 << CLK_SPDIFTX_DIV_CON_SHIFT);

	/* keep the debug UART (clocked from xin_osc0) undivided */
	if (rk3308_debug_uart_id() >= 0)
		uart_div[rk3308_debug_uart_id()] = 0;

	/* set uart0~4 lower than 100Mhz */
	rk_clrsetreg(&priv->cru->clksel_con[10],
		     CLK_UART0_DIV_CON_MASK,
		     uart_div[0] << CLK_UART0_DIV_CON_SHIFT);
	rk_clrsetreg(&priv->cru->clksel_con[13],
		     CLK_UART1_DIV_CON_MASK,
		     uart_div[1] << CLK_UART1_DIV_CON_SHIFT);
	rk_clrsetreg(&priv->cru->clksel_con[16],
		     CLK_UART2_DIV_CON_MASK,
		     uart_div[2] << CLK_UART2_DIV_CON_SHIFT);
	rk_clrsetreg(&priv->cru->clksel_con[19],
		     CLK_UART3_DIV_CON_MASK,
		     uart_div[3] << CLK_UART3_DIV_CON_SHIFT);
	rk_clrsetreg(&priv->cru->clksel_con[22],
		     CLK_UART4_DIV_CON_MASK,
		     uart_div[4] << CLK_UART4_DIV_CON_SHIFT);

	/* pll clk in pll out */
	rk_clrsetreg(&priv->cru->mode, VPLL1_WORK_MODE_MASK,
		     VPLL1_WORK_MODE_PLL << VPLL1_WORK_MODE_SHIFT);
	rk_clrsetreg(&priv->cru->mode, VPLL0_WORK_MODE_MASK,
		     VPLL0_WORK_MODE_PLL << VPLL0_WORK_MODE_SHIFT);
	rk_clrsetreg(&priv->cru->mode, DPLL_WORK_MODE_MASK,
		     DPLL_WORK_MODE_PLL << DPLL_WORK_MODE_SHIFT);
}

static void phy_pctrl_reset_cru(struct dram_info *priv)
{
	rk_clrsetreg(&priv->cru->softrst_con[1],
		     PRESETN_DDRPHY_REQ_MASK | RESETN_DDRPHYDIV_REQ_MASK |
		     RESETN_DDRPHY_REQ_MASK | PRESETN_DDRUPCTL_REQ_MASK |
		     RESETN_DDRUPCTL_REQ_MASK,
		     PRESETN_DDRPHY_REQ_EN << PRESETN_DDRPHY_REQ_SHIFT |
		     RESETN_DDRPHYDIV_REQ_EN << RESETN_DDRPHYDIV_REQ_SHIFT |
		     RESETN_DDRPHY_REQ_EN << RESETN_DDRPHY_REQ_SHIFT |
		     PRESETN_DDRUPCTL_REQ_EN << PRESETN_DDRUPCTL_REQ_SHIFT |
		     RESETN_DDRUPCTL_REQ_EN << RESETN_DDRUPCTL_REQ_SHIFT);
	udelay(10);

	rk_clrsetreg(&priv->cru->softrst_con[1],
		     PRESETN_DDRPHY_REQ_MASK | RESETN_DDRPHYDIV_REQ_MASK |
		     RESETN_DDRPHY_REQ_MASK,
		     PRESETN_DDRPHY_REQ_DIS << PRESETN_DDRPHY_REQ_SHIFT |
		     RESETN_DDRPHYDIV_REQ_DIS << RESETN_DDRPHYDIV_REQ_SHIFT |
		     RESETN_DDRPHY_REQ_DIS << RESETN_DDRPHY_REQ_SHIFT);
	udelay(10);

	rk_clrsetreg(&priv->cru->softrst_con[1],
		     PRESETN_DDRUPCTL_REQ_MASK | RESETN_DDRUPCTL_REQ_MASK,
		     PRESETN_DDRUPCTL_REQ_DIS << PRESETN_DDRUPCTL_REQ_SHIFT |
		     RESETN_DDRUPCTL_REQ_DIS << RESETN_DDRUPCTL_REQ_SHIFT);
	udelay(10);
}

static void pctl_cfg_grf(struct dram_info *priv,
			 struct sdram_params *params_priv)
{
	if (params_priv->ddr_config_t.ddr_type == DDR3 ||
	    params_priv->ddr_config_t.ddr_type == DDR2)
		rk_clrsetreg(&priv->grf->soc_con12, NOC_MSCH_MAINDDR3_MASK,
			     NOC_MSCH_MAINDDR3_EN << NOC_MSCH_MAINDDR3_SHIFT);
	else
		rk_clrsetreg(&priv->grf->soc_con12, NOC_MSCH_MAINDDR3_MASK,
			     NOC_MSCH_MAINDDR3_DIS << NOC_MSCH_MAINDDR3_SHIFT);
}

static void ddr_msch_cfg(struct dram_info *priv,
			 struct sdram_params *params_priv)
{
	writel(BWRATIO_HALF_BW | params_priv->ddr_timing_t.noc_timing.d32,
	       &priv->service_msch->ddrtiming);
	writel(params_priv->ddr_timing_t.readlatency,
	       &priv->service_msch->readlatency);
}

static void ddr_msch_cfg_rbc(struct sdram_params *params_priv,
			     struct dram_info *priv)
{
	int i = 0;

	if (params_priv->ddr_config_t.bank == 3) {
		/* bank = 8 */
		if (params_priv->ddr_config_t.col == 10)
			i = 1;
		else if (params_priv->ddr_config_t.col == 11)
			i = 2;
		else
			goto msch_err;

	} else if (params_priv->ddr_config_t.bank == 2) {
		/* bank = 4 */
		i = 0;
	} else {
		goto msch_err;
	}

	writel(i, &priv->service_msch->ddrconf);
	return;

msch_err:
	printascii("msch_err\n");
	while (1)
		;
}

static void ddr_phy_skew_cfg(struct dram_info *priv)
{
	copy_to_reg(&priv->phy->phy_reg_ca_skew[0],
		    &ddr_gd.ddr_skew.a0_a1_skew[0], 14 * 4);
	copy_to_reg(&priv->phy->phy_reg_skew_cs0data[0],
		    &ddr_gd.ddr_skew.cs0_dm0_skew[0], 22 * 4);

	writel(PHY_TX_DE_SKEW_EN << PHY_TX_DE_SKEW_SHIFT,
	       &priv->phy->phy_reg2);
}

static void set_ds_odt(struct dram_info *priv,
		       struct sdram_params *params_priv)
{
	/* set phy drive resistance */
	writel(PHY_RON_RTT_56OHM, &priv->phy->phy_reg11);
	clrsetbits_le32(&priv->phy->phy_reg12, CMD_PRCOMP_MASK,
			PHY_RON_RTT_56OHM << CMD_PRCOMP_SHIFT);

	writel(PHY_RON_RTT_45OHM, &priv->phy->phy_reg16);
	writel(PHY_RON_RTT_45OHM, &priv->phy->phy_reg18);
	writel(PHY_RON_RTT_56OHM, &priv->phy->phy_reg20);
	writel(PHY_RON_RTT_56OHM, &priv->phy->phy_reg2f);
	writel(PHY_RON_RTT_56OHM, &priv->phy->phy_reg30);
	writel(PHY_RON_RTT_56OHM, &priv->phy->phy_reg3f);
	if (params_priv->ddr_config_t.ddr_type == LPDDR2) {
		writel(PHY_RON_RTT_DISABLE, &priv->phy->phy_reg21);
		writel(PHY_RON_RTT_DISABLE, &priv->phy->phy_reg2e);
		writel(PHY_RON_RTT_DISABLE, &priv->phy->phy_reg31);
		writel(PHY_RON_RTT_DISABLE, &priv->phy->phy_reg3e);
	} else {
		if (params_priv->ddr_timing_t.freq >
		    DDR3_DDR2_ODT_DISABLE_FREQ) {
			/*set phy odt*/
			writel(PHY_RON_RTT_225OHM, &priv->phy->phy_reg21);
			writel(PHY_RON_RTT_225OHM, &priv->phy->phy_reg2e);
			writel(PHY_RON_RTT_225OHM, &priv->phy->phy_reg31);
			writel(PHY_RON_RTT_225OHM, &priv->phy->phy_reg3e);
		} else {
			/*disable phy odt*/
			writel(PHY_RON_RTT_DISABLE, &priv->phy->phy_reg21);
			writel(PHY_RON_RTT_DISABLE, &priv->phy->phy_reg2e);
			writel(PHY_RON_RTT_DISABLE, &priv->phy->phy_reg31);
			writel(PHY_RON_RTT_DISABLE, &priv->phy->phy_reg3e);
		}
	}
}

static void ddr_phy_dqs_rx_dll_cfg(struct dram_info *priv, u32 freq)
{
	if (freq > 736) {
		/* 22.5 degree delay */
		writel(LEFT_CHN_A_READ_DQS_22_5_DELAY, &priv->phy->phy_reg28);
		writel(RIGHT_CHN_A_READ_DQS_22_5_DELAY, &priv->phy->phy_reg38);
	} else {
		/* 45 degree delay */
		writel(LEFT_CHN_A_READ_DQS_45_DELAY, &priv->phy->phy_reg28);
		writel(RIGHT_CHN_A_READ_DQS_45_DELAY, &priv->phy->phy_reg38);
	}
}

static void ddr_msch_get_max_col(struct dram_info *priv,
				 struct ddr_schedule *sch_priv)
{
	writel(2, &priv->service_msch->ddrconf);
	sch_priv->col = 11;
	sch_priv->bank = 3;
}

static void ddr_msch_get_max_row(struct dram_info *priv,
				 struct ddr_schedule *sch_priv)
{
	writel(1, &priv->service_msch->ddrconf);
	sch_priv->row = 15;
	sch_priv->col = 10;
	sch_priv->bank = 3;
}

static void enable_ddr_standby(struct dram_info *priv,
			       struct sdram_params *params_priv)
{
	rk_clrsetreg(&priv->grf->upctrl_con0, CYSYREQ_UPCTL_DDRSTDBY_MASK,
		     CYSYREQ_UPCTL_DDRSTDBY_EN <<
		     CYSYREQ_UPCTL_DDRSTDBY_SHIFT);

	/* CG_EXIT_TH is equal phy dll lock time when we gate phy 4x clk */
	writel(CG_EXIT_TH << CG_EXIT_TH_SHIFT, &priv->standby->con1);

	if (params_priv->stdby_idle == 128) {
		if (params_priv->ddr_timing_t.freq == 451)
			params_priv->stdby_idle = 105;
		else if (params_priv->ddr_timing_t.freq == 393)
			params_priv->stdby_idle = 10;
	}
	writel(params_priv->stdby_idle << IDLE_TH_SHIFT |
		DDRPHY4X_GATE_EN << DDRPHY4X_GATE_SHIFT |
		UPCTL_CORE_CLK_GATE_EN << UPCTL_CORE_CLK_GATE_SHIFT |
		UPCTL_ACLK_GATE_EN << UPCTL_ACLK_GATE_SHIFT |
		CTL_IDLR_EN << CTL_IDLR_SHIFT |
		STDBY_EN << STDBY_EN_SHIFT, &priv->standby->con0);

	while (1) {
		if ((readl(&priv->standby->status0) &
		    STDBY_STATUS_MASK) == ST_STDBY) {
			break;
		}
	}
}

static void modify_sdram_params(struct dram_info *priv,
				struct sdram_params *params_priv)
{
	u32 tmp = 0;
	u32 bw = 1;
	u32 nMHz = params_priv->ddr_timing_t.freq;

	size_t	size = 1llu << (bw +
		       params_priv->ddr_config_t.col +
		       params_priv->ddr_config_t.cs0_row +
		       params_priv->ddr_config_t.bank);

	move_to_config_state(priv);
	switch (params_priv->ddr_config_t.ddr_type) {
	case DDR2:
		if (size <= 0x4000000)
			tmp = DDR2_TRFC_512MBIT;
		else if (size <= 0x8000000)
			tmp = DDR2_TRFC_1GBIT;
		else if (size <= 0x10000000)
			tmp = DDR2_TRFC_2GBIT;
		else
			tmp = DDR2_TRFC_4GBIT;

		priv->pctl->trfc = (tmp * nMHz + 999) / 1000;
		tmp = (((tmp + 10) * nMHz + 999) / 1000);
		if (tmp < 200)
			tmp = 200;
		priv->pctl->texsr = tmp & 0x3FF;
		break;
	case DDR3:
		if (size <= 0x4000000)
			tmp = DDR3_TRFC_512MBIT;
		else if (size <= 0x8000000)
			tmp = DDR3_TRFC_1GBIT;
		else if (size <= 0x10000000)
			tmp = DDR3_TRFC_2GBIT;
		else if (size <= 0x20000000)
			tmp = DDR3_TRFC_4GBIT;
		else
			tmp = DDR3_TRFC_8GBIT;
		priv->pctl->trfc = (tmp * nMHz + 999) / 1000;
		break;
	case LPDDR2:
		if (size <= 0x4000000)
			tmp = LPDDR2_TREC_512MBIT;
		else if (size <= 0x20000000)
			tmp = LPDDR2_TRFC_4GBIT;
		else
			tmp = LPDDR2_TRFC_8GBIT;

		priv->pctl->trfc = (tmp * nMHz + 999) / 1000;
		tmp = (((tmp + 10) * nMHz + 999) / 1000);
		if (tmp < 2)
			tmp = 2;
		priv->pctl->texsr = tmp & 0x3FF;
		break;
	}
	move_to_access_state(priv);
}

static int check_rd_gate(struct dram_info *priv)
{
	u32 max_val = 0;
	u32 min_val = 0xff;
	u32 gate[2];

	gate[0] = readl(&priv->phy->phy_regfb);
	gate[1] = readl(&priv->phy->phy_regfc);
	max_val = max(gate[0], gate[1]);
	min_val = min(gate[0], gate[1]);

	if (max_val > 0x80 || min_val < 0x20)
		return -1;
	else
		return 0;
}

static u32 dram_test(u32 i, u32 dqs)
{
	for (phys_addr_t j = 4 * dqs; j < 0x2000; j += 8)
		writel(PATTERN + i, j);

	for (phys_addr_t j = 4 * dqs; j < 0x2000; j += 8)
		if ((PATTERN + i) != readl(j))
			return 1;

	return 0;
}

/**
 * modify_data_training() - Setting DQS gating calibration bypass,
 * scanning data training range and then select center one.
 */
#define PHY_REG3C(n)		(0x10 * (n))

static void modify_data_training(struct dram_info *priv,
				 struct sdram_params *params_priv)
{
	u32 value = 0;
	u32 i = 0, dqs = 0;
	u32 max_value = 0, min_value = 0;

	writel(readl(&priv->phy->phy_regfb), &priv->phy->phy_reg2c);
	writel(readl(&priv->phy->phy_regfc), &priv->phy->phy_reg3c);

	/* DQS gating calibration bypass */
	setbits_le32(&priv->phy->phy_reg2, BIT(1));

	/* rk3308 only support DQS0, DQS1 */
	for (dqs = 0; dqs < 2; dqs++) {
		value = readl(&priv->phy->phy_regfb + dqs);
		i = 0;
		while (dram_test(i, dqs) == 0) {
			i++;
			writel(value + i,
			       &priv->phy->phy_reg2c + PHY_REG3C(dqs));
		}
		max_value = value + i - 1;

		i = 1;
		writel(value - i, &priv->phy->phy_reg2c + PHY_REG3C(dqs));
		while (dram_test(i, dqs) == 0) {
			i++;
			writel(value - i,
			       &priv->phy->phy_reg2c + PHY_REG3C(dqs));
		}
		min_value = value - i + 1;

		/* select center one as gate training result */
		writel((max_value + min_value + 1) / 2,
		       &priv->phy->phy_reg2c + PHY_REG3C(dqs));
	}
	printascii("REG2C: 0x");
	printhex8(readl(&priv->phy->phy_reg2c));
	printascii(", 0x");
	printhex8(readl(&priv->phy->phy_reg3c));
	printascii("\n");
}

static void enable_low_power(struct dram_info *priv,
			     struct sdram_params *params_priv)
{
	move_to_config_state(priv);

	if (params_priv->idle_pd == 48 && params_priv->idle_sr == 10) {
		if (params_priv->ddr_timing_t.freq == 451) {
			params_priv->idle_sr = 28;
			params_priv->idle_pd = 7;
		} else if (params_priv->ddr_timing_t.freq == 393) {
			params_priv->idle_sr = 31;
			params_priv->idle_pd = 15;
		}
	}
	clrsetbits_le32(&priv->pctl->mcfg, PD_IDLE_MASK,
			params_priv->idle_pd << PD_IDLE_SHIFT);
	clrsetbits_le32(&priv->pctl->mcfg1,
			SR_IDLE_MASK | HW_EXIT_IDLE_EN_MASK,
			params_priv->idle_sr | HW_EXIT_IDLE_EN);

	/* DDRCTL in low_power status because of auto self-refresh */
	writel(GO_STATE, &priv->pctl->sctl);
}

/* uPCTL / PHY sequencing (was sdram_rv1108_pctl_phy.c downstream) */

static void copy_to_reg(u32 *dest, const u32 *src, u32 n)
{
	int i;

	for (i = 0; i < n / sizeof(u32); i++) {
		writel(*src, dest);
		src++;
		dest++;
	}
}

static void phy_pctrl_reset(struct dram_info *priv)
{
	phy_pctrl_reset_cru(priv);
	clrsetbits_le32(&priv->phy->phy_reg0,
			RESET_DIGITAL_CORE_MASK | RESET_ANALOG_LOGIC_MASK,
			RESET_DIGITAL_CORE_ACT << RESET_DIGITAL_CORE_SHIFT |
			RESET_ANALOG_LOGIC_ACT << RESET_ANALOG_LOGIC_SHIFT);
	udelay(1);
	clrsetbits_le32(&priv->phy->phy_reg0,
			RESET_ANALOG_LOGIC_MASK,
			RESET_ANALOG_LOGIC_DIS << RESET_ANALOG_LOGIC_SHIFT);
	udelay(5);
	clrsetbits_le32(&priv->phy->phy_reg0,
			RESET_DIGITAL_CORE_MASK,
			RESET_DIGITAL_CORE_DIS << RESET_DIGITAL_CORE_SHIFT);
	udelay(1);
}

static void phy_dll_bypass_set(struct dram_info *priv, unsigned int freq)
{
	clrsetbits_le32(&priv->phy->phy_reg13,
			CMD_DLL_BYPASS_MASK << CMD_DLL_BYPASS_SHIFT,
			CMD_DLL_BYPASS << CMD_DLL_BYPASS_SHIFT);

	writel(CK_DLL_BYPASS_DISABLE << CK_DLL_BYPASS_SHIFT,
	       &priv->phy->phy_reg14);

	clrsetbits_le32(&priv->phy->phy_reg26,
			LEFT_CHN_A_DQ_DLL_BYPASS_MASK << LEFT_CHN_A_DQ_DLL_SHIFT,
			LEFT_CHN_A_DQ_DLL_BYPASS << LEFT_CHN_A_DQ_DLL_SHIFT);
	writel(LEFT_CHN_A_DQS_DLL_BYPASS_DIS << LEFT_CHN_A_DQS_DLL_SHIFT,
	       &priv->phy->phy_reg27);

	clrsetbits_le32(&priv->phy->phy_reg36,
			RIGHT_CHN_A_DQ_DLL_BYPASS_MASK << RIGHT_CHN_A_DQ_DLL_SHIFT,
			RIGHT_CHN_A_DQ_DLL_BYPASS << RIGHT_CHN_A_DQ_DLL_SHIFT);
	writel(RIGHT_CHN_A_DQS_DLL_BYPASS_DIS <<
	       RIGHT_CHN_A_DQS_DLL_SHIFT, &priv->phy->phy_reg37);

	if (freq <= PHY_LOW_SPEED_MHZ) {
		writel(RIGHT_CHN_A_TX_DQ_BYPASS_SET <<
		       RIGHT_CHN_A_TX_DQ_BYPASS_SHIFT |
		       LEFT_CHN_A_TX_DQ_BYPASS_SET <<
		       LEFT_CHN_A_TX_DQ_BYPASS_SHIFT |
		       CMD_CK_DLL_BYPASS_SET << CMD_CK_DLL_BYPASS_SHIFT,
		       &priv->phy->phy_regdll);
	} else {
		writel(RIGHT_CHN_A_TX_DQ_BYPASS_DIS <<
		       RIGHT_CHN_A_TX_DQ_BYPASS_SHIFT |
		       LEFT_CHN_A_TX_DQ_BYPASS_DIS <<
		       LEFT_CHN_A_TX_DQ_BYPASS_SHIFT |
		       CMD_CK_DLL_BYPASS_DIS << CMD_CK_DLL_BYPASS_SHIFT,
				&priv->phy->phy_regdll);
	}

	ddr_phy_dqs_rx_dll_cfg(priv, freq);
}

static void send_command(struct dram_info *priv,
			 u32 rank, u32 cmd, u32 arg)
{
	writel((START_CMD | (rank << RANK_SEL_SHIFT) | arg | cmd),
	       &priv->pctl->mcmd);
	while (readl(&priv->pctl->mcmd) & START_CMD)
		;
}

static void memory_init(struct dram_info *priv,
			struct sdram_params *params_priv)
{
	u32 mr0;

	if (params_priv->ddr_config_t.ddr_type == DDR3 ||
	    params_priv->ddr_config_t.ddr_type == DDR2) {
		send_command(priv, RANK_SEL_CS0_CS1, DESELECT_CMD, 0);
		udelay(1);
		send_command(priv, RANK_SEL_CS0_CS1, PREA_CMD, 0);
		send_command(priv, RANK_SEL_CS0_CS1, DESELECT_CMD, 0);
		udelay(1);
		send_command(priv, RANK_SEL_CS0_CS1, MRS_CMD,
			     (MR2 & BANK_ADDR_MASK) << BANK_ADDR_SHIFT |
			     (params_priv->ddr_timing_t.phy_timing.mr[2] &
			     CMD_ADDR_MASK) << CMD_ADDR_SHIFT);

		send_command(priv, RANK_SEL_CS0_CS1, MRS_CMD,
			     (MR3 & BANK_ADDR_MASK) << BANK_ADDR_SHIFT |
			     (params_priv->ddr_timing_t.phy_timing.mr[3] &
			     CMD_ADDR_MASK) << CMD_ADDR_SHIFT);

		send_command(priv, RANK_SEL_CS0_CS1, MRS_CMD,
			     (MR1 & BANK_ADDR_MASK) << BANK_ADDR_SHIFT |
			     (params_priv->ddr_timing_t.phy_timing.mr[1] &
			     CMD_ADDR_MASK) << CMD_ADDR_SHIFT);

		mr0 = params_priv->ddr_timing_t.phy_timing.mr[0];
		if (params_priv->ddr_config_t.ddr_type == DDR3) {
			send_command(priv, RANK_SEL_CS0_CS1, MRS_CMD,
				     (MR0 & BANK_ADDR_MASK) << BANK_ADDR_SHIFT |
				     (((mr0 | DDR3_DLL_RESET) &
				       CMD_ADDR_MASK) << CMD_ADDR_SHIFT));

			send_command(priv, RANK_SEL_CS0_CS1, ZQCL_CMD, 0);
		} else {
			send_command(priv, RANK_SEL_CS0_CS1, MRS_CMD,
				     (MR0 & BANK_ADDR_MASK) << BANK_ADDR_SHIFT |
				     (((mr0 | DDR3_DLL_RESET) &
				       CMD_ADDR_MASK) << CMD_ADDR_SHIFT));
			send_command(priv, RANK_SEL_CS0_CS1, PREA_CMD, 0);
			send_command(priv, RANK_SEL_CS0_CS1, REF_CMD, 0);
			send_command(priv, RANK_SEL_CS0_CS1, REF_CMD, 0);
			send_command(priv, RANK_SEL_CS0_CS1, MRS_CMD,
				     (MR0 & BANK_ADDR_MASK) <<
				     BANK_ADDR_SHIFT |
				     ((mr0 & CMD_ADDR_MASK) <<
				      CMD_ADDR_SHIFT));
		}
	} else {
		/* reset */
		send_command(priv, RANK_SEL_CS0_CS1, MRS_CMD,
			     (63 & LPDDR23_MA_MASK) << LPDDR23_MA_SHIFT |
			     (0 & LPDDR23_OP_MASK) <<
			     LPDDR23_OP_SHIFT);
		/* tINIT5 */
		udelay(10);
		/* ZQ calibration Init */
		send_command(priv, RANK_SEL_CS0, MRS_CMD,
			     (10 & LPDDR23_MA_MASK) << LPDDR23_MA_SHIFT |
			     (0xFF & LPDDR23_OP_MASK) <<
			     LPDDR23_OP_SHIFT);
		/* tZQINIT */
		udelay(1);
		send_command(priv, RANK_SEL_CS1, MRS_CMD,
			     (10 & LPDDR23_MA_MASK) << LPDDR23_MA_SHIFT |
			     (0xFF & LPDDR23_OP_MASK) <<
			     LPDDR23_OP_SHIFT);
		/* tZQINIT */
		udelay(1);
		send_command(priv, RANK_SEL_CS0_CS1, MRS_CMD,
			     (1 & LPDDR23_MA_MASK) << LPDDR23_MA_SHIFT |
			     (params_priv->ddr_timing_t.phy_timing.mr[1] &
			     LPDDR23_OP_MASK) << LPDDR23_OP_SHIFT);
		send_command(priv, RANK_SEL_CS0_CS1, MRS_CMD,
			     (2 & LPDDR23_MA_MASK) << LPDDR23_MA_SHIFT |
			     (params_priv->ddr_timing_t.phy_timing.mr[2] &
			     LPDDR23_OP_MASK) << LPDDR23_OP_SHIFT);
		send_command(priv, RANK_SEL_CS0_CS1, MRS_CMD,
			     (3 & LPDDR23_MA_MASK) << LPDDR23_MA_SHIFT |
			     (params_priv->ddr_timing_t.phy_timing.mr[3] &
			     LPDDR23_OP_MASK) << LPDDR23_OP_SHIFT);
	}
}

static void move_to_config_state(struct dram_info *priv)
{
	unsigned int state;

	while (1) {
		state = readl(&priv->pctl->stat) & PCTL_CTL_STAT_MASK;
		switch (state) {
		case LOW_POWER:
			writel(WAKEUP_STATE, &priv->pctl->sctl);
			while ((readl(&priv->pctl->stat) & PCTL_CTL_STAT_MASK)
				!= ACCESS)
				;
			/*
			 * If at low power state, need wakeup first, and then
			 * enter the config, so fallthrough
			 */
		case ACCESS:
		case INIT_MEM:
			writel(CFG_STATE, &priv->pctl->sctl);
			while ((readl(&priv->pctl->stat) & PCTL_CTL_STAT_MASK)
				!= CONFIG)
				;
			break;
		case CONFIG:
			return;
		default:
			break;
		}
	}
}

static void move_to_access_state(struct dram_info *priv)
{
	unsigned int state;

	while (1) {
		state = readl(&priv->pctl->stat) & PCTL_CTL_STAT_MASK;
		switch (state) {
		case LOW_POWER:
			writel(WAKEUP_STATE, &priv->pctl->sctl);
			while ((readl(&priv->pctl->stat) &
				PCTL_CTL_STAT_MASK) != ACCESS)
				;
			break;
		case INIT_MEM:
			writel(CFG_STATE, &priv->pctl->sctl);
			while ((readl(&priv->pctl->stat) &
				PCTL_CTL_STAT_MASK) != CONFIG)
				;
			/* fallthrough */
		case CONFIG:
			writel(GO_STATE, &priv->pctl->sctl);
			while ((readl(&priv->pctl->stat) &
				PCTL_CTL_STAT_MASK) != ACCESS)
				;
			break;
		case ACCESS:
			return;
		default:
			break;
		}
	}
}

static void pctl_cfg(struct dram_info *priv,
		     struct sdram_params *params_priv)
{
	u32 reg;
	u32 burstlen;
	u32 bl_mddr_lpddr2;

	/* DFI config */
	writel(DFI_DATA_BYTE_DISABLE_EN << DFI_DATA_BYTE_DISABLE_EN_SHIFT |
	       DFI_INIT_START_EN << DFI_INIT_START_SHIFT,
	       &priv->pctl->dfistcfg0);
	writel(DFI_DRAM_CLK_DISABLE_EN_DPD <<
	       DFI_DRAM_CLK_DISABLE_EN_DPD_SHIFT |
	       DFI_DRAM_CLK_DISABLE_EN << DFI_DRAM_CLK_DISABLE_EN_SHIFT,
	       &priv->pctl->dfistcfg1);
	writel(PARITY_EN << PARITY_EN_SHIFT |
	       PARITY_INTR_EN << PARITY_INTR_EN_SHIFT, &priv->pctl->dfistcfg2);

	writel(TPHYUPD_TYPE0, &priv->pctl->dfitphyupdtype0);
	writel(TPHY_RDLAT, &priv->pctl->dfitphyrdlat);
	writel(TPHY_WRDATA, &priv->pctl->dfitphywrdata);

	writel(DFI_PHYUPD_DISABLE | DFI_CTRLUPD_DISABLE,
	       &priv->pctl->dfiupdcfg);

	copy_to_reg(&priv->pctl->togcnt1u,
		    &params_priv->ddr_timing_t.pctl_timing.togcnt1u,
		    sizeof(struct pctl_timing));
	/*
	 * the PHY runs in 1:2 mode, noc_timing.b.burstlen
	 * have divide by scheuler clock, so need to * 4
	 */
	burstlen = params_priv->ddr_timing_t.noc_timing.b.burstlen * 4;

	if (params_priv->ddr_config_t.ddr_type == DDR3 ||
	    params_priv->ddr_config_t.ddr_type == DDR2) {
		writel((RANK0_ODT_WRITE_SEL << RANK0_ODT_WRITE_SEL_SHIFT |
		       RANK1_ODT_WRITE_SEL << RANK1_ODT_WRITE_SEL_SHIFT),
		       &priv->pctl->dfiodtcfg);

		writel(ODT_LEN_BL8_W << ODT_LEN_BL8_W_SHIFT,
		       &priv->pctl->dfiodtcfg1);

		writel(params_priv->ddr_timing_t.pctl_timing.trsth,
		       &priv->pctl->trsth);
		if (params_priv->ddr_config_t.ddr_type == DDR3)
			writel(MDDR_LPDDR23_CLOCK_STOP_IDLE_DIS | DDR3_EN |
			       MEM_BL_8 | TFAW_CFG_5_TDDR |
			       PD_EXIT_SLOW_EXIT_MODE | PD_TYPE_ACT_PD |
			       PD_IDLE_DISABLE |
			       params_priv->ddr_2t_en << TWO_T_SHIFT,
			       &priv->pctl->mcfg);
		else if (burstlen == 8)
			writel(MDDR_LPDDR23_CLOCK_STOP_IDLE_DIS | DDR2_EN |
			       MEM_BL_8 | TFAW_CFG_5_TDDR |
			       PD_EXIT_SLOW_EXIT_MODE | PD_TYPE_ACT_PD |
			       PD_IDLE_DISABLE |
			       params_priv->ddr_2t_en << TWO_T_SHIFT,
			       &priv->pctl->mcfg);
		else
			writel(MDDR_LPDDR23_CLOCK_STOP_IDLE_DIS | DDR2_EN |
			       MEM_BL_4 | TFAW_CFG_5_TDDR |
			       PD_EXIT_SLOW_EXIT_MODE | PD_TYPE_ACT_PD |
			       PD_IDLE_DISABLE |
			       params_priv->ddr_2t_en << TWO_T_SHIFT,
			       &priv->pctl->mcfg);
		writel(DFI_LP_EN_SR << DFI_LP_EN_SR_SHIFT |
		       DFI_LP_WAKEUP_SR_32_CYCLES << DFI_LP_WAKEUP_SR_SHIFT |
		       DFI_TLP_RESP << DFI_TLP_RESP_SHIFT,
		       &priv->pctl->dfilpcfg0);

		reg = readl(&priv->pctl->tcl);
		writel((reg - 1) / 2 - 1, &priv->pctl->dfitrddataen);
		reg = readl(&priv->pctl->tcwl);
		writel((reg - 1) / 2 - 1, &priv->pctl->dfitphywrlat);
	} else {
		if (burstlen == 4)
			bl_mddr_lpddr2 = MDDR_LPDDR2_BL_4;
		else
			bl_mddr_lpddr2 = MDDR_LPDDR2_BL_8;
		writel((RANK0_ODT_WRITE_DIS << RANK0_ODT_WRITE_SEL_SHIFT |
		       RANK1_ODT_WRITE_DIS << RANK1_ODT_WRITE_SEL_SHIFT),
		       &priv->pctl->dfiodtcfg);

		writel(ODT_LEN_BL8_W_0 << ODT_LEN_BL8_W_SHIFT,
		       &priv->pctl->dfiodtcfg1);

		writel(0, &priv->pctl->trsth);
		writel(MDDR_LPDDR23_CLOCK_STOP_IDLE_DIS | LPDDR2_EN |
			LPDDR2_S4 | bl_mddr_lpddr2 |
		       TFAW_CFG_6_TDDR | PD_EXIT_FAST_EXIT_MODE |
		       PD_TYPE_ACT_PD | PD_IDLE_DISABLE, &priv->pctl->mcfg);
		writel(DFI_LP_EN_SR << DFI_LP_EN_SR_SHIFT |
		       DFI_LP_WAKEUP_SR_32_CYCLES << DFI_LP_WAKEUP_SR_SHIFT |
		       DFI_TLP_RESP << DFI_TLP_RESP_SHIFT |
		       DFI_LP_WAKEUP_PD_32_CYCLES << DFI_LP_WAKEUP_PD_SHIFT |
		       DFI_LP_EN_PD,
		       &priv->pctl->dfilpcfg0);

		reg = readl(&priv->pctl->tcl);
		writel(reg / 2 - 1, &priv->pctl->dfitrddataen);
		reg = readl(&priv->pctl->tcwl);
		writel(reg / 2 - 1, &priv->pctl->dfitphywrlat);
	}
	pctl_cfg_grf(priv, params_priv);
	setbits_le32(&priv->pctl->scfg, HW_LOW_POWER_EN);

	/* only support x16 memory */
	clrsetbits_le32(&priv->pctl->ppcfg, PPMEM_EN_MASK, PPMEM_EN);
}

static void phy_cfg(struct dram_info *priv,
		    struct sdram_params *params_priv)
{
	u32 burstlen;

	burstlen = params_priv->ddr_timing_t.noc_timing.b.burstlen * 4;
	burstlen = (burstlen == 4) ? PHY_BL_4 : PHY_BL_8;
	ddr_msch_cfg(priv, params_priv);
	ddr_phy_skew_cfg(priv);
	switch (params_priv->ddr_config_t.ddr_type) {
	case DDR2:
		writel(MEMORY_SELECT_DDR2 | PHY_BL_8, &priv->phy->phy_reg1);
		break;
	case DDR3:
		writel(MEMORY_SELECT_DDR3 | PHY_BL_8, &priv->phy->phy_reg1);
		break;
	case LPDDR2:
	default:
		writel(MEMORY_SELECT_LPDDR2 | burstlen, &priv->phy->phy_reg1);
		break;
	}

	writel(params_priv->ddr_timing_t.phy_timing.cl_al,
	       &priv->phy->phy_regb);
	writel(params_priv->ddr_timing_t.pctl_timing.tcwl,
	       &priv->phy->phy_regc);

	set_ds_odt(priv, params_priv);

	/* only support x16 memory */
	clrsetbits_le32(&priv->phy->phy_reg0, DQ_16BIT_EN_MASK,
			DQ_16BIT_EN);
}

static void dram_cfg_rbc(struct dram_info *priv,
			 struct sdram_params *params_priv)
{
	move_to_config_state(priv);
	ddr_msch_cfg_rbc(params_priv, priv);
	move_to_access_state(priv);
}

static void data_training(struct dram_info *priv)
{
	u32 value;
	u32 tmp = 0;
	u32 tmp1 = 0;
	u32 timeout = 1000;

	/* disable auto refresh */
	value = readl(&priv->pctl->trefi);
	writel(UPD_REF, &priv->pctl->trefi);

	tmp1 = readl(&priv->phy->phy_reg2);

	writel(DQS_GATE_TRAINING_SEL_CS0 | DQS_GATE_TRAINING_DIS | tmp1,
	       &priv->phy->phy_reg2);
	writel(DQS_GATE_TRAINING_SEL_CS0 | DQS_GATE_TRAINING_ACT | tmp1,
	       &priv->phy->phy_reg2);

		/* delay until data training done */
	while (tmp != (CHN_A_HIGH_8BIT_TRAINING_DONE |
	       CHN_A_LOW_8BIT_TRAINING_DONE)) {
		udelay(1);
		tmp = (readl(&priv->phy->phy_regff) & CHN_A_TRAINING_DONE_MASK);
		timeout--;
		if (!timeout)
			break;
	}

	writel(DQS_GATE_TRAINING_SEL_CS0 | DQS_GATE_TRAINING_DIS | tmp1,
	       &priv->phy->phy_reg2);

	send_command(priv, RANK_SEL_CS0_CS1, PREA_CMD, 0);

	writel(value | UPD_REF, &priv->pctl->trefi);
}

static int sdram_detect(struct dram_info *priv,
			struct sdram_params *params_priv)
{
	u32 row, col, row_max, col_max, bank_max;
	u32 bw = 1;
	phys_addr_t test_addr;
	struct ddr_schedule ddr_sch;

	/* if col detect wrong,row needs initial */
	row = 0;

	/* detect col */
	move_to_config_state(priv);
	ddr_msch_get_max_col(priv, &ddr_sch);
	col_max = ddr_sch.col;
	bank_max = ddr_sch.bank;
	move_to_access_state(priv);

	for (col = col_max; col >= 10; col--) {
		writel(0, CFG_SYS_SDRAM_BASE);
		test_addr = (phys_addr_t)(CFG_SYS_SDRAM_BASE +
		    (1ul << (col + bw - 1ul)));
		writel(PATTERN, test_addr);
		if ((readl(test_addr) == PATTERN) &&
		    (readl(CFG_SYS_SDRAM_BASE) == 0))
			break;
	}
	if (col <= 9)
		goto cap_err;
	params_priv->ddr_config_t.col = col;

	if (params_priv->ddr_config_t.ddr_type == DDR3) {
		params_priv->ddr_config_t.bank = 3;
	} else {
		writel(0, CFG_SYS_SDRAM_BASE);
		test_addr = (phys_addr_t)(CFG_SYS_SDRAM_BASE +
		    (1ul << (bank_max + col_max +
		    bw - 1ul)));
		writel(PATTERN, test_addr);
		if ((readl(test_addr) == PATTERN) &&
		    (readl(CFG_SYS_SDRAM_BASE) == 0))
			params_priv->ddr_config_t.bank = 3;
		else
			params_priv->ddr_config_t.bank = 2;
	}

	/* detect row */
	move_to_config_state(priv);
	ddr_msch_get_max_row(priv, &ddr_sch);
	move_to_access_state(priv);
	col_max = ddr_sch.col;
	row_max = ddr_sch.row;

	for (row = row_max; row >= 12; row--) {
		writel(0, CFG_SYS_SDRAM_BASE);
		test_addr = (phys_addr_t)(CFG_SYS_SDRAM_BASE +
				(1ul << (row + bank_max +
				col_max + bw - 1ul)));

		writel(PATTERN, test_addr);
		if ((readl(test_addr) == PATTERN) &&
		    (readl(CFG_SYS_SDRAM_BASE) == 0))
			break;
	}
	if (row <= 11)
		goto cap_err;
	params_priv->ddr_config_t.cs0_row = row;
	return 0;
cap_err:
	return -EAGAIN;
}

#define DDR_VERSION	0x2

static void sdram_all_config(struct dram_info *priv,
			     struct sdram_params *params_priv)
{
	u32 version = DDR_VERSION;
	u32 os_reg = 0;
	u32 row_12 = 0;
	u32 ddr_info = 0;
	/* rk3308 only supports 1 channel, x16 ddr bus, x16 memory */
	u32 chn_cnt = 0;
	u32 rank = 1;
	u32 bw = 1;
	u32 dbw = 1;
	size_t size = 0;

	/* os_reg2 */
	os_reg = (params_priv->ddr_config_t.ddr_type & SYS_REG_DDRTYPE_MASK) <<
		 SYS_REG_DDRTYPE_SHIFT |
		 (chn_cnt & SYS_REG_NUM_CH_MASK) <<
		 SYS_REG_NUM_CH_SHIFT |
		 ((rank - 1) & SYS_REG_RANK_MASK) <<
		 SYS_REG_RANK_SHIFT(0) |
		 ((params_priv->ddr_config_t.col - 9) & SYS_REG_COL_MASK) <<
		 SYS_REG_COL_SHIFT(0) |
		 ((params_priv->ddr_config_t.bank == 3 ? 0 : 1) &
		 SYS_REG_BK_MASK) << SYS_REG_BK_SHIFT(0) |
		 ((params_priv->ddr_config_t.cs0_row - 13) &
		 SYS_REG_CS0_ROW_MASK) << SYS_REG_CS0_ROW_SHIFT(0) |
		 (bw & SYS_REG_BW_MASK) <<
		 SYS_REG_BW_SHIFT(0) |
		 (dbw & SYS_REG_DBW_MASK) <<
		 SYS_REG_DBW_SHIFT(0);

	writel(os_reg, &priv->grf->os_reg2);

	/* os_reg3 */
	if (params_priv->ddr_config_t.cs0_row == 12)
		row_12 = 1;
	os_reg = (version & SYS_REG_VERSION_MASK) <<
		 SYS_REG_VERSION_SHIFT | (row_12 &
		 SYS_REG_EXTEND_CS0_ROW_MASK) <<
		 SYS_REG_EXTEND_CS0_ROW_SHIFT(0);
	writel(os_reg, &priv->grf->os_reg3);

	printascii("In\n");
	printdec(params_priv->ddr_timing_t.freq);
	printascii("MHz\n");
	switch (params_priv->ddr_config_t.ddr_type & SYS_REG_DDRTYPE_MASK) {
	case 2:
		printascii("DDR2\n");
		break;
	case 5:
		printascii("LPDDR2\n");
		break;
	case 3:
	default:
		printascii("DDR3\n");
		break;
	}
	printascii(" Col=");
	printdec(params_priv->ddr_config_t.col);
	printascii(" Bank=");
	printdec(params_priv->ddr_config_t.bank);
	printascii(" Row=");
	printdec(params_priv->ddr_config_t.cs0_row);

	size = 1llu << (bw +
	       params_priv->ddr_config_t.col +
	       params_priv->ddr_config_t.cs0_row +
	       params_priv->ddr_config_t.bank);
	ddr_info = size >> 20;
	printascii(" Size=");
	printdec(ddr_info);
	printascii("MB\n");
	printascii("msch:");
	ddr_info = readl(&priv->service_msch->ddrconf);
	printdec(ddr_info);
	printascii("\n");

	priv->info.base = CFG_SYS_SDRAM_BASE;
	priv->info.size = size;
}

static int rk3308_dram_init(struct dram_info *sdram_priv,
			    struct sdram_params *params_priv)
{
	/* pmu enable ddr io retention */
	enable_ddr_io_ret(sdram_priv);
	rkdclk_init(sdram_priv, params_priv);
	phy_pctrl_reset(sdram_priv);
	phy_dll_bypass_set(sdram_priv, params_priv->ddr_timing_t.freq);
	pctl_cfg(sdram_priv, params_priv);
	phy_cfg(sdram_priv, params_priv);
	writel(POWER_UP_START, &sdram_priv->pctl->powctl);
	while (!(readl(&sdram_priv->pctl->powstat) & POWER_UP_DONE))
		;

	memory_init(sdram_priv, params_priv);
re_training:
	move_to_config_state(sdram_priv);
	data_training(sdram_priv);
	move_to_access_state(sdram_priv);
	if (sdram_detect(sdram_priv, params_priv)) {
		while (1)
			;
	}
	if (check_rd_gate(sdram_priv))
		goto re_training;

	/* workaround data training not in middle */
	modify_data_training(sdram_priv, params_priv);

	dram_cfg_rbc(sdram_priv, params_priv);
	sdram_all_config(sdram_priv, params_priv);
	enable_low_power(sdram_priv, params_priv);

	return 0;
}

int sdram_init(void)
{
	struct dram_info sdram_priv;
	struct sdram_params *params = sdram_configs;

	sdram_priv.cru = (void *)CRU_BASE;
	sdram_priv.grf = (void *)GRF_BASE;
	sdram_priv.sgrf = (void *)SGRF_BASE;
	sdram_priv.phy = (void *)DDR_PHY_BASE;
	sdram_priv.pctl = (void *)DDR_PCTL_BASE;
	sdram_priv.standby = (void *)DDR_STANDBY_BASE;
	sdram_priv.pmu = (void *)PMU_BASS_ADDR;
	sdram_priv.service_msch = (void *)SERVICE_MSCH_BASE;
	params->idle_pd = PD_INFO(ddr_gd.head_info.g_sr_pd_idle);
	params->idle_sr = SR_INFO(ddr_gd.head_info.g_sr_pd_idle);
	params->ddr_2t_en = DDR_2T_INFO(ddr_gd.head_info.g_2t_info);
	params->stdby_idle = STANDBY_IDLE(ddr_gd.head_info.g_ch_info);

	rk3308_dram_init(&sdram_priv, params);

	modify_sdram_params(&sdram_priv, params);

	if (params->idle_pd != 0 && params->idle_sr != 0)
		enable_ddr_standby(&sdram_priv, params);
	printascii("OUT\n");

	return 0;
}

#else

struct dram_info {
	struct ram_info info;
	struct rk3308_grf *grf;
};

static int rk3308_dmc_probe(struct udevice *dev)
{
	struct dram_info *priv = dev_get_priv(dev);

	priv->grf = syscon_get_first_range(ROCKCHIP_SYSCON_GRF);
	priv->info.base = CFG_SYS_SDRAM_BASE;
	priv->info.size = rockchip_sdram_size((phys_addr_t)&priv->grf->os_reg2);

	return 0;
}

static int rk3308_dmc_get_info(struct udevice *dev, struct ram_info *info)
{
	struct dram_info *priv = dev_get_priv(dev);

	*info = priv->info;

	return 0;
}

static struct ram_ops rk3308_dmc_ops = {
	.get_info = rk3308_dmc_get_info,
};

static const struct udevice_id rk3308_dmc_ids[] = {
	{ .compatible = "rockchip,rk3308-dmc" },
	{ }
};

U_BOOT_DRIVER(dmc_rk3308) = {
	.name = "rockchip_rk3308_dmc",
	.id = UCLASS_RAM,
	.of_match = rk3308_dmc_ids,
	.ops = &rk3308_dmc_ops,
	.probe = rk3308_dmc_probe,
	.priv_auto = sizeof(struct dram_info),
};

#endif /* CONFIG_TPL_BUILD */
