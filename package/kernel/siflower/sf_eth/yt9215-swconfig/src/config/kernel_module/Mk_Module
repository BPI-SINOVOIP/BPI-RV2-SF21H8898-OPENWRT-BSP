#***********************************************************
#
# Makefile
#
#***********************************************************
# set MAKE_PATH according to the compilation environment
export MAKE_PATH := $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST))))
include $(MAKE_PATH)/env.h
include $(MAKE_PATH)/env/make.env
include $(MAKE_PATH)/make.path
include $(MAKE_PATH)/make.rule
include $(MAKE_PATH)/make.cfg

.PHONY: sdk

TARGET_SDK = sdk

obj-m += $(TARGET_SDK).o

$(TARGET_SDK)-objs += $(SDK_SOURCE_OBJECTS)

ccflags-y := -I$(LINUX_SRC_PATH)/include/linux/

ccflags-y += $(SDK_INCLUDES)

ccflags-y += $(SDK_COMPILATION_SWITCHES)


sdk:
ifeq ($(Kernel_Mode), YES)
	make ARCH=${CPU_ARCH} -C $(LINUX_SRC_PATH) M=$(PWD) modules
endif

clean:
ifeq ($(Kernel_Mode), YES)
	make ARCH=$(CPU_ARCH) -C $(LINUX_SRC_PATH) M=$(PWD) clean
endif


