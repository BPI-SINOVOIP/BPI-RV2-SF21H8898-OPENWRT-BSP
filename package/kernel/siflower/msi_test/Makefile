include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=msi_test
PKG_VERSION:=1.0
PKG_RELEASE:=$(AUTORELEASE)
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/msi_test
SUBMENU:=Wireless Drivers
TITLE:=SIFLOWER interrupts test driver
FILES +=\
	$(PKG_BUILD_DIR)/msi_test.ko
endef

EXTRA_KCONFIG :=\
	CONFIG_MSI_TEST=m

EXTRA_CFLAGS:= \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG)))) \

NOSTDINC_FLAGS := \
	$(KERNEL_NOSTDINC_FLAGS) \
	-I$(PKG_BUILD_DIR)

MAKE_OPTS:= \
	$(KERNEL_MAKE_FLAGS) \
	M="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
	NOSTDINC_FLAGS="$(NOSTDINC_FLAGS)" \
	$(EXTRA_KCONFIG)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)
	$(CP) -r ./case $(PKG_BUILD_DIR)/
	$(CP) -r ./script $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
	$(MAKE_OPTS) \
	modules
endef

define KernelPackage/msi_test/install
	$(INSTALL_DIR) $(1)/lib/firmware
	cp $(PKG_BUILD_DIR)/case/*.bin $(1)/lib/firmware/
	$(INSTALL_DIR) $(1)/bin
	cp $(PKG_BUILD_DIR)/script/msitest $(1)/bin/
endef
$(eval $(call KernelPackage,msi_test))