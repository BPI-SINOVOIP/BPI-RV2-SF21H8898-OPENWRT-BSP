/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_RISCV_EMERGENCY_RESTART_H
#define _ASM_RISCV_EMERGENCY_RESTART_H

#include <linux/reboot.h>
#include <asm/sbi.h>

static inline void machine_emergency_restart(void)
{
	do_kernel_restart(NULL);
	sbi_system_reset((reboot_mode == REBOOT_WARM || reboot_mode == REBOOT_SOFT) ?
			SBI_SRST_RESET_TYPE_WARM_REBOOT :
			SBI_SRST_RESET_TYPE_COLD_REBOOT,
			SBI_SRST_RESET_REASON_SYS_FAILURE);
	while (1)
		wait_for_interrupt();
}

#endif /* _ASM_RISCV_EMERGENCY_RESTART_H */
