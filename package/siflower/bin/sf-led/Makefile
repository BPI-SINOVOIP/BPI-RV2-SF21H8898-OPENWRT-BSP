#
# Copyright (C) 2015-2018 neil.zhang@siflower.com.cn
#
# sifiower Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=sf-led
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/sf-led
SECTION:=utils
CATEGORY:=Utilities
TITLE:=led and button centralized dispatcher.
DEPENDS:=+libpthread
MAINTAINER:=Nevermore <nevermore.wang@siflower.com.cn>
endef

define Package/sf-led/description
SiFlower led control.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall -Wno-unused-but-set-variable -Wno-unused-function" \
		LDFLAGS="$(TARGET_LDFLAGS) -L$(PKG_BUILD_DIR) -lpthread"
endef

define Package/sf-led/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/net_detect $(1)/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sf_led $(1)/bin/
	$(INSTALL_BIN) ./files/net_detect_init  $(1)/etc/init.d/
endef


$(eval $(call BuildPackage,sf-led))
