ARCH:=riscv64
SUBTARGET:=sf21h8898
BOARDNAME:=Siflower SF21H8898 based boards
CPU_TYPE:=c908
KERNELNAME:=Image
DEFAULT_PACKAGES += opensbi_siflower

define Target/Description
  Build firmware images for Siflower SF21H8898 SoCs
endef
