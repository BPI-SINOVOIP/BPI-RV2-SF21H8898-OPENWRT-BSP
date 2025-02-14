#***********************************************************
#
# Makefile
#
#***********************************************************
# Set MAKE_PATH according to the compilation environment
export MAKE_PATH := $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST))))/make/


.PHONY: sdk
TARGET_SDK := yt9215_swconfig
obj-m += $(TARGET_SDK).o

ifdef CONFIG_SIFLOWER_MPW1_ESWITCH_LIGHT
include $(MAKE_PATH)/make.path

#SDK_COMPILATION_SWITCHES += -DACC_CTRLIF
SDK_COMPILATION_SWITCHES += -DSYS_INCLUDED
SDK_COMPILATION_SWITCHES += -DCTRLPKT_INCLUDED
SDK_COMPILATION_SWITCHES += -DL2_INCLUDED
SDK_COMPILATION_SWITCHES += -DPORT_INCLUDED
SDK_COMPILATION_SWITCHES += -DPORT_ISO_INCLUDED
SDK_COMPILATION_SWITCHES += -DQOS_INCLUDED
SDK_COMPILATION_SWITCHES += -DRATE_INCLUDED
SDK_COMPILATION_SWITCHES += -DSTAT_INCLUDED
SDK_COMPILATION_SWITCHES += -DVLAN_INCLUDED
SDK_COMPILATION_SWITCHES += -DLAG_INCLUDED
SDK_COMPILATION_SWITCHES += -DBOARD_YT9215S_FIB_DEMO
ccflags-y += $(SDK_COMPILATION_SWITCHES)

SDK_INCLUDES += -I$(COMMON_PATH_INC) -I$(INCLUDE_PATH) -I$(OSAL_PATH) \
				-I$(CORE_HAL_PATH) -I$(CORE_SW_CTRL_PATH) \
				-I$(CORE_FAL_PATH) -I$(CORE_CAL_PATH) -I$(CORE_FAL_PROJECT_PATH) \
				-I$(BOARDPROFILE_PATH) -I$(SDK_BASE_PATH) -I$(CORE_CHIP_DEF_PATH) \
				-I$(CORE_MAC_DRV_PATH) -I$(CORE_PHY_DRV_PATH) -I$(CTRL_PATH_CTRLIF) \
				-I$(FAL_PATH_TIGER)
ccflags-y += $(SDK_INCLUDES)

SDK_SOURCE_OBJECTS += $(PKG_BUILD_DIR)/yt9215s_api.o \
					  $(PKG_BUILD_DIR)/swconfig.o
$(TARGET_SDK)-objs += $(SDK_SOURCE_OBJECTS)

else
include $(MAKE_PATH)/env.h
include $(MAKE_PATH)/env/make.env
include $(MAKE_PATH)/make.path
include $(MAKE_PATH)/make.rule
include $(MAKE_PATH)/make.cfg

ccflags-y := -I$(LINUX_SRC_PATH)/include/linux/
ccflags-y += $(SDK_INCLUDES)

$(TARGET_SDK)-objs += $(SDK_SOURCE_OBJECTS)
ccflags-y += $(SDK_COMPILATION_SWITCHES)

sdk:
ifeq ($(Kernel_Mode), YES)
	make ARCH=$(CPU_ARCH) -C $(LINUX_SRC_PATH) M=$(PWD) modules
endif

clean:
ifeq ($(Kernel_Mode), YES)
	make ARCH=$(CPU_ARCH) -C $(LINUX_SRC_PATH) M=$(PWD) clean
endif

endif
