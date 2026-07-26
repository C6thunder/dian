################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
ndrivers/%.o: ../ndrivers/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/CCS_20_2/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO/BSP" -I"C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO/BSP/IMU" -I"C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO/ndrivers" -I"C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO" -I"C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO/Debug" -I"D:/CCS_20_2/mspm0_sdk_2_11_00_07/source/third_party/CMSIS/Core/Include" -I"D:/CCS_20_2/mspm0_sdk_2_11_00_07/source" -gdwarf-3 -Wall -MMD -MP -MF"ndrivers/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '
