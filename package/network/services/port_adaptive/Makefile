
include $(TOPDIR)/rules.mk

PKG_NAME:=port_adaptive
PKG_VERSION:=1
PKG_RELEASE:=2
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define Package/port_adaptive
  SECTION:=net
  CATEGORY:=Network
  TITLE:=WAN/LAN port auto adapt demo
  DEPENDS:= +libuci
endef

define Package/port_adaptive/description
 WAN/LAN auto adapt demo
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall -g" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Build/Configure
endef

define Package/port_adaptive/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/send $(1)/bin

	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) ./files/auto_adaptive.sh $(1)/bin

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/auto_init $(1)/etc/init.d
endef

$(eval $(call BuildPackage,port_adaptive))
