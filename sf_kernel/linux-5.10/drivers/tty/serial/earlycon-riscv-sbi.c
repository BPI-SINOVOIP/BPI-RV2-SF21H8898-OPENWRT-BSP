// SPDX-License-Identifier: GPL-2.0
/*
 * RISC-V SBI based earlycon
 *
 * Copyright (C) 2018 Anup Patel <anup@brainfault.org>
 */
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/mm.h>
#include <asm/sbi.h>

#ifdef CONFIG_RISCV_SBI_V01
static void sbi_putc(struct uart_port *port, int c)
{
	sbi_console_putchar(c);
}

static void sbi_console_write(struct console *con,
			      const char *s, unsigned n)
{
	struct earlycon_device *dev = con->data;
	uart_console_write(&dev->port, s, n, sbi_putc);
}
#endif

static struct sbiret sbi_debug_console_write(unsigned long num_bytes,
					     unsigned long base_addr_lo,
					     unsigned long base_addr_hi)
{
	return sbi_ecall(SBI_EXT_DBCN, SBI_EXT_DBCN_CONSOLE_WRITE, num_bytes,
			 base_addr_lo, base_addr_hi, 0, 0, 0);
}

static long sbi_console_write_buf(phys_addr_t phys, unsigned n)
{
	struct sbiret ret;

	do {
		ret = sbi_debug_console_write(n, phys, 0);
		if (ret.error)
			return ret.error;

		n -= ret.value;
		phys += ret.value;
	} while (n);

	return 0;
}

static void sbi_console_write_v2(struct console *con,
				 const char *s, unsigned n)
{
	if (!is_vmalloc_or_module_addr(s)) {
		sbi_console_write_buf(__pa(s), n);
	} else {
		do {
			unsigned long offset = offset_in_page(s);
			unsigned long nbytes;
			struct page *page;
			long ret;

			page = vmalloc_to_page(s);
			if (!page)
				return;

			nbytes = min_t(unsigned long, n, PAGE_SIZE - offset);
			ret = sbi_console_write_buf(page_to_phys(page) + offset, nbytes);
			if (ret)
				return;

			n -= nbytes;
			s += nbytes;
		} while (n);
	}
}

static int __init early_sbi_setup(struct earlycon_device *device,
				  const char *opt)
{
	if (sbi_probe_extension(SBI_EXT_DBCN) > 0) {
		device->con->write = sbi_console_write_v2;
		return 0;
	}
#ifdef CONFIG_RISCV_SBI_V01
	device->con->write = sbi_console_write;
	return 0;
#else
	return -ENODEV;
#endif
}
EARLYCON_DECLARE(sbi, early_sbi_setup);
