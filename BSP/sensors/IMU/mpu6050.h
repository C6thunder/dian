#ifndef MPU6050_H
#define MPU6050_H

#include "ti_msp_dl_config.h"

// MPU6050 I2C 地址：ADD=GND → 0x68
#define MPU6050_ADDR             0x68U

// MPU6050 寄存器
#define MPU6050_REG_SMPLRT_DIV   0x19U
#define MPU6050_REG_CONFIG       0x1AU
#define MPU6050_REG_GYRO_CONFIG  0x1BU
#define MPU6050_REG_ACCEL_CONFIG 0x1CU
#define MPU6050_REG_INT_PIN_CFG  0x37U
#define MPU6050_REG_INT_ENABLE   0x38U
#define MPU6050_REG_ACCEL_XOUT_H 0x3BU   // 连续 14 字节：accel(6) + temp(2) + gyro(6)
#define MPU6050_REG_PWR_MGMT_1   0x6BU
#define MPU6050_REG_PWR_MGMT_2   0x6CU  /* v4.16：XYZ gyro + Accel standby 控制 */
#define MPU6050_REG_GYRO_XOUT_H  0x43U   // 连续 6 字节: Gx(2)/Gy(2)/Gz(2)
#define MPU6050_REG_WHO_AM_I     0x75U
#define MPU6050_WHO_AM_I_VAL     0x68U   // 标准 MPU6050
#define MPU6500_WHO_AM_I_VAL     0x70U   // v4.12：用户模块实际是 MPU6500（兼容寄存器）
#define WHO_AM_I_ACCEPTED(w)     ((w) == MPU6050_WHO_AM_I_VAL || (w) == MPU6500_WHO_AM_I_VAL)

// 量程换算系数
// 加速度 ±2g 时为 16384 LSB/g；±4g 为 8192；±8g 为 4096；±16g 为 2048
// 陀螺仪 ±250°/s 时为 131 LSB/(°/s)；±500 为 65.5；±1000 为 32.8；±2000 为 16.4
#define MPU6050_ACCEL_SENS       16384.0f
#define MPU6050_GYRO_SENS        131.0f

typedef struct {
    // 原始 16 位读数
    int16_t ax, ay, az;     // 加速度原始值
    int16_t temp;            // 温度原始值
    int16_t gx, gy, gz;      // 陀螺仪原始值

    // 物理量
    float   ax_g,  ay_g,  az_g;    // 加速度 (g)
    float   gx_dps,gy_dps,gz_dps;  // 角速度 (°/s)
    float   temp_c;                // 温度 (℃)

    // 姿态角（互补滤波后）
    float   pitch_accel, roll_accel;  // 由加速度瞬时计算
    float   pitch, roll;              // 互补滤波输出 (°)
    float   yaw;                       // 仅陀螺仪积分的航向角 (°)
} MPU6050_Data_t;

/**
 * @brief 初始化 MPU6050：唤醒、设置采样率/低通/量程
 * @return 0 失败（WHO_AM_I 不对），1 成功
 */
uint8_t mpu6050_init(void);

/**
 * @brief 一次性读取所有 14 字节传感器数据并换算到物理量与加速度角
 */
void mpu6050_read(MPU6050_Data_t *data);

/**
 * @brief 用 dt (秒) 推进互补滤波器，更新 pitch/roll/yaw
 */
void mpu6050_update_attitude(MPU6050_Data_t *data, float dt);

/* eMPL/DMP 适配层用到：单字节读写 */
uint8_t mpu6050_write_byte(uint8_t reg, uint8_t val);
uint8_t mpu6050_read_byte(uint8_t reg, uint8_t *val);

/* v4.11：WHO_AM_I 原始字节（mpu6050_init() 里赋值）
 *   0x68 → 真 MPU6050 应答
 *   0x70 → 真 MPU6500 应答（v4.12：用户模块实际是 MPU6500）
 *   0x00 → SDA 短路到地 / 总线浮空低电平
 *   0xff → SDA 浮空赢上拉（MPU 没接 / NACK 持续）
 *   其他 → AD0 接错 / 挂错芯片 */
extern uint8_t g_mpu6050_who;

/* v4.13：直接读陀螺仪寄存器（不走 eMPL/dmp 库的 sensor mask 检查，
 * 这样即使 DMP 没起来也能拿到 gz）。
 *
 * v4.23：底层从硬件 I2C0 改成软件 I2C 位操作（PA0 SDA / PA1 SCL）。
 * 接口签名不变，调用方（IMU.c / main.c）不用改。 */
uint8_t mpu6050_read_gyro_raw(int16_t *gx, int16_t *gy, int16_t *gz);

#endif // MPU6050_H