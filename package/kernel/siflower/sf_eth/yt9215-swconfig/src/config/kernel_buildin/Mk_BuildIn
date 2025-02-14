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

# include path
EXTRA_CFLAGS += $(SDK_INCLUDES)

#macro
EXTRA_CFLAGS += $(SDK_COMPILATION_SWITCHES) 

# objects
obj-y := $(SDK_SOURCE_OBJECTS)