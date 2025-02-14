include $(TOPDIR)/rules.mk

PKG_NAME:=memtester
PKG_VERSION:=4.7.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://pyropus.ca./software/memtester/old-versions/
PKG_HASH:=skip

include $(INCLUDE_DIR)/package.mk

define Package/memtester
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=memtester
  DEPENDS:=+libc
endef

define Package/memtester/description
memtester 4.7.1
endef

define Build/Configure
	$(SED) "s#cc#$(TARGET_CC) $(TARGET_CFLAGS)#" $(PKG_BUILD_DIR)/conf-cc
	$(SED) "s#cc#$(TARGET_CC)#" $(PKG_BUILD_DIR)/conf-ld
endef

define Package/memtester/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/memtester $(1)/bin/
	$(INSTALL_BIN) ./files/memtester-multi.sh  $(1)/bin/
	$(INSTALL_BIN) ./files/date_output.sh  $(1)/bin/
endef


$(eval $(call BuildPackage,memtester))
