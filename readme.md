# MSPM0 PWM 舵机 + PD 循迹小车

TI MSPM0G3507 两轮差速循迹小车，基于 CCS Theia + SysConfig + DriverLib。

## 硬件

- **主控**: MSPM0G3507 (LQFP-64)
- **电机驱动**: TB6612 × 2
- **编码器**: MG513X (13 PPR × 28:1 = 364 count/rev, 1× 解码)
- **灰度传感器**: 8 路数字灰度 (平行 GPIO 直读)
- **IMU**: MPU6050 (软件 I2C, PA0/PA1)
- **摄像头**: MaixCAM UART2 (PA23/PA24, 5 字节协议)
- **OLED**: 0.96" I2C (PA28/PA31)
- **轮径**: 65mm

## 循迹控制 (`BSP/control.c`)

纯 PD 控制器，结构极简：

```
err_raw = 8 路灰度加权平均 (-40 ~ +40)
err_f   = EMA(err_raw, alpha=0.4)
corr    = Kp×err_f + Kd×Δerr_f
L       = BASE - corr
R       = BASE + corr
```

### 参数 (`control.c` 顶部宏)

| 参数 | 值 | 说明 |
|------|-----|------|
| `TRACE_KP` | 10 | 比例系数 |
| `TRACE_KD` | 20 | 微分系数 |
| `TRACE_BASE` | 1100 | 基础速度 (CCR 0~4000) |
| `TRACE_EMA` | 0.4 | EMA 平滑系数 |
| `TRACE_DEAD` | 3 | 死区 (\|corr\|<3 不修正) |
| `TRACE_MIN/MAX` | 300/3500 | duty 限幅 |

### 启停逻辑

```
上电 → 出发 → 跑 ~4m → 2 帧内 ≥3 个不同传感器压黑 → 急停
                     → 4.5m 强制兜底停
```

| 参数 | 值 | 说明 |
|------|-----|------|
| `TARGET_DIST_MM` | 4000 | 开始检测距离 |
| `MAX_DIST_MM` | 4500 | 强制停止距离 |
| `STOP_UNIQ_BLACK` | 3 | 2 帧并集中 ≥3 个不同灯黑 = 停止线 |

### 航向保持

PD 控制，用陀螺仪 Z 轴角速度做 D 项阻尼。

## 目录结构

```
├── main.c              # 主循环调度器 (1ms SysTick + 软定时器)
├── BSP/
│   ├── control.c/h     # PD 循迹 + 航向保持
│   ├── motor.c/h       # 电机 PWM + 方向 (TB6612)
│   ├── encoder.c/h     # 编码器 (霍尔 × 2)
│   ├── grayscale.c     # 8 路灰度传感器
│   ├── board.c/h       # 上电初始化 + SysTick
│   ├── soft_timer.c/h  # 软件定时器
│   ├── pid.c/h         # 通用 PID 库 (DJI 风格, 循迹未使用)
│   ├── IMU/            # MPU6050 + eMPL DMP
│   └── ...
├── empty.syscfg        # SysConfig 项目文件
└── ti_msp_dl_config.*  # SysConfig 生成 (勿手动编辑)
```

## 开发

- IDE: CCS Theia
- SDK: MSPM0 SDK + SysConfig
- 编译: 在 CCS Theia 中 Build
- 烧录: J-Link / XDS110 (通过 DSLite)

## 运行模式

| car.run_mode | 模式 |
|-------------|------|
| 0 | STOP (停车) |
| 1 | HOLD (航向保持) |
| 2 | TRACE (循迹, 默认) |

| car.disp_mode | OLED 显示 |
|--------------|----------|
| 0 | 速度屏 + IMU 状态 |
| 1 | IMU 姿态大字 |
| 2 | 循迹调试 (灰度/duty/陀螺) |
