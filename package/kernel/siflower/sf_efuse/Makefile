include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

PKG_NAME:=sf_efuse
PKG_RELEASE:=$(AUTORELEASE)

define KernelPackage/sf_efuse
  SUBMENU:=Kernel modules
  TITLE:=Siflower efuse read driver
  FILES:=$(PKG_BUILD_DIR)/sf_efuse.ko
  AUTOLOAD:=$(call AutoLoad, 61, sf_efuse)
endef

EXTRA_KCONFIG:= \
	CONFIG_SF_EFUSE=m

INC_DIR:=$(shell pwd)/../include
NOSTDINC_FLAGS += \
    $(KERNEL_NOSTDINC_FLAGS) \
    -I$(PKG_BUILD_DIR) \
    -I$(INC_DIR) \
    -Werror

define KernelPackage/sf_efuse/description
  Kernel module to siflower efuse driver.
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


$(eval $(call KernelPackage,sf_efuse))
