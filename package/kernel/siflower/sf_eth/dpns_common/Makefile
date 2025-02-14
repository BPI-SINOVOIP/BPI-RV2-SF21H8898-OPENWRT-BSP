include $(TOPDIR)/rules.mk

PKG_NAME:=dpns_common
PKG_RELEASE:=$(AUTORELEASE)

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

define KernelPackage/dpns_common
  SUBMENU:=Network Devices
  TITLE:=SIFLOWER DPNS Common Driver
  DEPENDS:=+kmod-dpns_genl
  FILES:=$(PKG_BUILD_DIR)/dpnsCommon.ko
  AUTOLOAD:=$(call AutoLoad, 49, dpnsCommon)
endef

define KernelPackage/dpns_common/config
	choice
		prompt "dpns tmu buffer config"
		default DPNS_THROUGHPUT_BALANCE
		help
		  This option allows you to select tmu buffer config mode.

	config DPNS_THROUGHPUT_BALANCE
		bool "Config DPNS TMU buffer balanced between wifi and eth"
		help
		  Tmu buffer configed balanced between wifi and eth.

	config DPNS_THROUGHPUT_WIFI_BEST
		bool "Config DPNS TMU buffer wifi best"
		help
		  Tmu buffer configed wifi best.

	config DPNS_THROUGHPUT_ETH_BEST
		bool "Config DPNS TMU buffer eth best"
		help
		  Tmu buffer configed ethernet best.

	endchoice
endef


EXTRA_KCONFIG:= CONFIG_SIFLOWER_DPNS_COMMON=m

ifdef CONFIG_DPNS_THROUGHPUT_BALANCE
NOSTDINC_FLAGS += -DCONFIG_DPNS_THROUGHPUT_BALANCE
endif

ifdef CONFIG_DPNS_THROUGHPUT_WIFI_BEST
NOSTDINC_FLAGS += -DCONFIG_DPNS_THROUGHPUT_WIFI_BEST
endif

ifdef CONFIG_DPNS_THROUGHPUT_ETH_BEST
NOSTDINC_FLAGS += -DCONFIG_DPNS_THROUGHPUT_ETH_BEST
endif


INC_DIR:=$(shell pwd)/../include
NOSTDINC_FLAGS += \
    $(KERNEL_NOSTDINC_FLAGS) \
    -I$(PKG_BUILD_DIR) \
    -I$(INC_DIR) \
    -Werror

define KernelPackage/dpns_common/description
 Kernel module to siflower dpns_common driver.
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


$(eval $(call KernelPackage,dpns_common))
