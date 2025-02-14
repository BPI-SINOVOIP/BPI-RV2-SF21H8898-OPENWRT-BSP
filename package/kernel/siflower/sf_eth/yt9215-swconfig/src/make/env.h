export SDK_BASE_PATH=$(shell dirname $(abspath $(lastword $(MAKEFILE_LIST))))/..
#
# Automatically generated file; DO NOT EDIT.
# YT switch sdk configuration
#
# OS_Cyg is not set
export OS_Linux=YES
# OS_FreeRTOS is not set
# User_Mode is not set
export Kernel_Mode=YES

#
# Toolchain configuration
#
export TOOLCHAIN_PATH=
export CROSS_COMPILE=
export CPU_ARCH=
export LINUX_SRC_PATH=
# end of Toolchain configuration

export Tiger=YES
# Board_AUTO_Detect is not set
# Board_YT9215RB_Default_Demo is not set
# Board_YT9215RB_YT8531_Demo is not set
# Board_YT9215S_YT8531_Fib_Demo is not set
export Board_YT9215S_Fib_Demo=YES
# export Board_YT9215SC_Default_Demo=YES
# Board_Customer is not set
# INTER_MCU is not set

#
# Supported phy list
#
# PHY_YT8531 is not set
# end of Supported phy list

# export SDK_MODEL_FULL=YES
export SDK_MODEL_CUST=YES
export YT_SDK_VERSION=

#
# sdk optional
#
# UART is not set
export CTRLIF=YES
# MEM is not set
# GCOV is not set
# SDK_TEST is not set
# end of sdk optional
