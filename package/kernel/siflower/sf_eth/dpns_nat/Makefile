include $(TOPDIR)/rules.mk

PKG_NAME:=dpns_nat
PKG_RELEASE:=$(AUTORELEASE)

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

define KernelPackage/dpns_nat
  SUBMENU:=Network Devices
  TITLE:=SIFLOWER DPNS NAT Driver
  DEPENDS:=+kmod-dpns_genl +kmod-dpns_common
  FILES:=$(PKG_BUILD_DIR)/dpnsNat.ko
  AUTOLOAD:=$(call AutoLoad, 53, dpnsNat)
endef

define KernelPackage/dpns_nat/config
	if PACKAGE_kmod-dpns_nat
		config PACKAGE_kmod-dpns_nat_ELK_DSNAT_SINGLE
			bool "use the same memory area for both external D/SNAPT"
	endif
endef

EXTRA_KCONFIG:= CONFIG_SIFLOWER_DPNS_NAT=m

INC_DIR:=$(shell pwd)/../include
NOSTDINC_FLAGS += \
    $(KERNEL_NOSTDINC_FLAGS) \
    $(if $(CONFIG_PACKAGE_kmod-dpns_nat_ELK_DSNAT_SINGLE),-DCONFIG_ELK_DSNAT_SINGLE,) \
    -I$(PKG_BUILD_DIR) \
    -I$(INC_DIR) \
    -Werror

define KernelPackage/dpns_nat/description
 Kernel module to siflower dpns_nat driver.
endef

define KernelPackage/dpns_nat/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) ./files/* $(1)/sbin
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


$(eval $(call KernelPackage,dpns_nat))
