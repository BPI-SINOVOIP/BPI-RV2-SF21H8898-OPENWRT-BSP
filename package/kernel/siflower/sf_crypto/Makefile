#
# Copyright (C) 2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=crypto-hw-siflower
PKG_RELEASE:=1
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/crypto-hw-siflower
  SUBMENU:=Cryptographic API modules
  TITLE:=Siflower crypto engine support
  DEPENDS:=+kmod-crypto-gcm +kmod-crypto-ccm
  FILES:=$(PKG_BUILD_DIR)/sf_crypto.ko
  AUTOLOAD:=$(call AutoProbe,sf_crypto)
endef

include $(INCLUDE_DIR)/kernel-defaults.mk

define KernelPackage/crypto-hw-siflower/config
	config PACKAGE_kmod-crypto-hw-siflower_HASH
		bool "Enable Siflower Crypto hardware hash support"
		help
		  Enable Siflower Crypto hardware hash support (MD5, SHA-1, SHA-2).
		  Due to hardware limitation, enabling this will limit AES to single channel.
endef
PKG_CONFIG_DEPENDS += CONFIG_PACKAGE_kmod-crypto-hw-siflower_HASH

ifdef CONFIG_PACKAGE_kmod-crypto-hw-siflower_HASH
NOSTDINC_FLAGS := -DCONFIG_CRYPTO_SIFLOWER_HASH
EXTRA_KCONFIG := CONFIG_CRYPTO_SIFLOWER_HASH=m
endif

define Build/Compile
	+$(KERNEL_MAKE) $(PKG_JOBS) M="$(PKG_BUILD_DIR)" NOSTDINC_FLAGS="$(NOSTDINC_FLAGS)" $(EXTRA_KCONFIG) modules
endef

$(eval $(call KernelPackage,crypto-hw-siflower))
