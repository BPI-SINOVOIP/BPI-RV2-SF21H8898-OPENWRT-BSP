# SPDX-License-Identifier: GPL-2.0-only
lib-y			+= delay.o
lib-y			+= memcpy.o
lib-y			+= memset.o
lib-$(CONFIG_MMU)	+= uaccess.o
lib-$(CONFIG_64BIT)	+= tishift.o
lib-$(CONFIG_RISCV_ISA_B)	+= crc32.o

obj-$(CONFIG_FUNCTION_ERROR_INJECTION) += error-inject.o
