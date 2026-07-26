# MSPM0G3507 智能车项目 — 上下文记忆

> 本文件用于在另一台电脑 / 另一个 Claude 会话中快速恢复本项目状态。
> 任何修改代码前请先读完本文件，避免破坏已有约定。

---

## 1. 项目概况

- **MCU**：MSPM0G3507（LQFP-64 / PM 封装，Cortex-M0+，80 MHz）
- **车型**：两轮差速小车（"天猛星"）
- **当前阶段**：航向保持直行 + **DMP 姿态解算** + 双模 OLED + 灰度传感器（已落驱动，未接控制环）
- **SDK / IDE**：TI MSPM0 SDK 2.11.00.07 + CCS 20.2
- **构建方式**：命令行 SysConfig + clang + make（不依赖 CCS GUI）
- **代码组织**：**三层结构** — `BSP/`（板级） + `ndrivers/`（HAL） + 顶层 `main.c`

---

## 2. 目录结构（**最新版**）

```
09_PWM_SERVO/
├── empty.syscfg                  # SysConfig 主配置
├── readme.md                     # ← 本文件
├── main.c                        # 顶层 flag 调度循环
├── memory/                       # 项目级记忆（可整体带走）
│
├── BSP/                          # 板级支持包
│   ├── struct_typedef.h          # Car_t + fp32/bool_t 别名
│   ├── soft_timer.h/c            # flag 调度器（5 路 ms 软定时器）
│   ├── board.h/c                 # board_init() 总入口 + SysTick_Handler
│   ├── control.h/c               # heading_hold PD（吃 car.yaw/gz_dps）
│   ├── pid.h/c                   # DJI 通用 PID（POSITION/DELTA/POSITION_ANGLE）
│   ├── Delay.h/c                 # delay_ms / delay_cycles
│   ├── encoder.h/c               # 编码器（双电机）
│   ├── key.h/c                   # 按键事件（KEY11/12）
│   ├── motor.h/c                 # 电机驱动
│   ├── grayscale.c               # 8 路灰度（PB8/PB9 串行）
│   ├── gw_grayscale_sensor.h
│   ├── IMU/
│   │   ├── IMU.h/c               # 高层 DMP 接口
│   │   └── mpu6050.h/c           # MPU6050 软件 I2C + DMP 字节读写
│   └── eMPL/                     # InvenSense MotionDriver（DMP 库，~5400 行）
│       ├── inv_mpu.c
│       ├── inv_mpu.h
│       ├── inv_mpu_dmp_motion_driver.c/h
│       ├── dmpKey.h / dmpmap.h
│       └── dmp_port.h/c          # 平台适配层（MPU6050_WriteReg/ReadData + mget_ms）
│
├── ndrivers/                     # 通用外设 HAL（与板级解耦）
│   ├── oled.c/h
│   └── uart.c/h
│
└── Debug/                        # 构建输出
    ├── makefile                  # 显式 per-file 编译规则
    └── 09_PWM_SERVO.out          # 烧录文件
```

> 老的 `user_driver/` 目录已废弃，文件全部迁到 `BSP/` / `ndrivers/`。

---

## 3. 工具链路径（关键！换电脑必须改这里）

| 工具 | 当前路径 |
|------|----------|
| SysConfig CLI | `D:/CCS_20_2/sysconfig_1.26.2/sysconfig_cli.bat` |
| C 编译器 | `D:/CCS_20_2/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe` |
| Make | `D:/CCS_20_2/ccs/utils/bin/gmake.exe` |
| SDK 元数据 | `D:/CCS_20_2/mspm0_sdk_2_11_00_07/.metadata/product.json` |
| 板级 SDK | `D:/CCS_20_2/mspm0_sdk_2_11_00_07/source` |

### 重新生成配置（修改 empty.syscfg 后必跑）

```bash
cd C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO
"D:/CCS_20_2/sysconfig_1.26.2/sysconfig_cli.bat" \
    --product "D:/CCS_20_2/mspm0_sdk_2_11_00_07/.metadata/product.json" \
    --script empty.syscfg --output Debug --context system
```

### 编译

```bash
cd C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO/Debug
"D:/CCS_20_2/ccs/utils/bin/gmake.exe" -k all
```

输出：`Debug/09_PWM_SERVO.out`（烧录用，当前 ~420KB）。

---

## 4. 运行时架构（**核心**）

### 主循环结构

```c
while (1) {
    __WFI();   // 等中断，省电

    // 1) 按键事件（同步处理）
    uint8_t ev = key_get_events();
    if (ev & KEY_EVENT_YAW_RESET)      { g_yaw = 0; ... }
    if (ev & KEY_EVENT_DISPLAY_TOGGLE) { car.disp_mode ^= 1; OLED_Clear(); }

    // 2) 各 flag 定时任务（SysTick 1ms 推进 g_tick_ms + soft_timer_tick()）
    if (soft_timer_is_timeout(SOFT_TIMER_READ_IMU))     { soft_timer_reset(); IMU_getData(...); ... }
    if (soft_timer_is_timeout(SOFT_TIMER_SAMPLE_SPEED)) { soft_timer_reset(); encoder_sample_speed(50); ... }
    if (soft_timer_is_timeout(SOFT_TIMER_HEADING_HOLD)) { soft_timer_reset(); control_heading_hold(); motor_set_duty(...); }
    if (soft_timer_is_timeout(SOFT_TIMER_OLED_REFRESH)) { soft_timer_reset(); OLED_Refresh(); }
    if (soft_timer_is_timeout(SOFT_TIMER_READ_GRAY))    { soft_timer_reset(); car.gray_raw = gw_gray_serial_read(); ... }
}
```

### 时基（SysTick 1ms 中断）

`board.c` 里 `board_systick_init()` 调 `SysTick_Config(CPUCLK_FREQ/1000)`（80 MHz → 80000 tick/1ms）。
中断里做两件事：`g_tick_ms++` + `soft_timer_tick()`。
**主循环不再 delay**，靠 `__WFI()` 等中断醒来检查 flag。

### 软定时器（5 路）

| ID | 周期 | 用途 |
|----|------|------|
| `SOFT_TIMER_READ_IMU` | 20 ms | DMP 读欧拉角 + 陀螺 |
| `SOFT_TIMER_SAMPLE_SPEED` | 50 ms | encoder_sample_speed → rpm / m/s |
| `SOFT_TIMER_HEADING_HOLD` | 10 ms | heading_hold PD 计算 + 写电机 |
| `SOFT_TIMER_OLED_REFRESH` | 50 ms | OLED_Refresh |
| `SOFT_TIMER_READ_GRAY` | 20 ms | gw_gray_serial_read 8 路 |

### Car_t（单一数据源）

所有模块不再持有自己的"私有 IMU/速度/电机"状态，全写到全局 `Car_t car`：
- `car.{pitch, roll, yaw, gz_dps}` — 来自 DMP
- `car.{left/right_count, rpm, ms, duty}` — 来自编码器/电机
- `car.gray[8] / car.gray_raw` — 来自灰度
- `car.{run_mode, disp_mode}` — 显示/运行模式

---

## 5. 已使用引脚（**绝不能再占用**）

| 引脚 | 功能 | 备注 |
|------|------|------|
| PA5 / PA6 | HFXT 40 MHz 晶振 | SYSCTL |
| PA10 / PA11 | UART0（PRINT，115200） | 注意 PA10 只能做 TX、PA11 只能做 RX |
| PA12 / PA13 | TIMG0 PWM 通道 0/1 | 电机 1 / 电机 2 速度 |
| PA14 | BB（电机 2 编码器 B 相） | |
| PA15 | LED1 | |
| PA16 | BIN1（电机 2 方向） | |
| PA19 / PA20 | SWD（调试） | |
| PA21 | TIMA0 CCP0（舵机 PWM） | |
| PA23 | VREF | |
| PA25 / PA26 / PA27 | 编码器 BA / AA / AB | 电机 1/2 |
| PA28 / PA31 | I2C0 SDA / SCL（OLED） | |
| PA0 / PA1 | **MPU6050 软件 I2C** | 见 §8 第 1 条 |
| PB0 | KEY11 → SW1 → 航向重置 | |
| PB1 | KEY12 → SW2 → OLED 模式切换 | |
| PB6 / PB7 | KEY9 / KEY10（syscfg 挂着，未接） | |
| PB8 / PB9 | **灰度传感器 DAT(输入)/CLK(输出)** | GanWei 辅助板串行接口 |
| PB17 / PB19 / PB24 | AIN1 / AIN2 / BIN2 | |

## 6. 空闲 IO（外扩排针，可接按键板等）

**H56 排针（10 pin）**：PB27, PB26, PB23, PB13, PB12, **PA11(占)**, PB16, **PA10(占)**, PB15, PB5
**H55 排针（10 pin）**：PB25, PB18, PB21, PB22, PA30, **PB0(占)**, **PB1(占)**, PB10, PB11, PB14
**H58 / H59**：4 pin，含 +5V，可供电

注意 PA10/PA11 虽然在 H56 上，但已被 UART 占。

---

## 7. 按键板（10 键 KH-6X6X5H-STM）

- **H1**：10 pin 排针（PZ254V-11-10P），每个脚对应 SW1~SW10 一侧
- **H2**：2 pin 排针（PZ254V-11-02P），公共端（GND）
- **按键触发**：按下短路到 GND，配 PULL_UP + FALL 中断

### 当前按键功能分配

| 按键 | 引脚 | 功能 |
|------|------|------|
| SW1 | PB0 (KEY11) | 航向重置（`g_yaw = 0`，重新定义"正前方"） |
| SW2 | PB1 (KEY12) | OLED 模式切换（全信息 ⇄ IMU 专用） |
| SW3~SW10 | 待用 | 可接 前进/后退 / 调速± / 紧急停止 |

### 扩展按键的步骤

1. `empty.syscfg` 里 `GPIO2.associatedPins.create(N+1)`，加一行 `KEYxx`（PULL_UP + FALL）
2. 跑 §3 的 SysConfig 命令重新生成
3. `BSP/key.h` 加一个 `KEY_EVENT_xxx` 宏
4. `BSP/key.c` 的 `GROUP1_IRQHandler` 加一行分发
5. `main.c` 在 while 循环里处理这个事件

---

## 8. 关键代码约定（**踩过的坑**）

### ① MSPM0G3507 的 I2C 引脚复用

PA0/PA1 **只能**复用为 I2C0（SDA/SCL），而 I2C0 已经被 OLED 占用，所以：
- SysConfig 里 **不能** 给 PA0/PA1 加 I2C 实例
- 解决方案：把 PA0/PA1 当成普通 GPIO 用，自己写软件 I2C（位操作），见 `BSP/IMU/mpu6050.c`
- 软件 I2C 必须能在读写之间切 SDA 方向：`DL_GPIO_initDigitalOutput` / `DL_GPIO_initDigitalInputFeatures`

### ② SysConfig 生成的宏名顺序

`MPU6050_SDA_IOMUX`（port-pin 在前），不是 `MPU6050_IOMUX_SDA`。所有 GPIO 引脚的 IOMUX 宏都遵循：pin 名 → `_IOMUX` 后缀。

### ③ KEY 引脚全部走 GPIOB → GROUP1_IRQHandler

PB0、PB1、PB6、PB7 全部在 GPIOB 上，**共享** `KEY_INT_IRQN`。ISR 里用 `DL_GPIO_getPendingInterrupt(GPIOB)` 读位掩码，再 `if (mask & KEY_KEYxx_IIDX)` 分发。

### ④ snprintf 缓冲区

`snprintf` 把 `char line[24]` 截断警告很常见，统一用 **`char line[32]`** 解决。

### ⑤ PA10 / PA11 只能 TX / RX 单向

SysConfig 校验时会拒绝："No option named PA10 defined"（RX 位置）。PA10 只能做 TX、PA11 只能做 RX。

### ⑥ 默认 makefile 只给项目根的 -I

CCS 自动生成的 `Debug/subdir_rules.mk` 里 `-I` 只给 **项目根** (`09_PWM_SERVO`)、`user_driver`、`Debug`、SDK。**不**给 `BSP/`、`ndrivers/` 等子目录。所以同目录搜索能找到同文件夹的 `.h`（如 `BSP/board.c` 写 `#include "Delay.h"` 找到 `BSP/Delay.h`），但**跨子目录** include 必须用项目根相对路径：

```c
/* 项目根的 main.c */
#include "BSP/Delay.h"          /* ✅ 通过 -I 项目根找到 */
#include "ndrivers/oled.h"      /* ✅ 同上 */

/* BSP/IMU/IMU.c */
#include "BSP/eMPL/inv_mpu.h"   /* ✅ */

#include "../eMPL/inv_mpu.h"    /* ⚠️ 也行，但靠文件系统回退，不如项目根相对稳 */
```

**不要**改 `makefile` 加 `-I` 路径——容易破坏自动重新生成。改 `.c` 文件用项目根相对路径就行。

### ⑦ 注释里禁止出现 `*/`

`BSP/eMPL/dmp_port.c` 第一次写注释时把 `MPU6050_*/mget_ms` 写在 `/* ... */` 里，`*/` 提前关闭注释，编译器把 `mget_ms` 当成代码 → 报 unknown type。**所有注释文本里都不能出现裸 `*/`**。

### ⑧ inv_mpu.c 必须 include Delay.h

InvenSense MotionDriver 里 `inv_mpu_dmp_motion_driver.c` 的 `MOTION_DRIVER_TARGET_MSP430` 分支把 `delay_ms` 重定义成 `delay_1ms`（仅在该 .c 生效）。但 `inv_mpu.c` 自己的 `delay_ms(100)` 调的是**我们**的 `BSP/Delay.c` 的 `delay_ms()`——所以 inv_mpu.c 需要 `#include "BSP/Delay.h"` 拿原型，否则 8 处 `-Wimplicit-function-declaration`。

### ⑨ inv_mpu_dmp_motion_driver.c 别 include board.h

原参考的 `board.h` 给了一堆 MSP430/log_i/log_e 等平台宏。但我们走 `MOTION_DRIVER_TARGET_MSP430` 分支，里面 4 个宏直接 redef，根本不读 board.h。**直接删 `#include "board.h"`**——之后要走 `EMPL_TARGET_*` 分支再补回来。

---

## 9. 当前实现的功能

### DMP 姿态解算（**已上线**）
- 文件：`BSP/eMPL/` + `BSP/eMPL/dmp_port.c`（适配层）+ `BSP/IMU/IMU.c`（高层封装）
- `IMU_init()` → `mpu_init()` + `mpu_dmp_init()`（加载 InvenSense DMP 固件到 MPU6050）
- `IMU_getData(pitch, roll, yaw)` → `mpu_dmp_get_data()` 直接拿欧拉角（°）
- `IMU_getGyro(gx, gy, gz)` → `mpu_get_gyro_reg()` 拿原始陀螺（°/s，量程 ±2000）
- 取代了原来的互补滤波（yaw 漂移更小）

### flag 调度器（**已上线**）
- 文件：`BSP/soft_timer.c/h`
- 基于 `g_tick_ms` ms 时基（SysTick 1ms 中断维护）
- 5 路重复定时器，`is_timeout` 标志由消费者重置

### 航向保持（PD 直行）
- 文件：`BSP/control.c`
- `BASE_DUTY = 900`，`DUTY_MIN = 400`，`DUTY_MAX = 2400`
- `HEADING_KP = 10.0`，`HEADING_KD = 1.5`，`DEAD_DEG = 1.0`，`CLAMP_DEG = 30.0`
- 默认两电机同方向直行，`car.yaw = 0` 作为目标航向

### DJI 通用 PID
- 文件：`BSP/pid.c/h`
- 三种模式：`PID_POSITION` / `PID_DELTA` / `PID_POSITION_ANGLE`（±180° 过零）
- 给循迹环的速度环 / 角度环留位置

### 灰度传感器
- 文件：`BSP/grayscale.c` + `BSP/gw_grayscale_sensor.h`
- 引脚：DAT=PB8（输入，上拉） / CLK=PB9（输出）
- 时序：8 个 CLK 上升沿读 DAT，参考 GanWei 辅助板 MSPM0G3507 串行例程
- 写到 `car.gray[8]`（每路 0/1）和 `car.gray_raw`（8 位打包）
- **驱动已就绪，控制环未接**（下一步：8 路 → 位置偏差 → 差速）

### 编码器测速
- 文件：`BSP/encoder.c`
- 50 ms 采样周期，换算 rpm 与 m/s，写到 `car.left/right_*`

### OLED 双模显示
- **模式 0（全信息）** size=12：
  ```
  M1:  450 rpm  0.32 m/s
  M2:  448 rpm  0.32 m/s
  Cnt: M1= +1234 M2= +1230
  P:  +1.2 R:  -0.3 Y:  +12
  ```
- **模式 1（IMU 专用）** size=16，4 行大字：
  ```
  P :  +1.2        ← Pitch (°)
  R :  -0.3        ← Roll (°)
  Y :  +12         ← Yaw (°)
  Gz:  -0.8/s      ← Gyro Z (°/s)
  ```
- 按 SW2 切换，切换瞬间 `OLED_Clear()` 防残影

---

## 10. 待办 / 后续可加功能

- **循迹环**：8 路灰度 → 位置偏差 → PID（用 `BSP/pid.c`）→ 差速写入 control.c
- SW3~SW10 接前进/后退 / 调速 / 急停
- 串口 PRINT 把 `car` 数据吐出来调试
- DMP 自检（`run_self_test`）做陀螺零偏校准
- OLED 加第 3 模式：灰度条 + 编码器柱状图

---

## 11. 参考资料

- **DMP 库原版**：`C:\Users\thunder\Desktop\电赛\示例\Electronics-Design-Contest---2024\BSP\eMPL\`
- **PID 原版**：`C:\Users\thunder\Desktop\电赛\示例\Electronics-Design-Contest---2024\BSP\pid.c/h`
- **灰度接口参考**：`C:\Users\thunder\Desktop\电赛\示例\MSPM0L1306辅助板开源\MSPM0L1306辅助板开源\辅助板MSPM0G3507串行例程\examples\gpio_toggle_output.c`