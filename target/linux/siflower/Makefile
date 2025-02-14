# SPDX-License-Identifier: GPL-2.0-only

include $(TOPDIR)/rules.mk

BOARD:=siflower
BOARDNAME:=SiFlower SoCs
SUBTARGETS:=sf21h8898-fullmask
FEATURES:=fpu pci usb usb_gadget cpiogz gpio ramdisk squashfs targz nand separate_ramdisk

KERNEL_PATCHVER:=5.10

include $(INCLUDE_DIR)/target.mk

$(eval $(call BuildTarget))
