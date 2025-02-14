// SPDX-License-Identifier: GPL-2.0
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/io.h>
#include <siflower/sys_events.h>

static int last_reboot_proc_show(struct seq_file *m, void *v)
{
	void __iomem *sys = ioremap(SYSTEM_EVENT, 0x10);
	last_reboot_event(sys, m);
	iounmap(sys);
	return 0;
}

static int __init proc_last_reboot_init(void)
{
	struct proc_dir_entry *e;
	e = proc_create_single("last_reboot", 0, NULL, last_reboot_proc_show);
	return 0;
}
fs_initcall(proc_last_reboot_init);