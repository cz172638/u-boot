// SPDX-License-Identifier: GPL-2.0+
/*
 * Rockchip RK3399 PCIe Endpoint controller driver
 *
 * Based on Linux kernel driver and TI K3 U-Boot implementation
 * The RK3399 uses Cadence PCIe IP with Rockchip-specific wrapper
 *
 * Copyright (C) 2025 Jiri Kastner
 * Author: Jiri Kastner <cz172638@gmail.com>
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <pci_ep.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <clk.h>
#include <reset.h>
#include <phy.h>
#include <syscon.h>
#include <regmap.h>

#include "pcie-cadence.h"

/* Rockchip PCIe wrapper registers */
#define PCIE_CLIENT_BASE			0x0
#define PCIE_CLIENT_CONFIG			(PCIE_CLIENT_BASE + 0x00)
#define   PCIE_CLIENT_CONF_ENABLE		BIT(0)
#define   PCIE_CLIENT_CONF_LANE_NUM(x)		(((x) & 0x3) << 4)
#define   PCIE_CLIENT_LINK_TRAIN_ENABLE		BIT(3)
#define   PCIE_CLIENT_MODE_RC			BIT(6)
#define   PCIE_CLIENT_MODE_EP			0
#define   PCIE_CLIENT_GEN_SEL(x)		(((x) & 0x3) << 7)

#define PCIE_CLIENT_DEBUG_OUT_0			(PCIE_CLIENT_BASE + 0x3c)
#define   PCIE_CLIENT_DEBUG_LTSSM_MASK		GENMASK(5, 0)
#define   PCIE_CLIENT_DEBUG_LTSSM_L0		0x11

#define PCIE_CLIENT_BASIC_STATUS1		(PCIE_CLIENT_BASE + 0x48)
#define   PCIE_CLIENT_LINK_STATUS_UP		BIT(20)
#define   PCIE_CLIENT_LINK_STATUS_MASK		GENMASK(21, 20)

#define PCIE_CLIENT_INT_MASK			(PCIE_CLIENT_BASE + 0x4c)
#define PCIE_CLIENT_INT_STATUS			(PCIE_CLIENT_BASE + 0x50)
#define   PCIE_CLIENT_INT_LEGACY_DONE		BIT(0)
#define   PCIE_CLIENT_INT_MSG			BIT(1)
#define   PCIE_CLIENT_INT_HOT_RST		BIT(2)
#define   PCIE_CLIENT_INT_DPA			BIT(3)
#define   PCIE_CLIENT_INT_FATAL_ERR		BIT(4)
#define   PCIE_CLIENT_INT_NFATAL_ERR		BIT(5)
#define   PCIE_CLIENT_INT_CORR_ERR		BIT(6)
#define   PCIE_CLIENT_INT_INTD			BIT(7)
#define   PCIE_CLIENT_INT_INTC			BIT(8)
#define   PCIE_CLIENT_INT_INTB			BIT(9)
#define   PCIE_CLIENT_INT_INTA			BIT(10)
#define   PCIE_CLIENT_INT_LOCAL			BIT(11)
#define   PCIE_CLIENT_INT_UDMA			BIT(12)
#define   PCIE_CLIENT_INT_PWR_STCG		BIT(14)
#define   PCIE_CLIENT_INT_HOT_PLUG		BIT(15)
#define   PCIE_CLIENT_INT_PHY			BIT(16)

#define PCIE_CLIENT_LEGACY_INT_CTRL		(PCIE_CLIENT_BASE + 0x0c)
#define   PCIE_CLIENT_LEGACY_INT_SET(x)		BIT((x) + 24)
#define   PCIE_CLIENT_LEGACY_INT_CLR(x)		BIT((x) + 16)

#define PCIE_CORE_CTRL_MGMT_BASE		0x900000
#define PCIE_CORE_INT_MASK			(PCIE_CORE_CTRL_MGMT_BASE + 0x0c)
#define   PCIE_CORE_INT				0xFFFFFFFF

/* Cadence register offsets */
#define ROCKCHIP_PCIE_EP_FUNC_BASE(fn) \
	(PCIE_CORE_CTRL_MGMT_BASE + (fn) * 0x1000)
#define ROCKCHIP_PCIE_EP_VIRT_FUNC_BASE(fn) \
	(PCIE_CORE_CTRL_MGMT_BASE + 0x10000 + (fn) * 0x1000)

/* Address translation unit (ATU) */
#define PCIE_RC_EP_ATR_OB_REGIONS_1_32		0x800000
#define PCIE_RC_EP_ATR_OB_REGIONS_33		0x900018

#define ROCKCHIP_PCIE_AT_OB_REGION_DESC0(r) \
	(PCIE_RC_EP_ATR_OB_REGIONS_1_32 + 0x0008 + ((r) & 0x1f) * 0x0020)
#define ROCKCHIP_PCIE_AT_OB_REGION_DESC1(r) \
	(PCIE_RC_EP_ATR_OB_REGIONS_1_32 + 0x000c + ((r) & 0x1f) * 0x0020)
#define ROCKCHIP_PCIE_AT_OB_REGION_DESC2(r) \
	(PCIE_RC_EP_ATR_OB_REGIONS_1_32 + 0x0010 + ((r) & 0x1f) * 0x0020)

/* BAR configuration */
#define ROCKCHIP_PCIE_CORE_EP_FUNC_BAR_CFG0(fn) \
	(ROCKCHIP_PCIE_EP_FUNC_BASE(fn) + 0x0240)
#define ROCKCHIP_PCIE_CORE_EP_FUNC_BAR_CFG1(fn) \
	(ROCKCHIP_PCIE_EP_FUNC_BASE(fn) + 0x0244)

#define ROCKCHIP_PCIE_EP_MSI_CTRL_REG		0xB0
#define   ROCKCHIP_PCIE_EP_MSI_CTRL_MMC_OFFSET	17
#define   ROCKCHIP_PCIE_EP_MSI_CTRL_MMC_MASK	GENMASK(19, 17)
#define   ROCKCHIP_PCIE_EP_MSI_CTRL_MME_OFFSET	20
#define   ROCKCHIP_PCIE_EP_MSI_CTRL_MME_MASK	GENMASK(22, 20)
#define   ROCKCHIP_PCIE_EP_MSI_CTRL_ME		BIT(16)
#define   ROCKCHIP_PCIE_EP_MSI_CTRL_MASK_MSI	BIT(24)

/* Maximum number of BARs */
#define ROCKCHIP_PCIE_EP_MAX_BARS		6

/* Maximum number of outbound regions */
#define ROCKCHIP_PCIE_MAX_OB_REGIONS		32

/**
 * struct rockchip_pcie_ep - RK3399 PCIe endpoint controller
 * @cdns_pcie: Base Cadence PCIe structure
 * @max_regions: Maximum number of regions supported
 * @ob_region_map: Bitmap of outbound regions in use
 * @ob_addr: Array of outbound region addresses
 * @irq_cpu_addr: IRQ CPU address
 * @irq_pci_addr: IRQ PCI address
 * @irq_pci_fn: IRQ PCI function
 */
struct rockchip_pcie_ep {
	struct cdns_pcie cdns_pcie;
	u32 max_regions;
	unsigned long ob_region_map;
	phys_addr_t *ob_addr;
	phys_addr_t irq_cpu_addr;
	u64 irq_pci_addr;
	u8 irq_pci_fn;
};

static inline u32 rockchip_pcie_readl(struct rockchip_pcie_ep *ep, u32 reg)
{
	return readl(ep->cdns_pcie.reg_base + reg);
}

static inline void rockchip_pcie_writel(struct rockchip_pcie_ep *ep,
					u32 val, u32 reg)
{
	writel(val, ep->cdns_pcie.reg_base + reg);
}

/**
 * rockchip_pcie_ep_wait_for_pll_locked() - Wait for PHY PLLs to lock
 * @ep: Pointer to the RK3399 PCIe EP instance
 *
 * Wait for the PCIe PHY PLLs to lock with timeout.
 * This is critical before accessing certain registers.
 *
 * Return: 0 on success, negative errno on failure
 */
static int rockchip_pcie_ep_wait_for_pll_locked(struct rockchip_pcie_ep *ep)
{
	u32 val;
	int timeout = 1000; /* 1 second timeout */

	while (timeout--) {
		val = rockchip_pcie_readl(ep, PCIE_CLIENT_BASIC_STATUS1);
		if ((val & PCIE_CLIENT_LINK_STATUS_MASK) ==
		    PCIE_CLIENT_LINK_STATUS_UP) {
			return 0;
		}
		udelay(1000);
	}

	printf("RK3399 PCIe EP: PHY PLL lock timeout\n");
	return -ETIMEDOUT;
}

/**
 * rockchip_pcie_ep_assert_pci_conf_enable() - Assert PCIe configuration enable
 * @ep: Pointer to the RK3399 PCIe EP instance
 *
 * Enables the PCIe configuration after probe, allowing the endpoint
 * to be configured by the host.
 */
static void rockchip_pcie_ep_assert_pci_conf_enable(struct rockchip_pcie_ep *ep)
{
	u32 val;

	val = rockchip_pcie_readl(ep, PCIE_CLIENT_CONFIG);
	val |= PCIE_CLIENT_CONF_ENABLE;
	rockchip_pcie_writel(ep, val, PCIE_CLIENT_CONFIG);
}

/**
 * rockchip_ob_region() - Get outbound region number from address
 * @phys_addr: Physical address
 *
 * Return: Region number (0-31)
 */
static inline int rockchip_ob_region(phys_addr_t phys_addr)
{
	return (phys_addr >> 20) & 0x1f;
}

/**
 * rockchip_pcie_prog_ep_ob_atu() - Program outbound address translation
 * @ep: Pointer to the RK3399 PCIe EP instance
 * @fn: Function number
 * @r: Region number
 * @type: Transaction type
 * @cpu_addr: CPU address
 * @pci_addr: PCI address
 * @size: Size of the region
 *
 * Configure the outbound address translation unit (ATU) for endpoint mode.
 * This maps CPU memory space to PCIe address space.
 *
 * Return: 0 on success, negative errno on failure
 */
static int rockchip_pcie_prog_ep_ob_atu(struct rockchip_pcie_ep *ep, u8 fn,
					u32 r, u32 type, u64 cpu_addr,
					u64 pci_addr, u32 size)
{
	u32 num_pass_bits = ilog2(size);
	u32 desc0, desc1, desc2;

	if (r >= ep->max_regions)
		return -EINVAL;

	if (size < SZ_128K)
		return -EINVAL;

	/* Description 0: Type and function number */
	desc0 = type | (fn << 8);

	/* Description 1: PCIe address lower 32 bits */
	desc1 = (u32)(pci_addr & GENMASK_ULL(31, 8));

	/* Description 2: PCIe address upper 32 bits and pass bits */
	desc2 = (u32)(pci_addr >> 32) | ((num_pass_bits - 1) & 0x3f);

	rockchip_pcie_writel(ep, desc0, ROCKCHIP_PCIE_AT_OB_REGION_DESC0(r));
	rockchip_pcie_writel(ep, desc1, ROCKCHIP_PCIE_AT_OB_REGION_DESC1(r));
	rockchip_pcie_writel(ep, desc2, ROCKCHIP_PCIE_AT_OB_REGION_DESC2(r));

	return 0;
}

/**
 * rockchip_pcie_ep_write_header() - Write PCIe configuration space header
 * @epc: PCI endpoint controller
 * @fn: Function number
 * @vfn: Virtual function number
 * @header: Configuration space header data
 *
 * Write the standard PCI configuration space header fields.
 *
 * Return: 0 on success
 */
static int rockchip_pcie_ep_write_header(struct pci_epc *epc, u8 fn, u8 vfn,
					 struct pci_epf_header *header)
{
	struct rockchip_pcie_ep *ep = epc_get_drvdata(epc);
	u32 reg;

	if (fn > 0) {
		dev_err(epc->dev, "RK3399 PCIe EP only supports function 0\n");
		return -EINVAL;
	}

	reg = ROCKCHIP_PCIE_EP_FUNC_BASE(fn);

	/* Write vendor ID and device ID */
	cdns_pcie_writel(&ep->cdns_pcie, reg + PCI_VENDOR_ID,
			 header->vendorid | (header->deviceid << 16));

	/* Write subsystem vendor ID and subsystem device ID */
	cdns_pcie_writel(&ep->cdns_pcie, reg + PCI_SUBSYSTEM_VENDOR_ID,
			 header->subsys_vendor_id |
			 (header->subsys_id << 16));

	/* Write class code and revision ID */
	cdns_pcie_writel(&ep->cdns_pcie, reg + PCI_CLASS_REVISION,
			 header->revid | (header->baseclass_code << 24) |
			 (header->subclass_code << 16) |
			 (header->progif_code << 8));

	/* Write cache line size */
	cdns_pcie_writeb(&ep->cdns_pcie, reg + PCI_CACHE_LINE_SIZE,
			 header->cache_line_size);

	/* Write interrupt pin */
	cdns_pcie_writeb(&ep->cdns_pcie, reg + PCI_INTERRUPT_PIN,
			 header->interrupt_pin);

	return 0;
}

/**
 * rockchip_pcie_ep_set_bar() - Configure BAR
 * @epc: PCI endpoint controller
 * @fn: Function number
 * @vfn: Virtual function number
 * @bar: BAR configuration
 *
 * Configure a Base Address Register (BAR) for the endpoint.
 *
 * Return: 0 on success, negative errno on failure
 */
static int rockchip_pcie_ep_set_bar(struct pci_epc *epc, u8 fn, u8 vfn,
				    struct pci_epf_bar *bar)
{
	struct rockchip_pcie_ep *ep = epc_get_drvdata(epc);
	u32 bar_idx = bar->barno;
	u64 bar_phys = bar->phys_addr;
	enum pci_barno barno = bar->barno;
	u32 flags = bar->flags;
	u32 addr0, addr1, reg, cfg, ctrl;
	u64 sz;

	if (fn > 0) {
		dev_err(epc->dev, "RK3399 PCIe EP only supports function 0\n");
		return -EINVAL;
	}

	if (bar_idx > ROCKCHIP_PCIE_EP_MAX_BARS)
		return -EINVAL;

	/* Get BAR size and round up to power of 2 */
	sz = bar->size;
	if (sz < SZ_128)
		return -EINVAL;

	sz = 1ULL << fls64(sz - 1);

	/* Calculate aperture size (log2(size) - 7, since 128B = 0) */
	u32 aperture = ilog2(sz) - 7;

	/* Determine BAR control based on flags */
	if ((flags & PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_IO) {
		ctrl = 0; /* I/O BAR */
	} else {
		bool is_64bit = (sz > SZ_2G) ||
				(flags & PCI_BASE_ADDRESS_MEM_TYPE_64);
		bool is_prefetch = !!(flags & PCI_BASE_ADDRESS_MEM_PREFETCH);

		if (is_64bit && (barno & 1))
			return -EINVAL;

		if (is_64bit && is_prefetch)
			ctrl = 4; /* 64-bit prefetchable */
		else if (is_prefetch)
			ctrl = 3; /* 32-bit prefetchable */
		else if (is_64bit)
			ctrl = 2; /* 64-bit non-prefetchable */
		else
			ctrl = 1; /* 32-bit non-prefetchable */
	}

	/* Write BAR configuration */
	reg = ROCKCHIP_PCIE_CORE_EP_FUNC_BAR_CFG0(fn) + bar_idx * 4;
	cfg = (ctrl << 28) | (aperture << 0);
	rockchip_pcie_writel(ep, cfg, reg);

	/* Configure inbound address translation */
	addr0 = lower_32_bits(bar_phys);
	addr1 = upper_32_bits(bar_phys);

	rockchip_pcie_writel(ep, addr0,
			     PCIE_CORE_CTRL_MGMT_BASE + 0x0240 +
			     fn * 0x1000 + bar_idx * 8);
	rockchip_pcie_writel(ep, addr1,
			     PCIE_CORE_CTRL_MGMT_BASE + 0x0244 +
			     fn * 0x1000 + bar_idx * 8);

	return 0;
}

/**
 * rockchip_pcie_ep_set_msi() - Configure MSI capability
 * @epc: PCI endpoint controller
 * @fn: Function number
 * @vfn: Virtual function number
 * @mmc: Multiple message capable
 *
 * Configure the MSI capability for the endpoint.
 *
 * Return: 0 on success, negative errno on failure
 */
static int rockchip_pcie_ep_set_msi(struct pci_epc *epc, u8 fn, u8 vfn,
				    u8 mmc)
{
	struct rockchip_pcie_ep *ep = epc_get_drvdata(epc);
	u32 flags;

	if (fn > 0) {
		dev_err(epc->dev, "RK3399 PCIe EP only supports function 0\n");
		return -EINVAL;
	}

	flags = cdns_pcie_readl(&ep->cdns_pcie,
				ROCKCHIP_PCIE_EP_FUNC_BASE(fn) +
				ROCKCHIP_PCIE_EP_MSI_CTRL_REG);

	flags &= ~ROCKCHIP_PCIE_EP_MSI_CTRL_MMC_MASK;
	flags |= ((mmc << ROCKCHIP_PCIE_EP_MSI_CTRL_MMC_OFFSET) &
		  ROCKCHIP_PCIE_EP_MSI_CTRL_MMC_MASK);

	cdns_pcie_writel(&ep->cdns_pcie, flags,
			 ROCKCHIP_PCIE_EP_FUNC_BASE(fn) +
			 ROCKCHIP_PCIE_EP_MSI_CTRL_REG);

	return 0;
}

/**
 * rockchip_pcie_ep_get_msi() - Get MSI configuration
 * @epc: PCI endpoint controller
 * @fn: Function number
 * @vfn: Virtual function number
 *
 * Get the current MSI configuration (multiple message enable).
 *
 * Return: MSI MME value, or negative errno on failure
 */
static int rockchip_pcie_ep_get_msi(struct pci_epc *epc, u8 fn, u8 vfn)
{
	struct rockchip_pcie_ep *ep = epc_get_drvdata(epc);
	u32 flags;

	if (fn > 0) {
		dev_err(epc->dev, "RK3399 PCIe EP only supports function 0\n");
		return -EINVAL;
	}

	flags = cdns_pcie_readl(&ep->cdns_pcie,
				ROCKCHIP_PCIE_EP_FUNC_BASE(fn) +
				ROCKCHIP_PCIE_EP_MSI_CTRL_REG);

	return ((flags & ROCKCHIP_PCIE_EP_MSI_CTRL_MME_MASK) >>
		ROCKCHIP_PCIE_EP_MSI_CTRL_MME_OFFSET);
}

/**
 * rockchip_pcie_ep_assert_intx() - Assert legacy interrupt
 * @ep: Pointer to the RK3399 PCIe EP instance
 * @fn: Function number
 * @intx: Interrupt pin (INTA=0, INTB=1, INTC=2, INTD=3)
 * @is_asserted: true to assert, false to deassert
 *
 * Generate or clear a legacy INTx interrupt.
 */
static void rockchip_pcie_ep_assert_intx(struct rockchip_pcie_ep *ep,
					 u8 fn, u8 intx, bool is_asserted)
{
	u32 val;

	if (intx > 3)
		return;

	val = is_asserted ? PCIE_CLIENT_LEGACY_INT_SET(intx) :
			    PCIE_CLIENT_LEGACY_INT_CLR(intx);

	rockchip_pcie_writel(ep, val, PCIE_CLIENT_LEGACY_INT_CTRL);
}

/**
 * rockchip_pcie_ep_raise_irq() - Raise interrupt
 * @epc: PCI endpoint controller
 * @fn: Function number
 * @vfn: Virtual function number
 * @type: Interrupt type
 * @interrupt_num: Interrupt number
 *
 * Raise a specified interrupt type.
 *
 * Return: 0 on success, negative errno on failure
 */
static int rockchip_pcie_ep_raise_irq(struct pci_epc *epc, u8 fn, u8 vfn,
				      enum pci_epc_irq_type type,
				      u16 interrupt_num)
{
	struct rockchip_pcie_ep *ep = epc_get_drvdata(epc);

	if (fn > 0) {
		dev_err(epc->dev, "RK3399 PCIe EP only supports function 0\n");
		return -EINVAL;
	}

	switch (type) {
	case PCI_EPC_IRQ_LEGACY:
		if (interrupt_num > 3)
			return -EINVAL;
		rockchip_pcie_ep_assert_intx(ep, fn, interrupt_num, true);
		/* Deassert after a short delay */
		udelay(1);
		rockchip_pcie_ep_assert_intx(ep, fn, interrupt_num, false);
		break;

	case PCI_EPC_IRQ_MSI:
		/* MSI is handled by the Cadence core */
		return cdns_pcie_ep_raise_irq(&ep->cdns_pcie, fn,
					      type, interrupt_num);

	default:
		return -EINVAL;
	}

	return 0;
}

/**
 * rockchip_pcie_ep_start() - Start the PCIe link
 * @epc: PCI endpoint controller
 *
 * Start the PCIe link and enable the controller.
 *
 * Return: 0 on success
 */
static int rockchip_pcie_ep_start(struct pci_epc *epc)
{
	struct rockchip_pcie_ep *ep = epc_get_drvdata(epc);
	u32 val;

	/* Enable link training */
	val = rockchip_pcie_readl(ep, PCIE_CLIENT_CONFIG);
	val |= PCIE_CLIENT_LINK_TRAIN_ENABLE;
	rockchip_pcie_writel(ep, val, PCIE_CLIENT_CONFIG);

	return 0;
}

/**
 * rockchip_pcie_ep_stop() - Stop the PCIe link
 * @epc: PCI endpoint controller
 *
 * Stop the PCIe link and disable the controller.
 */
static void rockchip_pcie_ep_stop(struct pci_epc *epc)
{
	struct rockchip_pcie_ep *ep = epc_get_drvdata(epc);
	u32 val;

	/* Disable link training */
	val = rockchip_pcie_readl(ep, PCIE_CLIENT_CONFIG);
	val &= ~PCIE_CLIENT_LINK_TRAIN_ENABLE;
	rockchip_pcie_writel(ep, val, PCIE_CLIENT_CONFIG);
}

static const struct pci_epc_ops rockchip_pcie_epc_ops = {
	.write_header	= rockchip_pcie_ep_write_header,
	.set_bar	= rockchip_pcie_ep_set_bar,
	.set_msi	= rockchip_pcie_ep_set_msi,
	.get_msi	= rockchip_pcie_ep_get_msi,
	.raise_irq	= rockchip_pcie_ep_raise_irq,
	.start		= rockchip_pcie_ep_start,
	.stop		= rockchip_pcie_ep_stop,
};

/**
 * rockchip_pcie_ep_enable_interrupts() - Enable endpoint interrupts
 * @ep: Pointer to the RK3399 PCIe EP instance
 *
 * Enable all relevant endpoint interrupts for proper operation.
 */
static void rockchip_pcie_ep_enable_interrupts(struct rockchip_pcie_ep *ep)
{
	u32 client_mask = PCIE_CLIENT_INT_LEGACY_DONE |
			  PCIE_CLIENT_INT_MSG | PCIE_CLIENT_INT_DPA |
			  PCIE_CLIENT_INT_FATAL_ERR |
			  PCIE_CLIENT_INT_NFATAL_ERR |
			  PCIE_CLIENT_INT_CORR_ERR |
			  PCIE_CLIENT_INT_LOCAL | PCIE_CLIENT_INT_UDMA |
			  PCIE_CLIENT_INT_PWR_STCG;

	/* Write-1-to-clear for upper 16 bits, set lower 16 to enable */
	rockchip_pcie_writel(ep, (client_mask << 16) | (~client_mask & 0xFFFF),
			     PCIE_CLIENT_INT_MASK);

	/* Unmask core interrupts */
	rockchip_pcie_writel(ep, (u32)(~PCIE_CORE_INT), PCIE_CORE_INT_MASK);
}

/**
 * rockchip_pcie_ep_init_hw() - Initialize hardware
 * @ep: Pointer to the RK3399 PCIe EP instance
 *
 * Initialize the RK3399 PCIe endpoint hardware.
 *
 * Return: 0 on success, negative errno on failure
 */
static int rockchip_pcie_ep_init_hw(struct rockchip_pcie_ep *ep)
{
	u32 val;
	int ret;

	/* Configure as endpoint */
	val = PCIE_CLIENT_GEN_SEL(2) |  /* PCIe Gen2 */
	      PCIE_CLIENT_CONF_LANE_NUM(0) | /* x1 mode for now */
	      PCIE_CLIENT_MODE_EP;       /* Endpoint mode */
	rockchip_pcie_writel(ep, val, PCIE_CLIENT_CONFIG);

	/* Wait for PHY PLLs to lock */
	ret = rockchip_pcie_ep_wait_for_pll_locked(ep);
	if (ret)
		return ret;

	/* Assert configuration enable bit */
	rockchip_pcie_ep_assert_pci_conf_enable(ep);

	/* Enable interrupts */
	rockchip_pcie_ep_enable_interrupts(ep);

	/* Initialize max regions */
	ep->max_regions = ROCKCHIP_PCIE_MAX_OB_REGIONS;

	return 0;
}

/**
 * rockchip_pcie_ep_probe() - Probe the RK3399 PCIe EP device
 * @dev: U-Boot device
 *
 * Probe and initialize the RK3399 PCIe endpoint controller.
 *
 * Return: 0 on success, negative errno on failure
 */
static int rockchip_pcie_ep_probe(struct udevice *dev)
{
	struct rockchip_pcie_ep *ep = dev_get_priv(dev);
	struct pci_epc *epc;
	struct clk clk;
	struct reset_ctl reset;
	struct phy phy;
	int ret;

	/* Get register base */
	ep->cdns_pcie.reg_base = dev_read_addr_ptr(dev);
	if (!ep->cdns_pcie.reg_base)
		return -EINVAL;

	/* Get and enable clocks */
	ret = clk_get_by_name(dev, "aclk", &clk);
	if (!ret) {
		ret = clk_enable(&clk);
		if (ret) {
			dev_err(dev, "Failed to enable aclk clock\n");
			return ret;
		}
	}

	ret = clk_get_by_name(dev, "aclk-perf", &clk);
	if (!ret)
		clk_enable(&clk);

	ret = clk_get_by_name(dev, "pclk", &clk);
	if (!ret)
		clk_enable(&clk);

	ret = clk_get_by_name(dev, "pm", &clk);
	if (!ret)
		clk_enable(&clk);

	/* Get and deassert resets */
	ret = reset_get_by_name(dev, "core", &reset);
	if (!ret)
		reset_deassert(&reset);

	ret = reset_get_by_name(dev, "mgmt", &reset);
	if (!ret)
		reset_deassert(&reset);

	ret = reset_get_by_name(dev, "mgmt-sticky", &reset);
	if (!ret)
		reset_deassert(&reset);

	ret = reset_get_by_name(dev, "pipe", &reset);
	if (!ret)
		reset_deassert(&reset);

	/* Initialize PHY */
	ret = generic_phy_get_by_name(dev, "pcie-phy", &phy);
	if (!ret) {
		ret = generic_phy_init(&phy);
		if (ret) {
			dev_err(dev, "Failed to init PHY\n");
			return ret;
		}

		ret = generic_phy_power_on(&phy);
		if (ret) {
			dev_err(dev, "Failed to power on PHY\n");
			return ret;
		}
	}

	/* Initialize hardware */
	ret = rockchip_pcie_ep_init_hw(ep);
	if (ret) {
		dev_err(dev, "Failed to initialize hardware\n");
		return ret;
	}

	/* Create endpoint controller */
	epc = devm_pci_epc_create(dev, &rockchip_pcie_epc_ops);
	if (!epc) {
		dev_err(dev, "Failed to create EPC device\n");
		return -ENOMEM;
	}

	epc_set_drvdata(epc, ep);

	/* Set endpoint capabilities */
	epc->max_functions = 1;
	epc->max_vfs = 0;

	dev_info(dev, "RK3399 PCIe EP controller initialized\n");

	return 0;
}

static const struct udevice_id rockchip_pcie_ep_ids[] = {
	{ .compatible = "rockchip,rk3399-pcie-ep" },
	{ }
};

U_BOOT_DRIVER(rockchip_pcie_ep) = {
	.name		= "rockchip_rk3399_pcie_ep",
	.id		= UCLASS_PCI_EP,
	.of_match	= rockchip_pcie_ep_ids,
	.probe		= rockchip_pcie_ep_probe,
	.priv_auto	= sizeof(struct rockchip_pcie_ep),
};
