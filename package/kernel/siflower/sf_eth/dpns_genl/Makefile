include $(TOPDIR)/rules.mk

PKG_NAME:=dpns_genl
PKG_RELEASE:=$(AUTORELEASE)

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

define KernelPackage/dpns_genl
  SUBMENU:=Network Devices
  TITLE:=SIFLOWER DPNS GENERIC NETLINK Driver
  FILES:=$(PKG_BUILD_DIR)/dpnsGenl.ko
  AUTOLOAD:=$(call AutoLoad, 48, dpnsGenl)
endef

INC_DIR:=$(shell pwd)/../include
EXTRA_KCONFIG:= CONFIG_SIFLOWER_DPNS_GENL=m

NOSTDINC_FLAGS += \
    $(KERNEL_NOSTDINC_FLAGS) \
    -I$(PKG_BUILD_DIR) \
    -I$(INC_DIR) \
    -Werror

define KernelPackage/dpns_genl/description
 Kernel module to siflower dpns_genl driver.
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(CP) ../dpns_l2/src/se_common.h $(PKG_BUILD_DIR)/
	$(CP) ../dpns_tmu/src/sf_tmu_regs.h $(PKG_BUILD_DIR)/
	$(CP) ../dpns_tmu/src/sf_tmu_genl.h $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(CP) ../dpns_l2/src/se_common.h $(PKG_BUILD_DIR)/
	$(CP) ../dpns_tmu/src/sf_tmu_regs.h $(PKG_BUILD_DIR)/
	$(CP) ../dpns_tmu/src/sf_tmu_genl.h $(PKG_BUILD_DIR)/
	$(MAKE) $(PKG_JOBS) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)" \
		SUBDIRS="$(PKG_BUILD_DIR)" \
		NOSTDINC_FLAGS="$(NOSTDINC_FLAGS)" \
		$(EXTRA_KCONFIG)  \
		modules
endef


$(eval $(call KernelPackage,dpns_genl))
