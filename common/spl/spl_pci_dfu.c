// SPDX-License-Identifier: GPL-2.0+
/*
 * SPL boot over PCIe: act as a PCIe endpoint and wait until the root
 * complex has written the next image into BAR0.
 *
 * (C) Copyright 2016
 * Texas Instruments, <www.ti.com>
 */
#include <cpu_func.h>
#include <errno.h>
#include <pci_ep.h>
#include <spl.h>
#include <dm/uclass.h>
#include <linux/io.h>
#include <linux/printk.h>

/*
 * Macros define size of magic word and boot phase string
 * in bytes.
 */
#define MAGIC_WORD_SIZE 4
#define BOOT_PHASE_STRING_SIZE 63

int spl_pci_dfu(void)
{
	u32 offset, magic_word;
	volatile void *addr;
	struct udevice *dev;
	struct pci_bar bar;
	struct pci_ep_header hdr;
	uint fn = 0;
	int ret;
	char *bootphase;

	uclass_get_device_by_seq(UCLASS_PCI_EP, 0, &dev);
	if (!dev) {
		pr_err("Failed to get pci ep device\n");
		return -ENODEV;
	}

	hdr.deviceid = CONFIG_SPL_PCI_DFU_DEVICE_ID;
	hdr.vendorid = CONFIG_SPL_PCI_DFU_VENDOR_ID;
	hdr.baseclass_code = PCI_BASE_CLASS_MEMORY;
	hdr.subclass_code = PCI_CLASS_MEMORY_RAM & 0xff;

	ret = pci_ep_write_header(dev, fn, &hdr);
	if (ret) {
		pr_err("Failed to write header: %d\n", ret);
		return ret;
	}

	bar.barno = BAR_0;
	bar.phys_addr = (dma_addr_t)CONFIG_SPL_PCI_DFU_SPL_LOAD_FIT_ADDRESS;
	bar.flags = PCI_BASE_ADDRESS_SPACE_MEMORY |
			  PCI_BASE_ADDRESS_MEM_TYPE_32 |
			  PCI_BASE_ADDRESS_MEM_PREFETCH;

	bar.size = CONFIG_SPL_PCI_DFU_BAR_SIZE;

	ret = pci_ep_set_bar(dev, fn, &bar);
	if (ret) {
		pr_err("Failed to set bar: %d\n", ret);
		return ret;
	}

	ret = pci_ep_start(dev);
	if (ret) {
		pr_err("Failed to start ep: %d\n", ret);
		return ret;
	}

	addr = (void *)CONFIG_SPL_PCI_DFU_SPL_LOAD_FIT_ADDRESS;
	offset = CONFIG_SPL_PCI_DFU_BAR_SIZE - MAGIC_WORD_SIZE;

	if (sizeof(CONFIG_SPL_PCI_DFU_BOOT_PHASE) > BOOT_PHASE_STRING_SIZE) {
		pr_err("Not copying boot phase. String too long\n");
	} else {
		bootphase = (char *)(addr + CONFIG_SPL_PCI_DFU_BAR_SIZE -
				  (BOOT_PHASE_STRING_SIZE + MAGIC_WORD_SIZE + 1));
		strlcpy(bootphase, CONFIG_SPL_PCI_DFU_BOOT_PHASE,
			sizeof(CONFIG_SPL_PCI_DFU_BOOT_PHASE) + 1);
	}

	addr = addr + offset;
	magic_word = CONFIG_SPL_PCI_DFU_MAGIC_WORD;
	(*(int *)addr) = 0;
	flush_dcache_all();
	for (;;) {
		if (*(int *)addr == magic_word)
			break;
		invalidate_dcache_all();
	}

	return 0;
}
