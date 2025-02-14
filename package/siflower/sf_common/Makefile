#
# Copyright (C) 2010-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/version.mk

PKG_NAME:=siflower-common
PKG_VERSION:=1
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/siflower-common/Default
  SECTION:=sfcommon
  CATEGORY:=Utilities
  TITLE:=siflowr-common
  SUBMENU:=siflower-common
endef

define Build/Compile
endef

define Package/networks
$(call Package/siflower-common/Default)
  TITLE:=scprits about ethernets
endef

define Package/networks/config
	if PACKAGE_networks
	config PACKAGE_configs_net-netclash
		bool "net - netclash"
		default "n"
	# config PACKAGE_configs_net-speed
	# 	bool "net - speed"
	# 	default "n"
	# config PACKAGE_configs_net-wan
	# 	bool "net - wan speed and status"
	# 	default "n"
	# config PACKAGE_configs_net-check
	# 	bool "net - check_net"
	# 	default "n"
	# config PACKAGE_configs_net-dhcpserver
	# 	bool "net - dhcp server status"
	# 	default "n"
	endif
endef

define Package/networks/install

ifdef CONFIG_PACKAGE_configs_net-netclash
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) ./networks/usr/sbin/netclash $(1)/usr/sbin/
endif

# ifdef CONFIG_PACKAGE_configs_net-speed
# 	$(INSTALL_DIR) $(1)/sbin
# 	$(CP) ./networks/sbin/speed $(1)/sbin/
# endif

# ifdef CONFIG_PACKAGE_configs_net-wan
# 	$(INSTALL_DIR) $(1)/bin
# 	$(CP) ./networks/bin/wanLinkStatus $(1)/bin/
# 	$(INSTALL_DIR) $(1)/etc/init.d
# 	$(CP) ./networks/etc/init.d/wan_speed $(1)/etc/init.d/
# endif

# ifdef CONFIG_PACKAGE_configs_net-check
# 	$(INSTALL_DIR) $(1)/sbin
# 	$(CP) ./networks/sbin/check_net $(1)/sbin/
# endif

# ifdef CONFIG_PACKAGE_configs_net-dhcpserver
# 	$(INSTALL_DIR) $(1)/etc/init.d/
# 	$(CP) ./dhcpserver/etc/init.d/get_dhcpserver_status $(1)/etc/init.d/
# 	$(INSTALL_DIR) $(1)/bin
# 	$(CP) ./dhcpserver/bin/dhcpserver_status.sh $(1)/bin/
# endif

endef

$(eval $(call BuildPackage,networks))
