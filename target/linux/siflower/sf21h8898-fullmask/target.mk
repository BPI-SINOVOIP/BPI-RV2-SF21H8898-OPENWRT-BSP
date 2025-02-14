ARCH:=riscv64
SUBTARGET:=sf21h8898-fullmask
BOARDNAME:=siflower sf21h8898 fullmask based boards
CPU_TYPE:=c908
KERNELNAME:=Image
DEFAULT_PACKAGES += opensbi_siflower

define Target/Description
  Build firmware images for siflower SF21H8898 FULLMASK SoC
endef
