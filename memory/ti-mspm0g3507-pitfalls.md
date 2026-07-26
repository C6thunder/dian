---
name: ti-mspm0g3507-pitfalls
description: SysConfig / driverlib quirks specific to MSPM0G3507 + this project that cost time to discover
metadata: 
  node_type: memory
  type: reference
  originSessionId: 0d00e125-1389-4764-9148-f4595d7d8630
  modified: 2026-07-22T09:48:57.361Z
---

MSPM0G3507 + SysConfig 1.26.x 几个易踩的坑：

1. **I2C 引脚复用限制**：PA0/PA1 只能用作 I2C0 的 SDA/SCL。若 I2C0 已被 OLED 占用，SysConfig 会拒绝给 PA0/PA1 配其他 I2C 实例。解决：把它们当普通 GPIO 用，自己写软件 I2C 位操作。

2. **SysConfig 宏名顺序**：是 `MPU6050_SDA_IOMUX`（pin name + `_IOMUX`），**不是** `MPU6050_IOMUX_SDA`。所有 GPIO 引脚的 IOMUX 宏都遵循这个顺序。

3. **PA10 只能 TX、PA11 只能 RX**：SysConfig 校验会拒绝 PA10 当 RX 用。UART 要串起来就 PA10=TX、PA11=RX。

4. **snprintf 缓冲区**：模板里 `char line[24]` 在格式化多个 int/float 时会触发 truncation warning。统一用 `char line[32]`。

5. **GPIO IRQ 共享**：同一 GPIO 端口的所有引脚共用一个 NVIC。PB0/PB1/PB6/PB7 全在 GPIOB 上 → 共用 `KEY_INT_IRQN` → ISR 用 `DL_GPIO_getPendingInterrupt(GPIOB)` 读位掩码后 `if` 分发。

6. **默认 makefile 的 -I 只有项目根**：CCS 自动生成的 `Debug/subdir_rules.mk` 里 -I 只给 `project_root` / `user_driver` / `Debug` / SDK。**不**给 `BSP/`、`ndrivers/` 等子目录。所以 `.c` 写 `#include "Delay.h"` 在子目录里能找到（同目录搜索），但**跨子目录** include 必须写 `#include "BSP/Delay.h"` 这种项目根相对路径。

7. **改 include 路径不要动 makefile**：如果想恢复"子目录有它自己的 -I"，很容易破坏自动重新生成的 makefile。但有个简单办法：在 `Debug/makefile.init`（被 `-include ../makefile.init` 自动包含）里加 CFLAGS；但当前 compile 命令是硬编码，不读 CFLAGS。**最简单**：源码里用 `#include "BSP/X.h"` 项目根相对路径，零构建系统改动。

8. **inv_mpu.c 8 个 delay_ms 警告**：InvenSense MotionDriver 原代码用 MSP430 的 `delay_1ms` 宏（来自 `inv_mpu_dmp_motion_driver.c` 的 `MOTION_DRIVER_TARGET_MSP430` 分支），那个分支只在那个 .c 里生效。**inv_mpu.c 调用的是我们 BSP/Delay.c 的 `delay_ms`**——所以 inv_mpu.c 需要 `#include "BSP/Delay.h"` 才能拿到原型，否则永远是 `-Wimplicit-function-declaration` warning。

9. **`#include "board.h"` 在 inv_mpu_dmp_motion_driver.c 里是死的**：原参考项目 board.h 给一堆 MSP430 / log_i / log_e。我们走 `MOTION_DRIVER_TARGET_MSP430` 分支，里面 4 个宏都直接 redef，没有从 board.h 拿任何东西。**直接删 `#include "board.h"`**——如果之后要 `EMPL_TARGET_*` 分支，再加回来。

**Why:** 这些不是文档第一眼能看出来的，每次遇到都要查 + 实验。汇总在这里避免重复踩。

**How to apply:** 配新引脚 / 写新 I2C / 加新按键 / 改 snprintf 前先扫一遍这条。

相关：[[project-readme-pointer]]