include $(TOPDIR)/rules.mk

PKG_NAME:=dpns_acl
PKG_RELEASE:=$(AUTORELEASE)

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

define KernelPackage/dpns_acl
  SUBMENU:=Network Devices
  TITLE:=SIFLOWER DPNS ACL Driver
  DEPENDS:=+kmod-dpns_genl +kmod-dpns_common
  FILES:=$(PKG_BUILD_DIR)/dpnsAcl.ko
  AUTOLOAD:=$(call AutoLoad, 50, dpnsAcl)
endef

EXTRA_KCONFIG:= CONFIG_SIFLOWER_DPNS_ACL=m

INC_DIR:=$(shell pwd)/../include
NOSTDINC_FLAGS += \
    $(KERNEL_NOSTDINC_FLAGS) \
    -I$(PKG_BUILD_DIR) \
    -I$(INC_DIR) \
    -Werror

define KernelPackage/dpns_acl/description
 Kernel module to siflower dpns_acl driver.
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


$(eval $(call KernelPackage,dpns_acl))
