define Device/bpi
	$(call Device/NAND)
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := BPI-EVB
	DEVICE_DTS := sf21h8898-bpi-nand
	SUPPORTED_DEVICES := siflower,sf21h8898-bpi-nand
endef
TARGET_DEVICES += bpi

define Device/bpi-rv2-nand
	$(call Device/NAND)
	DEVICE_VENDOR := BananaPi
	DEVICE_MODEL := BPI-RV2 (Booting from NAND)
	DEVICE_DTS := sf21h8898-bpi-rv2-nand
	DEVICE_PACKAGES := kmod-phy-air_en8811h kmod-usb-dwc2 kmod-phy-sf21a6826-usb kmod-rtc-pcf8563 kmod-i2c-designware-platform
	SUPPORTED_DEVICES := bananapi,bpi-rv2-nand
endef
TARGET_DEVICES += bpi-rv2-nand

define Device/siflower-one-nand
	$(call Device/NAND)
	DEVICE_VENDOR := Siflower
	DEVICE_MODEL := One (Booting from NAND)
	DEVICE_DTS := sf21h8898-siflower-one-nand
	DEVICE_PACKAGES := kmod-usb-dwc2 kmod-phy-sf21a6826-usb kmod-rtc-pcf8563 kmod-i2c-designware-platform
	SUPPORTED_DEVICES := siflower,one-nand
endef
TARGET_DEVICES += siflower-one-nand

define Device/siflower-one-nor
	$(call Device/NOR_FIT)
	DEVICE_VENDOR := Siflower
	DEVICE_MODEL := One (Booting from NOR)
	DEVICE_DTS := sf21h8898-siflower-one-nor
	DEVICE_PACKAGES := kmod-usb-dwc2 kmod-phy-sf21a6826-usb kmod-rtc-pcf8563 kmod-i2c-designware-platform
	SUPPORTED_DEVICES := siflower,one-nor
endef
TARGET_DEVICES += siflower-one-nor
