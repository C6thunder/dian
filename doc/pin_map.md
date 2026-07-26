# MSPM0G3507 引脚分配表（2026-07-26）

## 芯片信息
- **MCU**: MSPM0G3507 (LQFP-64)
- **CPUCLK**: 80 MHz (HFXT 40MHz × 5 / 2)
- **SDK**: mspm0_sdk 2.11.00.07
- **SysConfig**: 1.26.2

---

## GPIOA

| 引脚 | 功能 | 外设 | 备注 |
|------|------|------|------|
| PA0 | MPU6050_SDA | 软件 I2C | GPIO5 模块，与 UART0_RX 冲突 |
| PA1 | MPU6050_SCL | 软件 I2C | GPIO5 模块，与 UART0_TX 冲突 |
| PA5 | HFXIN | 外部晶振 | 40MHz |
| PA6 | HFXOUT | 外部晶振 | 40MHz |
| PA10 | — | 未使用 | ⚠️ Tianmengxing 默认 UART 引脚 |
| PA11 | — | 未使用 | ⚠️ Tianmengxing 默认 UART 引脚 |
| PA12 | PWMA_C0 | TIMG0 CCP0 | 电机 PWM（电机1 = 右轮） |
| PA13 | PWMA_C1 | TIMG0 CCP1 | 电机 PWM（电机2 = 左轮） |
| PA14 | DC_MOTOR_BB | GPIO 输入 | 编码器 电机1 B相 |
| PA15 | SERVO_C2 | TIMA0 CCP2 | 舵机 PWM |
| PA16 | DC_MOTOR_BIN1 | GPIO 输出 | 电机2 方向控制 |
| PA19 | SWDIO | DEBUGSS | 调试接口 |
| PA20 | SWCLK | DEBUGSS | 调试接口 |
| PA23 | **MaixCAM_TX** | **UART2 TX** | 摄像头通信 @9600 |
| PA24 | **MaixCAM_RX** | **UART2 RX** | 摄像头通信 @9600 |
| PA25 | DC_MOTOR_BA | GPIO 输入 | 编码器 电机1 A相 |
| PA26 | DC_MOTOR_AA | GPIO 输入 | 编码器 电机2 A相 |
| PA27 | DC_MOTOR_AB | GPIO 输入 | 编码器 电机2 B相 |
| PA28 | I2C_0_SDA | I2C0 | OLED 显示屏 (0x3C) |
| PA31 | I2C_0_SCL | I2C0 | OLED 显示屏 (0x3C) |

## GPIOB

| 引脚 | 功能 | 外设 | 备注 |
|------|------|------|------|
| PB0 | KEY11 | GPIO 输入 (上拉) | 按键 SW1 → 航向重置 |
| PB1 | KEY12 | GPIO 输入 (上拉) | 按键 SW2 → 显示模式切换 |
| PB2 | — | 未使用 | |
| PB3 | — | 未使用 | |
| PB5 | GRAY8 | GPIO 输入 | 灰度传感器 CH8 |
| PB6 | KEY9 | GPIO 输入 (下拉) | 按键（未接） |
| PB7 | KEY10 | GPIO 输入 (下拉) | 按键（未接） |
| PB8 | LED1 | GPIO 输出 | 调试 LED，摄像头收到字节时翻转 |
| PB12 | GRAY5 | GPIO 输入 | 灰度传感器 CH5 |
| PB13 | GRAY4 | GPIO 输入 | 灰度传感器 CH4 |
| PB15 | GRAY7 | GPIO 输入 | 灰度传感器 CH7 |
| PB16 | GRAY6 | GPIO 输入 | 灰度传感器 CH6 |
| PB17 | DC_MOTOR_AIN1 | GPIO 输出 | 电机1 方向控制 |
| PB19 | DC_MOTOR_AIN2 | GPIO 输出 | 电机1 方向控制 |
| PB23 | GRAY3 | GPIO 输入 | 灰度传感器 CH3 |
| PB24 | DC_MOTOR_BIN2 | GPIO 输出 | 电机2 方向控制 |
| PB26 | GRAY2 | GPIO 输入 | 灰度传感器 CH2 |
| PB27 | GRAY1 | GPIO 输入 | 灰度传感器 CH1 |

## 可用空闲引脚

| 引脚 | 端口 | 备注 |
|------|------|------|
| PA2 | GPIOA | |
| PA3 | GPIOA | |
| PA4 | GPIOA | |
| PA7 | GPIOA | |
| PA8 | GPIOA | |
| PA9 | GPIOA | |
| PA10 | GPIOA | ⚠️ Tianmengxing 默认 UART_TX |
| PA11 | GPIOA | ⚠️ Tianmengxing 默认 UART_RX |
| PA17 | GPIOA | |
| PA18 | GPIOA | ⚠️ Tianmengxing 特殊引脚 |
| PA21 | GPIOA | ⚠️ Tianmengxing 特殊引脚 |
| PA22 | GPIOA | |
| PA29 | GPIOA | |
| PA30 | GPIOA | |
| PB2 | GPIOB | |
| PB3 | GPIOB | |
| PB4 | GPIOB | |
| PB9 | GPIOB | |
| PB10 | GPIOB | |
| PB11 | GPIOB | |
| PB14 | GPIOB | |
| PB18 | GPIOB | |
| PB20 | GPIOB | |
| PB21 | GPIOB | |
| PB22 | GPIOB | |
| PB25 | GPIOB | |
