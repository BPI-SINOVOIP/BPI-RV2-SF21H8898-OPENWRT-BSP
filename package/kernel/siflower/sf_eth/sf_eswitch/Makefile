include $(TOPDIR)/rules.mk

PKG_NAME:=sf_eswitch
PKG_RELEASE:=$(AUTORELEASE)

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

define KernelPackage/sf_eswitch
  SUBMENU:=Network Devices
  TITLE:=SIFLOWER Ethernet Switch Driver
  FILES:=$(PKG_BUILD_DIR)/sf_eswitch.ko
  AUTOLOAD:=$(call AutoLoad, 50, sf_eswitch)
endef

define KernelPackage/sf_eswitch/config
	if PACKAGE_kmod-sf_eswitch
		config SIFLOWER_ESWITCH_LIGHT
			bool "build light eswitch driver"
			default "y"
	endif
endef


EXTRA_KCONFIG:= CONFIG_SIFLOWER_ESWITCH_DRIVER=m

NOSTDINC_FLAGS := \
    $(KERNEL_NOSTDINC_FLAGS) \
    -I$(PKG_BUILD_DIR)

define KernelPackage/sf_eswitch/description
 Kernel module to siflower sf_eswitch driver.
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


$(eval $(call KernelPackage,sf_eswitch))
