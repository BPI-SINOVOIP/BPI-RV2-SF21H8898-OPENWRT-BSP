#
# Open Source Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=mbw
PKG_VERSION:=c3155b5
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/mbw
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=mbw.
endef

define Package/mbw/description
	mbw
endef

define Package/mbw/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mbw $(1)/bin/
endef

$(eval $(call BuildPackage,mbw))