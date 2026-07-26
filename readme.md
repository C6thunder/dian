# PWM Servo 小车 — 循迹 + MaixCAM 摄像头

MSPM0G3507 智能循迹小车，带 MPU6050 航向保持、8路灰度循迹、**MaixCAM Pro 摄像头数字卡片识别**。

## 硬件

| 模块 | 型号 | 接口 |
|------|------|------|
| MCU | MSPM0G3507 (LQFP-64) | 80MHz |
| IMU | MPU6050 (6轴) | 软件 I2C (PA0/PA1) |
| 显示屏 | SSD1306 OLED 128x64 | I2C0 (PA28/PA31) |
| 电机驱动 | TB6612 x2 | TIMG0 PWM (PA12/PA13) |
| 舵机 | — | TIMA0 PWM (PA15) |
| 灰度 | 8路红外 | GPIOB 输入 |
| **摄像头** | **MaixCAM Pro** | **UART2 9600 (PA23/PA24)** |

## 摄像头通信

```
[0x6B] [0x5B] [0x5B] [CMD] [0xB3]   // 5 字节

CMD = 0x00      无检测
CMD = 0x01~0x08 数字 1~8（YOLOv5）
```

MaixCAM Pro 代码: `E:\Downloads\model-127448.maixcam\main.py`

## 接线

```
MaixCAM Pro              小车 UART2
A19 (UART1_TX)     ->    Pin 3 RX (PA24)
A18 (UART1_RX)     ->    Pin 4 TX (PA23)
GND                ->    Pin 2 GND
```

## 文件结构

```
├── main.c                    主循环
├── empty.syscfg              SysConfig 硬件配置
├── BSP/
│   ├── board.c/h             上电初始化
│   ├── camera.c/h            MaixCAM 摄像头 UART
│   ├── motor.c/h             电机 PWM
│   ├── encoder.c/h           编码器
│   ├── control.c/h           循迹 PD 控制
│   ├── pid.c/h               PID 算法
│   ├── soft_timer.c/h        软件定时器
│   ├── struct_typedef.h      全局结构 (Car_t)
│   ├── key.c/h               按键
│   ├── grayscale.c           灰度传感器
│   ├── IMU/                  MPU6050 + DMP
│   └── eMPL/                 E-MPL 库
├── ndrivers/oled.c           OLED 驱动
├── doc/                      项目文档
│   ├── pin_map.md            引脚分配
│   ├── modules.md            模块清单
│   └── uart_connectors.md    串口说明
└── Debug/                    构建输出
```

## OLED 调试屏 (mode 2)

```
num:3 GR:1A         <- 摄像头数字 + 灰度
e: +4 d: +0         <- 误差 + 阻尼
L:850  R:860        <- 左右占空比
rx:1234 f:56 Gz:+0  <- 字节/帧 + 陀螺
```

- SW2(PB1) 切换模式
- LED1(PB8) 收到字节时闪

## 软定时器

| 定时器 | 周期 | 用途 |
|--------|------|------|
| READ_IMU | 10ms | 读 MPU6050 |
| SAMPLE_SPEED | 50ms | 编码器转速 |
| HEADING_HOLD | 10ms | 循迹 PD |
| OLED_REFRESH | 30ms | OLED 刷新 |
| READ_GRAY | 10ms | 灰度传感器 |
| READ_CAMERA | 20ms | 摄像头轮询 |

## 构建

- CCS Theia / CCS 20.2
- MSPM0 SDK 2.11.00.07
- SysConfig 1.26.2
- TI ARM Clang 4.0.3
- J-Link 调试器
