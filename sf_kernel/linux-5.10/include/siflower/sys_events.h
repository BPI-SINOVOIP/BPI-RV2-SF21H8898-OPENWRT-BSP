#ifdef CONFIG_SF_REBOOT_RECORD
#ifndef _SYS_EVENTS_H
#define _SYS_EVENTS_H

#include <linux/bits.h>
#include <linux/bitfield.h>
#include <linux/export.h>
#include <linux/seq_file.h>
#include <asm/io.h>

#define SYSTEM_EVENT 		(0x0CE00020)
#define SYSTEM_EVENT_RECORD (0x0CE00014)
#define HARD_REBOOT_OFF 	(0x4)
#define SOFT_REC_MASK GENMASK(7, 0)

extern int soft_record;

typedef enum {
	CLEAR = 0,
	UNKNOWN,
	NP,
	OOM,
	UI,
	BM,
	AF,
	MA,
	ETH_BUG,
	WIFI_BUG,
	//INVAILD = 256,
} SOFT_E;

extern void set_soft_event(char events[256][128]);
extern void set_hard_event(char events[64][128]);
extern void set_soft_record(int val);
extern void last_reboot_event(void __iomem *sys, struct seq_file *m);

#define SF_BUG_ON(type, condition)                                             \
	do {                                                                   \
		if (unlikely(condition)) {                                     \
			set_soft_record(type);                                 \
			BUG();                                                 \
		}                                                              \
	} while (0)

#endif
#endif