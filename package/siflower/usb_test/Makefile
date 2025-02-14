#
# Copyright (C) 2010-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=usb-test
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/usb-test
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=usb-test
endef

define Package/usb-test/description
	usb-test
endef

define Build/Compile
endef

define Build/Configure
endef

define Package/usb-test/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) ./files/usb_test.sh $(1)/bin/usb_test.sh
endef

$(eval $(call BuildPackage,usb-test))