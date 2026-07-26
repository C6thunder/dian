################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# v4.22：删 ../ndrivers/uart.c（MPU 是 I2C 模块，不再用硬件 UART 收 MPU 数据）。
#  - 子目录目前只剩 oled.c 一个源文件。
C_SRCS += \
../ndrivers/oled.c \

C_DEPS += \
./ndrivers/oled.d \

OBJS += \
./ndrivers/oled.o \

OBJS__QUOTED += \
"ndrivers\oled.o" \

C_DEPS__QUOTED += \
"ndrivers\oled.d" \

C_SRCS__QUOTED += \
"../ndrivers/oled.c" \
