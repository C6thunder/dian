/* ===========================================================================
 * dmp_port.h
 *  - 把 InvenSense MotionDriver (eMPL) 嫁接到本项目所需的平台适配层
 *  - 这里声明的函数，inv_mpu.c 内部按 MOTION_DRIVER_TARGET_MSP430 约定会去调用
 * =========================================================================*/
#ifndef DMP_PORT_H
#define DMP_PORT_H

#include <stdint.h>

/* inv_mpu.c 通过这些名字读写 MPU6050：
 *   MPU6050_WriteReg(addr, reg, len, *data) -> 0 ok
 *   MPU6050_ReadData (addr, reg, len, *data) -> 0 ok
 * 实现里直接调 BSP/IMU/mpu6050.c 内部的位操作 I2C
 */
char MPU6050_WriteReg(uint8_t addr, uint8_t regaddr, uint8_t num, uint8_t *regdata);
char MPU6050_ReadData(uint8_t addr, uint8_t regaddr, uint8_t num, uint8_t *Read);

/* inv_mpu.c 内部拿 ms 用 */
void mget_ms(unsigned long *count);

#endif /* DMP_PORT_H */