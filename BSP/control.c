/*
 * control.c — 简洁 PD 循迹 + 航向保持
 *
 * 循迹：纯 PD（比例+微分），直接从 8 路灰度算误差 → 修正左右轮差速
 * 公式：err = 加权误差,  corr = Kp*err + Kd*(err - last_err)
 *       L = BASE - corr,  R = BASE + corr
 */

#include "control.h"

/* ========== 循迹 PD 参数 ========== */
#define TRACE_KP       10        /* 比例系数 */
#define TRACE_KD       20        /* 微分系数 */
#define TRACE_DEAD      3        /* 死区：|corr|<3 不修正 */
#define TRACE_BASE     1100      /* 基础速度 (CCR, 0~4000) */
#define TRACE_EMA       0.3f     /* EMA 平滑系数（越小越平滑） */
#define TRACE_MIN      300       /* 最低占空比 */
#define TRACE_MAX      3500      /* 最高占空比 */

/* ========== 8 路灰度 → 位置误差 ==========
 * 权重表：中间传感器权重小，两边大（离中心越远，err 越大）
 * channel:  1   2   3   4   5   6   7   8
 * weight: -40 -30 -15  -3  +3 +15 +30 +40
 * 返回 -40..+40：负=偏左, 正=偏右, 0=居中 */
static int gray_to_error(void)
{
    static const int w[8] = {40, 30, 15, 3, -3, -15, -30, -40};
    int sum_w = 0, cnt = 0;

    for (int i = 0; i < 8; i++) {
        if (car.gray[i] == 0U) {   /* 0=压在黑线上 */
            sum_w += w[i];
            cnt++;
        }
    }

    /* 全部白/全部黑 → 居中 */
    if (cnt == 0 || cnt == 8) return 0;

    return sum_w / cnt;
}

/* ========== 循迹 ========== */
void control_line_trace(void)
{
    static float err_f = 0.0f;   /* EMA 滤波后的误差 */
    static int   first  = 1;

    int err_raw = gray_to_error();

    /* EMA 平滑 */
    if (first) {
        err_f = (float)err_raw;
        first = 0;
    } else {
        err_f = err_f * (1.0f - TRACE_EMA) + (float)err_raw * TRACE_EMA;
    }

    /* P + D（基于平滑误差） */
    static float last_err = 0.0f;
    float P = TRACE_KP * err_f;
    float D = TRACE_KD * (err_f - last_err);
    last_err = err_f;

    int corr = (int)(P + D);

    /* 死区：微小修正直接忽略，避免高频抖动 */
    if (corr > -TRACE_DEAD && corr < TRACE_DEAD) corr = 0;

    /* 左右轮差速：L = BASE - corr,  R = BASE + corr */
    int L = (int)TRACE_BASE - corr;
    int R = (int)TRACE_BASE + corr;

    if (L < TRACE_MIN) L = TRACE_MIN;
    if (L > TRACE_MAX) L = TRACE_MAX;
    if (R < TRACE_MIN) R = TRACE_MIN;
    if (R > TRACE_MAX) R = TRACE_MAX;

    car.left_duty  = (uint16_t)L;
    car.right_duty = (uint16_t)R;
}

/* ========== 航向保持（简单 PD） ========== */
static struct {
    uint16_t base, min, max;
    float    kp, kd;
    float    dead_deg;
    float    clamp_deg;
} g_hh;

void control_heading_hold(void)
{
    float err = car.yaw;

    /* 死区 + 限幅 */
    if (err > -g_hh.dead_deg && err < g_hh.dead_deg) err = 0.0f;
    if (err >  g_hh.clamp_deg) err =  g_hh.clamp_deg;
    if (err < -g_hh.clamp_deg) err = -g_hh.clamp_deg;

    float P = g_hh.kp * err;
    float D = -g_hh.kd * car.gz_dps;   /* 陀螺 Z 阻尼 */

    int L = (int)g_hh.base - (int)(P + D);
    int R = (int)g_hh.base + (int)(P + D);

    if (L < (int)g_hh.min) L = (int)g_hh.min;
    if (L > (int)g_hh.max) L = (int)g_hh.max;
    if (R < (int)g_hh.min) R = (int)g_hh.min;
    if (R > (int)g_hh.max) R = (int)g_hh.max;

    car.left_duty  = (uint16_t)L;
    car.right_duty = (uint16_t)R;
}

/* ========== 初始化 ========== */
void control_init(void)
{
    g_hh.base     = 900U;
    g_hh.min      = 400U;
    g_hh.max      = 2400U;
    g_hh.kp       = 30.0f;
    g_hh.kd       = 1.5f;
    g_hh.dead_deg = 1.0f;
    g_hh.clamp_deg = 30.0f;
}
