
include $(TOPDIR)/rules.mk

PKG_NAME:=sigma
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define Package/sigma
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Wi-FiTestSuite
  DEPENDS:=+libc +libpthread +librt
endef

define Package/sigma/description
	sigma-dut-test
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		RANLIB="$(TARGET_CROSS)ranlib"
	$(MAKE) -C $(PKG_BUILD_DIR)/ucc_test \
		CC="$(TARGET_CC)"
endef

define Build/Configure
endef

define Package/sigma/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sigma_dut $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ucc_test/ucc $(1)/usr/bin/
endef

$(eval $(call BuildPackage,sigma))
