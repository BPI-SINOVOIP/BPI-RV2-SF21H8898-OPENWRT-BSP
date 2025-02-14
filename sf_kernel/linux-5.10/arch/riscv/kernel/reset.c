// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <linux/reboot.h>
#include <linux/pm.h>
#include <asm/sbi.h>

static void default_power_off(void)
{
	sbi_system_reset(SBI_SRST_RESET_TYPE_SHUTDOWN, SBI_SRST_RESET_REASON_NONE);
	while (1)
		wait_for_interrupt();
}

void (*pm_power_off)(void) = default_power_off;
EXPORT_SYMBOL(pm_power_off);

void machine_restart(char *cmd)
{
	do_kernel_restart(cmd);
	sbi_system_reset((reboot_mode == REBOOT_WARM || reboot_mode == REBOOT_SOFT) ?
			 SBI_SRST_RESET_TYPE_WARM_REBOOT :
			 SBI_SRST_RESET_TYPE_COLD_REBOOT,
			 SBI_SRST_RESET_REASON_NONE);
	while (1)
		wait_for_interrupt();
}

void machine_halt(void)
{
	pm_power_off();
}

void machine_power_off(void)
{
	pm_power_off();
}
