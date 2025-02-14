DTS_DIR := $(DTS_DIR)/siflower
KERNEL_LOADADDR := 0x20000000

define Build/fit-with-opensbi
	$(TOPDIR)/scripts/mkits.sh \
		-F $(STAGING_DIR_IMAGE)/fw_jump-siflower.bin.lzma \
		-D $(DEVICE_NAME) -o $@.its -k $@ \
		-C $(word 1,$(1)) $(if $(word 2,$(1)),\
		$(if $(DEVICE_DTS_OVERLAY),-d $(KERNEL_BUILD_DIR)/image-$$(basename $(word 2,$(1))),\
			-d $(word 2,$(1)))) \
		$(if $(findstring with-rootfs,$(word 3,$(1))),-r $(IMAGE_ROOTFS)) \
		$(if $(findstring with-initrd,$(word 3,$(1))), \
			$(if $(CONFIG_TARGET_ROOTFS_INITRAMFS_SEPARATE), \
				-i $(KERNEL_BUILD_DIR)/initrd.cpio$(strip $(call Build/initrd_compression)))) \
		-a $(KERNEL_LOADADDR) -e $(if $(KERNEL_ENTRY),$(KERNEL_ENTRY),$(KERNEL_LOADADDR)) \
		$(if $(DEVICE_FDT_NUM),-n $(DEVICE_FDT_NUM)) \
		$(if $(DEVICE_DTS_DELIMITER),-l $(DEVICE_DTS_DELIMITER)) \
		$(if $(DEVICE_DTS_OVERLAY),$(foreach dtso,$(DEVICE_DTS_OVERLAY), -O $(dtso):$(KERNEL_BUILD_DIR)/image-$(dtso).dtb)) \
		-c $(if $(DEVICE_DTS_CONFIG),$(DEVICE_DTS_CONFIG),"config-1") \
		-A $(LINUX_KARCH) -v $(LINUX_VERSION)
	PATH=$(LINUX_DIR)/scripts/dtc:$(PATH) mkimage $(if $(findstring external,$(word 3,$(1))),\
		-E -B 0x1000 $(if $(findstring static,$(word 3,$(1))),-p 0x1000)) -f $@.its $@.new
	@mv $@.new $@
endef

define Device/Default
  PROFILES = Default $$(DEVICE_NAME)
  KERNEL_INITRAMFS = kernel-bin | lzma | fit-with-opensbi lzma $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb with-initrd
  KERNEL = kernel-bin | lzma
  KERNEL_ENTRY := 0x24000000
  FILESYSTEMS := squashfs
  DEVICE_DTS_DIR := $(DTS_DIR)
  IMAGES := sysupgrade.bin
ifeq ($(KERNERL_SIGN), TRUE)
  IMAGE/sysupgrade.bin = append-kernel | sign_fit lzma $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb external-static-with-rootfs | pad-rootfs | append-metadata
else
  IMAGE/sysupgrade.bin = append-kernel | fit-with-opensbi lzma $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb | append-rootfs | pad-rootfs | append-metadata
endif
endef

define Device/NAND
  KERNEL := kernel-bin | gzip
  KERNEL_INITRAMFS = kernel-bin | lzma | \
	fit lzma $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb with-initrd | pad-to 128k
  KERNEL_ENTRY := 0x20000000
  IMAGE/sysupgrade.bin = append-kernel | fit gzip $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb external-static-with-rootfs | append-metadata
endef

define Device/NOR_FIT
  KERNEL := kernel-bin | lzma
  KERNEL_INITRAMFS = kernel-bin | lzma | \
	fit lzma $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb with-initrd | pad-to 128k
  KERNEL_ENTRY := 0x20000000
  IMAGE/sysupgrade.bin = append-kernel | fit lzma $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb external-static-with-rootfs | pad-rootfs | append-metadata
endef

define Device/evb
	BLOCKSIZE := 64k
	DEVICE_VENDOR := SIFLOWER
	DEVICE_MODEL := EVB
	DEVICE_DTS := sf21h8898-fullmask-evb
	SUPPORTED_DEVICES := siflower,sf21h8898-evb
endef
TARGET_DEVICES += evb

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
	DEVICE_PACKAGES := kmod-phy-air_en8811h kmod-usb-dwc2 kmod-phy-sf21a6826-usb kmod-rtc-pcf8563
	SUPPORTED_DEVICES := bananapi,bpi-rv2-nand
endef
TARGET_DEVICES += bpi-rv2-nand
