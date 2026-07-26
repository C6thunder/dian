/* ===========================================================================
 * control.h
 *  - 车体控制（航向保持 PD + 灰度循迹 PD）
 *  - main.c 按 car.run_mode 选 control_heading_hold() / control_line_trace()
 *
 *  v9：循迹改"离散查表 + 冻结记忆"，无 weight[] 表，无边缘状态机。
 *      电赛经典写法 — 把视觉位置映射成 5 档 err（±1.5/±3/±6/±10/±25）
 *      + 急弯覆盖 ±40 + 丢线冻结 last_pos_err。
 * =========================================================================*/
#ifndef CONTROL_H
#define CONTROL_H

#include "struct_typedef.h"

/* 航向保持（PD）参数 */
typedef struct {
    uint16_t base_duty;        // 直行基础 PWM (CCR)
    uint16_t duty_min;
    uint16_t duty_max;
    float    kp;               // 航向比例
    float    kd;               // 航向微分
    float    dead_deg;
    float    clamp_deg;
} HeadingCfg_t;

/* 灰度循迹（PD）参数 — v9 */
typedef struct {
    uint16_t base_duty;          // 前进基础 duty（左右轮同值）
    uint16_t duty_min;
    uint16_t duty_max;
    float    kp;                 // 位置误差 P
    float    ki;                 // 位置误差 I（恒为 0）
    float    kd;                 // 位置误差 D
    float    max_out;            // PID 输出限幅（左右最大差速）
    float    max_iout;           // 积分限幅

    /* —— v2 字段保留以兼容 struct，不用 —— */
    float    ema_alpha;
    float    lost_hold_ms;
    float    lost_decay_per_sec;
    float    centered_thresh;
    uint8_t  centered_ticks;
    float    i_drift_gain;
    float    i_drift_max;
    float    lost_thresh;

    /* —— v5 陀螺融合 —— */
    float    ky;
    float    yaw_decay_per_sec;
    float    yaw_drift_max;
    int16_t  left_bias;

    /* —— v6 出弯阻尼 + 弯道降速 —— */
    float    speed_err_thresh;
    float    speed_yaw_thresh;
    float    speed_curve_scale;
    float    exit_ky;
    float    exit_err_thresh;
    float    exit_yaw_thresh;
} TraceCfg_t;

/* 初始化（航向 PD + 循迹 PID 都设默认） */
void control_init(void);

/* 航向保持：从 car.yaw/gz_dps 算出左右轮 duty，写 car.left_duty/right_duty */
void control_heading_hold(void);

/* 灰度循迹：从 car.gray[8] 离散查表得 err + PD + 陀螺航向补偿 */
void control_line_trace(void);

/* 当前计算好的位置误差（给 OLED 显示用，0 表示黑线居中） */
float control_get_line_pos_err(void);

/* Get_Error() 原始离散返回值（未经 EMA 滤波），OLED 诊断用 */
float control_get_line_error_raw(void);

/* 陀螺仪阻尼项，OLED 诊断用 */
float control_get_gyro_damp(void);

/* v5：当前累积的陀螺航向漂移（°），给 OLED 监视陀螺是否真的在工作 */
float control_get_yaw_drift(void);

/* 丢线记忆占位（v4.8 已删，函数保留以链接） */
float control_get_line_valid_err(void);

/* 当前连续丢线时长（毫秒）占位 — 保留以链接 */
uint32_t control_get_lost_ms(void);

/* v17：丢线状态 + 最后已知黑线位置（OLED 调试） */
int control_is_lost(void);
int control_get_last_valid_error(void);

/* 直接读（main 给电机） */
uint16_t control_get_left_duty(void);
uint16_t control_get_right_duty(void);

#endif /* CONTROL_H */
