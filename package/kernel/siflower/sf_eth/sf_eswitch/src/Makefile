
INC_DIR := intel7084_src/include
SRC_DIR := intel7084_src/src

# intel7084 api build
ifdef CONFIG_SIFLOWER_ESWITCH_LIGHT
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/sf_intel7084_api.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/sf_intel7084_ops.o
else
SFAX8_INTEL7084_SRC_OBJS   := $(SRC_DIR)/gsw_io_wrap.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/gsw_sw_init.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/gsw_api_func.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/gsw_core.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/gsw_pce.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/gsw_vlan.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/gsw_qos.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/gsw_multi.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/sf_intel7084_ops.o
#SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/sf_intel7084_qos_api.o
endif

SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/sf_intel7084_bridge_redirect.o


sf_eswitch-y   += $(SFAX8_INTEL7084_SRC_OBJS)
sf_eswitch-y   += sf_eswitch_driver.o
sf_eswitch-y   += sf_eswitch_debug.o

ccflags-y += -I$(INC_DIR)
# Build as part of Kernel driver
obj-$(CONFIG_SIFLOWER_ESWITCH_DRIVER)	+= sf_eswitch.o
