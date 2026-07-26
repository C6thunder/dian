/* ===========================================================================
 * soft_timer.h
 *  - 软件定时器（基于 ms 时基 g_tick_ms）
 *  - 每个定时器到点会置位一个标志位（用于 flag 调度器）
 *
 * 用法：
 *   soft_timer_repeat_init(SOFT_TIMER_READ_IMU, 50);   // 每 50ms 读 IMU
 *   soft_timer_start(SOFT_TIMER_READ_IMU);
 *   ...
 *   while (1) {
 *       if (soft_timer_is_timeout(SOFT_TIMER_READ_IMU)) {
 *           soft_timer_reset(SOFT_TIMER_READ_IMU);     // 重复定时器可省
 *           read_imu();
 *       }
 *   }
 * =========================================================================*/
#ifndef SOFT_TIMER_H
#define SOFT_TIMER_H

#include "struct_typedef.h"

typedef enum {
    SOFT_TIMER_READ_IMU = 0,    // 20ms    读 MPU6050 一次
    SOFT_TIMER_SAMPLE_SPEED,    // 50ms   计算编码器 rpm/m/s
    SOFT_TIMER_HEADING_HOLD,    // 20ms   跑一次循迹/航向保持 PD
    SOFT_TIMER_OLED_REFRESH,    // 30ms   OLED 显示
    SOFT_TIMER_READ_GRAY,       // 20ms   读 8 路灰度
    SOFT_TIMER_MAX,
} soft_timer_type_t;

void soft_timer_init(void);
void soft_timer_single_init(soft_timer_type_t id, uint32_t period_ms);
void soft_timer_repeat_init(soft_timer_type_t id, uint32_t period_ms);
void soft_timer_start(soft_timer_type_t id);
void soft_timer_stop(soft_timer_type_t id);
uint8_t soft_timer_is_timeout(soft_timer_type_t id);   // 读超时标志（不重置）
void    soft_timer_reset(soft_timer_type_t id);        // 手动重置
uint32_t soft_timer_elapsed(soft_timer_type_t id);     // 已等待时间（ms）

/* 在 SysTick 或主循环里调用，ms 推进 */
void soft_timer_tick(void);

#endif /* SOFT_TIMER_H */
