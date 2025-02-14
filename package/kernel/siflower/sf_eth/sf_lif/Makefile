include $(TOPDIR)/rules.mk

PKG_NAME:=siflower-eth
PKG_RELEASE:=$(AUTORELEASE)
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

define KernelPackage/siflower-eth-dma
  TITLE:=Siflower Ethernet DMA driver
  SUBMENU:=Network Devices
  FILES:= $(PKG_BUILD_DIR)/sfxgmac-dma.ko
  DEPENDS:=+kmod-sf_kernel_common
  AUTOLOAD:=$(call AutoLoad, 47, sfxgmac-dma)
endef

define KernelPackage/siflower-eth-dma/config
	config NET_SIFLOWER_ETH_RX_THREAD
		bool "Enable threaded RX NAPI"
		default "n"
		help
		  Enable threaded RX NAPI.
endef

define Package/siflower-eth-dma/description
  Ethernet DMA driver for Siflower SoCs.
endef

define KernelPackage/siflower-eth-mac
  TITLE:=Siflower Ethernet MAC driver
  SUBMENU:=Network Devices
  FILES:= \
	$(PKG_BUILD_DIR)/sfXgmac.ko \
	$(LINUX_DIR)/net/phy/phylink.ko
  DEPENDS:=+kmod-phylink +kmod-siflower-eth-dma +kmod-siflower-eth-xpcs
  AUTOLOAD:=$(call AutoLoad, 59, sfXgmac)
endef

define KernelPackage/siflower-eth-mac/config
	config LY1141M_PHY
		bool "Enable LY1141M PHY"
		default "n"
		help
		  Enable LY1141M PHY.
endef

define Package/siflower-eth-mac/description
  Ethernet MAC driver for Siflower SoCs.
endef

define KernelPackage/siflower-eth-xpcs
  TITLE:=Siflower Ethernet XPCS driver
  SUBMENU:=Network Devices
  FILES:= \
	$(PKG_BUILD_DIR)/sfxpcs.ko
  AUTOLOAD:=$(call AutoLoad, 58, sfxpcs)
endef

define Package/siflower-eth-xpcs/description
  Ethernet XPCS driver for Siflower SoCs.
endef


INC_DIR:=$(shell pwd)/../include
EXTRA_KCONFIG := \
	CONFIG_NET_SIFLOWER_ETH_DMA=m	\
	CONFIG_NET_SIFLOWER_ETH_XPCS=m \
	CONFIG_NET_SIFLOWER_ETH_XGMAC=m

NOSTDINC_FLAGS += \
    $(KERNEL_NOSTDINC_FLAGS) \
    -I$(PKG_BUILD_DIR) \
    -I$(INC_DIR) \
    -Werror

define KernelPackage/sf_lif/description
 Kernel module to siflower lifernet driver.
endef

ifdef CONFIG_SFAX8_FACTORY_READ
NOSTDINC_FLAGS += -DCONFIG_SFAX8_FACTORY_READ
endif

ifdef CONFIG_PAGE_POOL_STATS
NOSTDINC_FLAGS += -DCONFIG_PAGE_POOL_STATS
endif

ifdef CONFIG_NET_SIFLOWER_ETH_USE_INTERNAL_SRAM
NOSTDINC_FLAGS += -DCONFIG_NET_SIFLOWER_ETH_USE_INTERNAL_SRAM
endif

ifdef CONFIG_NET_SIFLOWER_ETH_RX_THREAD
NOSTDINC_FLAGS += -DCONFIG_NET_SIFLOWER_ETH_RX_THREAD
endif

ifdef CONFIG_LY1141M_PHY
NOSTDINC_FLAGS += -DCONFIG_LY1141M_PHY
endif

ifdef CONFIG_DPNS_THROUGHPUT_ETH_BEST
NOSTDINC_FLAGS += -DCONFIG_DPNS_THROUGHPUT_ETH_BEST
endif

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


$(eval $(call KernelPackage,siflower-eth-dma))
$(eval $(call KernelPackage,siflower-eth-mac))
$(eval $(call KernelPackage,siflower-eth-xpcs))
