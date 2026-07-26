/* ===========================================================================
 * IMU.h
 *  - 高层 IMU 接口（基于 InvenSense MotionDriver / DMP）
 *  - 启动：IMU_init()
 *  - 每帧：IMU_getData(pitch, roll, yaw)
 * =========================================================================*/
#ifndef IMU_H
#define IMU_H

#include <stdint.h>

/* 初始化 MPU6050 + DMP；返回 0 成功 */
uint8_t IMU_init(void);

/* 读取 DMP 解算出的欧拉角（°）
 * pitch / roll / yaw 由调用者提供指针 */
uint8_t IMU_getData(float *pitch, float *roll, float *yaw);

/* DMP 内置的原始陀螺（°/s）
 * v4.14：另输出 3 个 int16 原始寄存器值，O:/D:/_raw 看 I2C 链路到底是 OK 还是 NACK */
uint8_t IMU_getGyro(float *gx, float *gy, float *gz,
                    int16_t *gx_raw, int16_t *gy_raw, int16_t *gz_raw);

/* v4.16.2：wakeup 之后 PWR_MGMT_1 / PWR_MGMT_2 实际值，给 OLED 状态条读 */
extern uint8_t g_pwr_mgmt_1;
extern uint8_t g_pwr_mgmt_2;

#endif /* IMU_H */