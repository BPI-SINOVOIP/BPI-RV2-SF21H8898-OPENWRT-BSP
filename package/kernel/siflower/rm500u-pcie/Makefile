include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=rm500u-pcie
PKG_RELEASE:=1.1.8
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/rm500u-pcie
  SECTION:=kernel
  SUBMENU:=Network Devices
  TITLE:=Quectel RM500U module in PCIE mode
  FILES:=$(PKG_BUILD_DIR)/sprd_pcie.ko
  AUTOLOAD:=$(call AutoProbe,sprd_pcie)
endef

define KernelPackage/rm500u-pcie/description
  PCIE driver for Quectel RM500U
endef

define Build/Compile
        $(KERNEL_MAKE) M=$(PKG_BUILD_DIR) modules
endef

$(eval $(call KernelPackage,rm500u-pcie))
