#ifndef ENCODER_H
#define ENCODER_H

#include "ti_msp_dl_config.h"

/* ==================== 编码器/齿轮/轮 规格 ====================
 * 电机：MG513X 带 13 线霍尔编码器，减速比 1:28
 *   电机轴   = 13 PPR (Hall sensor pulses)
 *   输出轴   = 13 × 28 = 364 pulse/rev（=车轮）
 *   1× 解码  = 364 count/rev（仅 A 上升沿触发 1 次）
 *   4× 解码  = 364 × 4 = 1456 count/rev（双边沿）
 */
#define MOTOR_PPR            13U    /* 电机轴霍尔脉冲数 */
#define GEAR_RATIO           28U    /* 减速比 */
#define DECODE_MODE          1U     /* 1=仅 A 上升沿；4=A&B 双边沿（需改 sysconfig） */
#define WHEEL_DIAMETER_MM    65U    /* 轮径 mm */
#define PI_F                 3.14159f

/* 每输出轴（=每轮）转对应的累计计数
 *  注意：换电机只改 MOTOR_PPR / GEAR_RATIO 即可 */
#define COUNTS_PER_WHEEL_REV  (MOTOR_PPR * GEAR_RATIO * DECODE_MODE)  /* = 364（1×）*/

typedef struct {
    volatile int32_t count;       // 累计计数值（带方向）
    int32_t          last_count;  // 上次采样时的计数值
    float            rpm;         // 最近一次采样算出的转速 (rpm)
    float            speed_ms;    // 最近一次采样算出的轮速 (m/s)
} Encoder_t;

// 初始化两个编码器计数器
void encoder_init(void);

// 在编码器 A 相上升沿中断中调用，motor_id: 1 或 2
void encoder_process_isr(uint8_t motor_id);

// 获取累计计数值
int32_t encoder_get_count(uint8_t motor_id);

// 清零累计计数
void encoder_clear_count(uint8_t motor_id);

// 周期性采样速度（应在主循环中以固定周期调用）
// sample_period_ms: 两次调用之间的实际间隔
void encoder_sample_speed(uint32_t sample_period_ms);

// 获取最近一次采样的转速 (rpm)
float encoder_get_rpm(uint8_t motor_id);

// 获取最近一次采样的轮速 (m/s)
float encoder_get_speed_ms(uint8_t motor_id);

#endif // ENCODER_H
