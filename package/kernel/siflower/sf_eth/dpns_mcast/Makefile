include $(TOPDIR)/rules.mk

PKG_NAME:=dpns_mcast
PKG_RELEASE:=$(AUTORELEASE)

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

define KernelPackage/dpns_mcast
  SUBMENU:=Network Devices
  TITLE:=SIFLOWER DPNS MCAST Driver
  DEPENDS:=+kmod-dpns_genl +kmod-dpns_common
  FILES:=$(PKG_BUILD_DIR)/dpnsMcast.ko
  AUTOLOAD:=$(call AutoLoad, 55, dpnsMcast)
endef

define KernelPackage/dpns_mcast/config
	if PACKAGE_kmod-dpns_mcast
		config SIFLOWER_DPNS_MCAST_GENL
			bool "Siflower DPNS MCAST generic netlink support"
			default "y"
	endif
endef

EXTRA_KCONFIG:= CONFIG_SIFLOWER_DPNS_MCAST=m

ifdef CONFIG_SIFLOWER_DPNS_MCAST_GENL
NOSTDINC_FLAGS += -DCONFIG_SIFLOWER_DPNS_MCAST_GENL
EXTRA_KCONFIG += CONFIG_SIFLOWER_DPNS_MCAST_GENL=y
endif

INC_DIR:=$(shell pwd)/../include
NOSTDINC_FLAGS += \
    $(KERNEL_NOSTDINC_FLAGS) \
    -I$(PKG_BUILD_DIR) \
    -I$(INC_DIR) \
    -Werror

define KernelPackage/dpns_mcast/description
 Kernel module to siflower dpns_mcast driver.
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


$(eval $(call KernelPackage,dpns_mcast))
