include $(TOPDIR)/rules.mk

PKG_NAME:=dpns_tmu
PKG_RELEASE:=$(AUTORELEASE)

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

define KernelPackage/dpns_tmu
  SUBMENU:=Network Devices
  TITLE:=SIFLOWER DPNS TMU Driver
  DEPENDS:=+kmod-dpns_genl +kmod-dpns_common
  FILES:=$(PKG_BUILD_DIR)/dpnsTmu.ko
  AUTOLOAD:=$(call AutoLoad, 56, dpnsTmu)
endef

define KernelPackage/dpns_tmu/config
	if PACKAGE_kmod-dpns_tmu
		config SIFLOWER_DPNS_TMU_GENL
			bool "Siflower DPNS TMU generic netlink support"
			default "y"

		config SIFLOWER_DPNS_TMU_DEBUGFS
			bool "Siflower DPNS TMU debugfs support"
			default "y"
	endif
endef

EXTRA_KCONFIG:= CONFIG_SIFLOWER_DPNS_TMU=m

ifdef CONFIG_SIFLOWER_DPNS_TMU_GENL
NOSTDINC_FLAGS += -DCONFIG_SIFLOWER_DPNS_TMU_GENL
EXTRA_KCONFIG += CONFIG_SIFLOWER_DPNS_TMU_GENL=y
endif

ifdef CONFIG_SIFLOWER_DPNS_TMU_DEBUGFS
NOSTDINC_FLAGS += -DCONFIG_SIFLOWER_DPNS_TMU_DEBUGFS
EXTRA_KCONFIG += CONFIG_SIFLOWER_DPNS_TMU_DEBUGFS=y
endif

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

define KernelPackage/dpns_tmu/description
 Kernel module to siflower dpns_tmu driver.
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


$(eval $(call KernelPackage,dpns_tmu))
