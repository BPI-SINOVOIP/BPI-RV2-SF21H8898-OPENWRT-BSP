include $(TOPDIR)/rules.mk

PKG_NAME:=dpnsMain
PKG_RELEASE:=$(AUTORELEASE)

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

define KernelPackage/dpnsMain
  SUBMENU:=Network Devices
  TITLE:=SIFLOWER DPNS Driver
  DEPENDS:=+kmod-dpns_common +kmod-dpns_genl +kmod-dpns_l2 +kmod-dpns_vlan \
    +kmod-siflower-eth-dma +kmod-dpns_nat +kmod-dpns_router \
  	+PACKAGE_kmod-dpns_tmu:kmod-dpns_tmu +PACKAGE_kmod-dpns_mcast:kmod-dpns_mcast +PACKAGE_kmod-dpns_acl:kmod-dpns_acl
  FILES:=$(PKG_BUILD_DIR)/dpnsMain.ko
  AUTOLOAD:=$(call AutoLoad, 57, dpnsMain)
endef


EXTRA_KCONFIG:= CONFIG_SIFLOWER_DPNS=m

ifdef CONFIG_PACKAGE_kmod-dpns_l2
NOSTDINC_FLAGS += -DCONFIG_SIFLOWER_DPNS_L2
endif

ifdef CONFIG_PACKAGE_kmod-dpns_vlan
NOSTDINC_FLAGS += -DCONFIG_SIFLOWER_DPNS_VLAN
endif

ifdef CONFIG_PACKAGE_kmod-dpns_nat
NOSTDINC_FLAGS += -DCONFIG_SIFLOWER_DPNS_NAT
endif

ifdef CONFIG_PACKAGE_kmod-dpns_router
NOSTDINC_FLAGS += -DCONFIG_SIFLOWER_DPNS_ROUTER
endif

ifdef CONFIG_PACKAGE_kmod-dpns_tmu
NOSTDINC_FLAGS += -DCONFIG_SIFLOWER_DPNS_TMU
endif

ifdef CONFIG_PACKAGE_kmod-dpns_mcast
NOSTDINC_FLAGS += -DCONFIG_SIFLOWER_DPNS_MCAST
endif

ifdef CONFIG_PACKAGE_kmod-dpns_acl
NOSTDINC_FLAGS += -DCONFIG_SIFLOWER_DPNS_ACL
endif

ifdef CONFIG_PACKAGE_kmod-dpns_common
NOSTDINC_FLAGS += -DCONFIG_SIFLOWER_DPNS_COMMON
endif


INC_DIR:=$(shell pwd)/../include
NOSTDINC_FLAGS += \
    $(KERNEL_NOSTDINC_FLAGS) \
    -I$(PKG_BUILD_DIR) \
    -I$(INC_DIR) \
    -Werror

define KernelPackage/dpnsMain/description
 Kernel module to siflower dpnsMain driver.
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) $(PKG_JOBS) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)" \
		SUBDIRS="$(PKG_BUILD_DIR)" \
		NOSTDINC_FLAGS="$(NOSTDINC_FLAGS)" \
		$(EXTRA_KCONFIG)  \
		modules
endef

define Build/Clean
	rm -rf $(dir $(PKG_BUILD_DIR))dpns*
	rm -rf $(dir $(PKG_BUILD_DIR))sf*
	rm -rf $(dir $(PKG_BUILD_DIR))siflower-*
endef


$(eval $(call KernelPackage,dpnsMain))
