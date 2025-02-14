include $(TOPDIR)/rules.mk

PKG_NAME:=yt9215-swconfig
PKG_RELEASE:=1.1.005

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

define KernelPackage/yt9215-swconfig
  SUBMENU:=Network Devices
  TITLE:=Motorcomm YT9215S Ethernet Switch Driver
  DEPENDS:=+kmod-dpns_l2
  FILES:=$(PKG_BUILD_DIR)/yt9215_swconfig.ko
  AUTOLOAD:=$(call AutoLoad,47,yt9215_swconfig)
  VARIANT:=1sw
endef

define KernelPackage/yt9215-swconfig/description
  swconfig driver for YT9215S (Port 8 fixed to 2500Base-X)
endef

define KernelPackage/yt9215-swconfig-2sw
  SUBMENU:=Network Devices
  TITLE:=Motorcomm YT9215S x2 Ethernet Switch Driver
  DEPENDS:=+kmod-dpns_l2
  FILES:=$(PKG_BUILD_DIR)/yt9215_swconfig.ko
  AUTOLOAD:=$(call AutoLoad,47,yt9215_swconfig)
  VARIANT:=2sw
endef

define KernelPackage/yt9215-swconfig/config
	if PACKAGE_kmod-yt9215-swconfig
		config SIFLOWER_MPW1_ESWITCH_LIGHT
			bool "build light eswitch driver"
			default "n"
	endif
endef

define KernelPackage/yt9215-swconfig-2sw/config
	if PACKAGE_kmod-yt9215-swconfig-2sw
		config SIFLOWER_MPW1_ESWITCH_LIGHT
			bool "build light eswitch driver"
			default "n"
	endif
endef

define KernelPackage/yt9215-swconfig-2sw/description
  swconfig driver for two YT9215S (Port 8 fixed to 2500Base-X)
endef

COPTS = -DBOARD_MAX_UNIT_NUM=1

ifeq ($(BUILD_VARIANT),2sw)
	COPTS = -DBOARD_MAX_UNIT_NUM=2
endif

ifdef CONFIG_SIFLOWER_MPW1_ESWITCH_LIGHT
EXTRA_KCONFIG += CONFIG_SIFLOWER_MPW1_ESWITCH_LIGHT=y
NOSTDINC_FLAGS += -DCONFIG_SIFLOWER_MPW1_ESWITCH_LIGHT
endif

define Build/Compile
	$(KERNEL_MAKE) M=$(PKG_BUILD_DIR) EXTRA_CFLAGS=$(COPTS) \
	$(KERNEL_MAKE_FLAGS) \
	NOSTDINC_FLAGS=$(NOSTDINC_FLAGS) \
	$(EXTRA_KCONFIG) \
	modules
endef

$(eval $(call KernelPackage,yt9215-swconfig))
$(eval $(call KernelPackage,yt9215-swconfig-2sw))
