# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2022 OpenWrt.org
#

include $(TOPDIR)/rules.mk

PKG_NAME:=opensbi
PKG_VERSION:=1.6

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL=https://codeload.github.com/riscv-software-src/opensbi/tar.gz/refs/tags/v$(PKG_VERSION)?
PKG_HASH:=d11702103f177a2914e94eec57ce5ed820296d874f6b6525c4482e55d71a3667

PKG_BUILD_DIR=$(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)

PKG_TARGETS:=bin
PKG_FLAGS:=nonshared
PKG_LICENSE:=BSD-2-Clause
PKG_LICENSE_FILES:=COPYING.BSD
PKG_BUILD_PARALLEL:=1

PKG_MAINTAINER:=Zoltan HERPAI <wigyori@uid0.hu>

include $(INCLUDE_DIR)/package.mk

define Package/opensbi
    SECTION:=boot
    CATEGORY:=Boot Loaders
    DEPENDS:=@riscv64
    URL:=https://github.com/riscv/opensbi/blob/master/README.md
    VARIANT:=$(subst _,/,$(subst opensbi_,,$(1)))
    TITLE:=OpenSBI generic
    OPENSBI_IMAGE:=
    PLAT:=
    DEFCONFIG:=defconfig
endef

define Package/opensbi_generic
  $(Package/opensbi)
  TITLE:=OpenSBI generic
  OPENSBI_IMAGE:=fw_dynamic
  PLAT:=generic
endef

define Package/opensbi_siflower
  $(Package/opensbi)
  TITLE:=OpenSBI Siflower
  DEPENDS+=@TARGET_siflower
  OPENSBI_IMAGE:=fw_jump
  PLAT:=generic
  DEFCONFIG:=siflower_defconfig
  MAKE_VARS+=FW_JUMP_ADDR=0x20000000
endef

export GCC_HONOUR_COPTS=s

MAKE_VARS = \
	CROSS_COMPILE="$(TARGET_CROSS)"

define Build/Compile
	$(eval $(Package/opensbi_$(BUILD_VARIANT))) \
		+$(MAKE_VARS) $(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) \
		PLATFORM=$(PLAT) PLATFORM_DEFCONFIG=$(DEFCONFIG)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	$(CP) $(PKG_BUILD_DIR)/build/platform/$(PLAT)/firmware/$(OPENSBI_IMAGE).bin $(STAGING_DIR_IMAGE)/$(OPENSBI_IMAGE)-${BUILD_VARIANT}.bin
	$(STAGING_DIR_HOST)/bin/lzma e $(STAGING_DIR_IMAGE)/$(OPENSBI_IMAGE)-${BUILD_VARIANT}.bin -lc1 -lp2 -pb2 $(STAGING_DIR_IMAGE)/$(OPENSBI_IMAGE)-${BUILD_VARIANT}.bin.lzma
endef

$(eval $(call BuildPackage,opensbi_generic))
$(eval $(call BuildPackage,opensbi_siflower))
