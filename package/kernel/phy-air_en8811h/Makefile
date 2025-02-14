#
# Copyright (C) 2020 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=phy-air_en8811h
PKG_VERSION:=24082302
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/phy-air_en8811h
   SUBMENU:=Network Devices
   TITLE:=Airoha EN8811H PHY driver
   FILES:= \
        $(PKG_BUILD_DIR)/air_en8811h.ko
   AUTOLOAD:=$(call AutoLoad,20,air_en8811h,1)
endef

define KernelPackage/phy-air_en8811h/description
   Supports the Airoha EN8811H PHY.
endef

define Build/Compile
        $(KERNEL_MAKE) M=$(PKG_BUILD_DIR) modules
endef

define KernelPackage/phy-air_en8811h/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) ./src/EN8811_FW/8811_24082302/*.bin $(1)/lib/firmware/
endef

$(eval $(call KernelPackage,phy-air_en8811h))
