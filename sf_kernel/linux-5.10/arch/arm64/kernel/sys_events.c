
#include <siflower/sys_events.h>

int soft_record = 0;
const char *soft_events[] = {
	[CLEAR] = "",
	[UNKNOWN] = "Unknown Reason",
	[NP] = "Kernel panic - NULL pointer",
	[OOM] = "Kernel panic - Out of memory",
	[UI] = "Kernel panic - Undefined instruction",
	[BM] = "Kernel panic - Bad Mode",
	[AF] = "Kernel panic - Assert Fail",
	[MA] = "Kernel panic - Illegal memory access",
	[ETH_BUG] = "Assert Fail - BUG ON - In sf_eth module",
	[WIFI_BUG] = "Assert Fail - BUG ON - In sf_wifi module"
};
const char *hard_events[] = { [0] = "Power Down", [63] = "Watchdog Reset" };

void set_soft_record(int val)
{
	soft_record = val;
}
EXPORT_SYMBOL(set_soft_record);

int get_hard_index(int index)
{
	int temp = 0;
	while (!(index & 0xf)) {
		temp += 4;
		index >>= 4;
	}
	while (!(index & 0x1)) {
		temp++;
		index >>= 1;
	}
	return temp;
}

void last_reboot_event(void __iomem *sys, struct seq_file *m)
{
	int index = readl(sys + HARD_REBOOT_OFF);
	if (index) {
		index = get_hard_index(index);
		goto show_hard_event;
	} else {
		index = readl(sys + HARD_REBOOT_OFF * 2);
		if (index) {
			index = get_hard_index(index) + 32;
			goto show_hard_event;
		} else {
			index = readl(sys);
			if (!index) {
				goto show_hard_event;
			}
			if (m) {
				seq_printf(m,
					   "Last time reboot: SOFT Reboot %s\n",
					   soft_events[index]);
			} else
				printk("Last time reboot: SOFT Reboot %s\n",
				       soft_events[index]);
			return;
		}
	}
show_hard_event:
	if (m) {
		seq_printf(m, "Last time reboot: HARD Reboot %s\n",
			   hard_events[index]);
	} else
		printk("Last time reboot: HARD Reboot %s\n",
		       hard_events[index]);
}