/* ===========================================================================
 * board.h
 *  - 上电总初始化入口（main 只调一次）
 * =========================================================================*/
#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

// 一次性初始化所有外设，返回 0 成功 / 1 MPU6050 没找到
uint8_t board_init(void);

// 软定时器 tick 推进（建议在 SysTick 1ms 中断里调用；缺中断时由 main 自行步进）
void board_tick_inc(void);

/* v4.9：上电 IMU 检测结果（给 OLED 状态条用）
 *   g_mpu6050_present = 1 → WHO_AM_I 通过、寄存器配好
 *                     = 0 → MPU6050 没找到 / 总线无应答
 *   g_dmp_present      = 1 → DMP 固件加载 + FIFO 配置成功
 *                       = 0 → DMP 加载失败（E-MPL 库的标准返码 1..9）
 *   g_mpu6050_who      → WHO_AM_I 原始字节（mpu6050.c 定义，0x68 为正常） */
extern volatile uint8_t g_mpu6050_present;
extern volatile uint8_t g_dmp_present;
extern uint8_t g_mpu6050_who;

#endif /* BOARD_H */