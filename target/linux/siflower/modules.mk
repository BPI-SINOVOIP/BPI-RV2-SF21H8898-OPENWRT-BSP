define KernelPackage/phy-sf21a6826-usb
  TITLE:=Siflower SF21A6826 USB 2.0 PHY Driver
  KCONFIG:=CONFIG_PHY_SF21A6826_USB
  DEPENDS:=@TARGET_siflower
  SUBMENU:=$(USB_MENU)
  FILES:=$(LINUX_DIR)/drivers/phy/phy-sf21a6826-usb.ko
  AUTOLOAD:=$(call AutoLoad,45,phy-sf21a6826-usb,1)
endef

define KernelPackage/phy-sf21a6826-usb/description
  Support for Siflower SF21A6826 USB 2.0 PHY connected to the USB
  controller.
endef

$(eval $(call KernelPackage,phy-sf21a6826-usb))
