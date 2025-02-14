include $(TOPDIR)/rules.mk

PKG_NAME:=dpns_vlan
PKG_RELEASE:=$(AUTORELEASE)

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

define KernelPackage/dpns_vlan
  SUBMENU:=Network Devices
  TITLE:=SIFLOWER DPNS VLAN Driver
  DEPENDS:=+kmod-dpns_genl +kmod-dpns_common
  FILES:=$(PKG_BUILD_DIR)/dpnsVlan.ko
  AUTOLOAD:=$(call AutoLoad, 52, dpnsVlan)
endef


EXTRA_KCONFIG:= CONFIG_SIFLOWER_DPNS_VLAN=m

INC_DIR:=$(shell pwd)/../include
NOSTDINC_FLAGS += \
    $(KERNEL_NOSTDINC_FLAGS) \
    -I$(PKG_BUILD_DIR) \
    -I$(INC_DIR) \
    -Werror

define KernelPackage/dpns_vlan/description
 Kernel module to siflower dpns_vlan driver.
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


$(eval $(call KernelPackage,dpns_vlan))
