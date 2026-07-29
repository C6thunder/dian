################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

INC_PATH = -I"C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO/BSP" \
-I"C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO/BSP/motor" \
-I"C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO/BSP/control" \
-I"C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO/BSP/sensors" \
-I"C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO/BSP/sensors/IMU" \
-I"C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO/BSP/sensors/eMPL" \
-I"C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO/BSP/utils" \
-I"C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO/ndrivers" \
-I"C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO" \
-I"C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO/Debug" \
-I"D:/CCS_20_2/mspm0_sdk_2_11_00_07/source/third_party/CMSIS/Core/Include" \
-I"D:/CCS_20_2/mspm0_sdk_2_11_00_07/source"

# Each subdirectory must supply rules for building sources it contributes
BSP/%.o: ../BSP/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/CCS_20_2/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 $(INC_PATH) -gdwarf-3 -Wall -MMD -MP -MF"BSP/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

BSP/motor/%.o: ../BSP/motor/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/CCS_20_2/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 $(INC_PATH) -gdwarf-3 -Wall -MMD -MP -MF"BSP/motor/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

BSP/control/%.o: ../BSP/control/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/CCS_20_2/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 $(INC_PATH) -gdwarf-3 -Wall -MMD -MP -MF"BSP/control/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

BSP/sensors/%.o: ../BSP/sensors/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/CCS_20_2/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 $(INC_PATH) -gdwarf-3 -Wall -MMD -MP -MF"BSP/sensors/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

BSP/sensors/IMU/%.o: ../BSP/sensors/IMU/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/CCS_20_2/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 $(INC_PATH) -gdwarf-3 -Wall -MMD -MP -MF"BSP/sensors/IMU/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

BSP/sensors/eMPL/%.o: ../BSP/sensors/eMPL/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/CCS_20_2/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 $(INC_PATH) -gdwarf-3 -Wall -MMD -MP -MF"BSP/sensors/eMPL/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

BSP/utils/%.o: ../BSP/utils/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/CCS_20_2/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 $(INC_PATH) -gdwarf-3 -Wall -MMD -MP -MF"BSP/utils/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '
