
include $(TOPDIR)/rules.mk

PKG_NAME:=wifi-test-suite
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define Package/wifi-test-suite
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Wi-FiTestSuite
  DEPENDS:=+libc +libpthread +librt
endef

define Package/wifi-test-suite/description
	Wi-FiTestSuite-Linux-DUT-9.2.0
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		RANLIB="$(TARGET_CROSS)ranlib"
endef

define Build/Configure
endef

define Package/wifi-test-suite/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dut/wfa_dut $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ca/wfa_ca $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/usr/local/sbin
	$(CP) $(PKG_BUILD_DIR)/scripts/* $(1)/usr/local/sbin/
endef

$(eval $(call BuildPackage,wifi-test-suite))
