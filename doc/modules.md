# 项目模块清单（2026-07-26）

## 硬件外设

| 模块 | 芯片/型号 | 接口 | 引脚 | 备注 |
|------|-----------|------|------|------|
| OLED 显示屏 | SSD1306 | I2C0 (400kHz) | PA28(SDA), PA31(SCL) | 128×64, 地址 0x3C |
| MPU6050 | InvenSense | 软件 I2C | PA0(SDA), PA1(SCL) | 6轴 IMU，含 DMP |
| 舵机 | — | TIMA0 PWM | PA15 | 50Hz, 150~850us |
| 电机1（右轮） | DC + 编码器 | TIMG0 PWM + GPIO | PA12(PWM), PA17(AIN1), PA19(AIN2), PA25(编码器A), PA14(编码器B) | TB6612 驱动, 324线编码器 |
| 电机2（左轮） | DC + 编码器 | TIMG0 PWM + GPIO | PA13(PWM), PA16(BIN1), PB24(BIN2), PA26(编码器A), PA27(编码器B) | TB6612 驱动, 324线编码器 |
| 灰度传感器 | 8路红外 | GPIO 输入 | PB27,26,23,13,12,16,15,5 | 数字输出，0=黑线 |
| MaixCAM 摄像头 | Sipeed MaixCAM Pro | UART2 (9600 8N1) | PA23(TX), PA24(RX) | 5字节协议 |
| 按键 | 轻触开关 | GPIO 输入 | PB0(SW1), PB1(SW2) | SW1=航向重置, SW2=显示切换 |
| LED | — | GPIO 输出 | PB8 | 调试指示 |

## SysConfig 模块

| 模块 | 实例名 | 用途 |
|------|--------|------|
| GPIO | GPIO1 (LED) | LED1 @ PB8 |
| GPIO | GPIO2 (KEY) | KEY9/10/11/12 @ PB6,7,0,1 |
| GPIO | GPIO3 (DC_MOTOR) | 电机控制 + 编码器 |
| GPIO | GPIO4 (GRAY) | 8路灰度 @ PB27..PB5 |
| GPIO | GPIO5 (MPU6050_SW_I2C) | 软件 I2C @ PA0,PA1 |
| I2C | I2C_0 | OLED @ PA28,PA31 |
| PWM | SERVO (TIMA0) | 舵机 @ PA15 CCP2 |
| PWM | PWMA (TIMG0) | 电机 PWM @ PA12,PA13 |
| UART | MaixCAM (UART2) | 摄像头 @ PA23,PA24, 9600 |
| SYSCTL | — | 时钟 80MHz |
| Board | — | SWD 调试 @ PA19,PA20 |

## 软件定时器

| 定时器 | 周期 | 用途 |
|--------|------|------|
| SOFT_TIMER_READ_IMU | 10ms | 读 MPU6050 |
| SOFT_TIMER_SAMPLE_SPEED | 50ms | 计算编码器转速 |
| SOFT_TIMER_HEADING_HOLD | 10ms | 循迹/航向 PD 控制 |
| SOFT_TIMER_OLED_REFRESH | 30ms | OLED 刷新 |
| SOFT_TIMER_READ_GRAY | 10ms | 读灰度传感器 |
| SOFT_TIMER_READ_CAMERA | 20ms | 轮询 MaixCAM 摄像头 |

## MaixCAM ↔ MSPM0 通信协议

- **波特率**: 9600-8N1
- **帧格式** (5字节): `[0x6B] [0x5B] [0x5B] [CMD] [0xB3]`
- **CMD**: 0x00=无检测, 0x01~0x08=数字1~8

## 中断优先级

| 外设 | 优先级 | 备注 |
|------|--------|------|
| GPIOA (编码器) | 0 (最高) | GROUP1 共享 |
| GPIOB (按键) | 0 | GROUP1 共享 |
| SysTick | 2 | 1ms 时基 |
| UART2 (MaixCAM) | 3 (最低) | 摄像头通信 |
