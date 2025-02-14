# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2012-2015 OpenWrt.org
# Copyright (C) 2016-2017 LEDE project

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

export KERNERL_SIGN=false

# default all platform image(fit) build
define Device/Default
  PROFILES = Default $$(DEVICE_NAME)
  KERNEL_INITRAMFS = kernel-bin | lzma | fit lzma $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb with-initrd
  KERNEL = kernel-bin | lzma
  FILESYSTEMS := squashfs
  DEVICE_DTS_DIR := $(DTS_DIR)
  IMAGES := sysupgrade.bin
ifeq ($(KERNERL_SIGN), TRUE)
  IMAGE/sysupgrade.bin = append-kernel | sign_fit lzma $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb external-static-with-rootfs | pad-rootfs | append-metadata
else
  IMAGE/sysupgrade.bin = append-kernel | fit lzma $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb external-static-with-rootfs | pad-rootfs | append-metadata
endif
endef

define Device/UbiFit
  KERNEL_IN_UBI := 1
  KERNEL = kernel-bin | lzma | fit lzma $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb
  IMAGES := nand-factory.ubi nand-sysupgrade.bin
  IMAGE/nand-factory.ubi := append-ubi
  IMAGE/nand-sysupgrade.bin := sysupgrade-tar | append-metadata
endef

include $(SUBTARGET).mk

define Image/Build
	$(call Image/Build/$(1),$(1))
endef

$(eval $(call BuildImage))
