// SPDX-License-Identifier: GPL-2.0+
/*
 * RK3308 TPL: bring up DRAM and return to the BootROM, which then loads
 * the next stage (SPL) into DRAM.
 *
 * Modelled on px30-board-tpl.c.
 */

#include <debug_uart.h>
#include <init.h>
#include <asm/arch-rockchip/bootrom.h>
#include <asm/arch-rockchip/sdram_rk3308.h>
#include <asm/arch-rockchip/timer.h>

void board_init_f(ulong dummy)
{
	int ret;

#if defined(CONFIG_DEBUG_UART) && defined(CONFIG_TPL_SERIAL)
	debug_uart_init();
#if CONFIG_TPL_BANNER_PRINT
	printascii("U-Boot TPL board init\n");
#endif
#endif

	rockchip_stimer_init();

	ret = sdram_init();
	if (ret)
		printascii("sdram_init failed\n");

	/* return to maskrom */
	back_to_bootrom(BROM_BOOT_NEXTSTAGE);
}
