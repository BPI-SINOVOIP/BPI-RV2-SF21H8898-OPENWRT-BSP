include $(TOPDIR)/rules.mk

PKG_NAME:=sf_kernel_common
PKG_RELEASE:=$(AUTORELEASE)

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

define KernelPackage/sf_kernel_common
  SUBMENU:=Network Devices
  TITLE:=SIFLOWER Kernel Common API Driver
  FILES:= $(PKG_BUILD_DIR)/sf_kernel_common.ko
  AUTOLOAD:=$(call AutoLoad, 40, sf_kernel_common)
endef


EXTRA_KCONFIG:= CONFIG_SIFLOWER_KERNEL_COMMON=m

ifdef CONFIG_KERNEL_KALLSYMS
NOSTDINC_FLAGS += -DCONFIG_KALLSYMS
endif

INC_DIR:=$(shell pwd)/../include
NOSTDINC_FLAGS += \
    $(KERNEL_NOSTDINC_FLAGS) \
    -I$(PKG_BUILD_DIR) \
    -I$(INC_DIR) \
    -Werror

define KernelPackage/sf_kernel_common/description
 Kernel module to siflower sf_kernel_common driver.
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


$(eval $(call KernelPackage,sf_kernel_common))
