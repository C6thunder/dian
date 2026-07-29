/*
 * control.c — 简洁 PD 循迹 + 航向保持
 *
 * 循迹：纯 PD（比例+微分），直接从 8 路灰度算误差 → 修正左右轮差速
 * 公式：err_f = EMA(err_raw),  corr = Kp*err_f + Kd*(err_f - last_err_f)
 *       L = BASE - corr,  R = BASE + corr
 *
 * 启停逻辑：
 *   上电立即出发 → 行驶 ~4m 检测 2帧内≥4路不同黑 → 急停，最远 4.5m 强制停
 */

#include "control.h"
#include "encoder.h"
#include "motor.h"

/* ========== 循迹 PD 参数 ========== */
#define TRACE_KP       10        /* 比例系数 */
#define TRACE_KD       20        /* 微分系数 */
#define TRACE_DEAD      3        /* 死区：|corr|<3 不修正 */
#define TRACE_BASE     1100      /* 基础速度 (CCR, 0~4000) */
#define TRACE_EMA       0.4f     /* EMA 平滑系数 */
#define TRACE_MIN      300       /* 最低占空比 */
#define TRACE_MAX      3500      /* 最高占空比 */

/* 停止检测窗口：4.0m ~ 4.5m
 *   4.0m 后开始检测误差突变 → 4.5m 强制停（兜底）*/
#define TARGET_DIST_MM       4000
#define MAX_DIST_MM          4500
#define STOP_UNIQ_BLACK      3       /* 2帧内亮过 ≥4 个不同灯 = 停止线 */
#define TARGET_COUNTS        ((int32_t)((float)(TARGET_DIST_MM) * COUNTS_PER_WHEEL_REV / (PI_F * WHEEL_DIAMETER_MM)))
#define MAX_COUNTS           ((int32_t)((float)(MAX_DIST_MM) * COUNTS_PER_WHEEL_REV / (PI_F * WHEEL_DIAMETER_MM)))

/* ========== 8 路灰度 → 位置误差 ========== */
static int gray_to_error(void)
{
    static const int w[8] = {40, 30, 15, 3, -3, -15, -30, -40};
    int sum_w = 0, cnt = 0;

    for (int i = 0; i < 8; i++) {
        if (car.gray[i] == 0U) {
            sum_w += w[i];
            cnt++;
        }
    }

    if (cnt == 0 || cnt == 8) return 0;
    return sum_w / cnt;
}

/* 黑线传感器数量 */
static int black_count(void)
{
    int n = 0;
    for (int i = 0; i < 8; i++) {
        if (car.gray[i] == 0U) n++;
    }
    return n;
}

/* ========== 循迹（距离 + 误差突变停） ========== */
typedef enum { RUN, DONE } trace_state_t;

void control_line_trace(void)
{
    static trace_state_t state     = RUN;
    static float         err_f     = 0.0f;
    static int           first_ema = 1;
    static float         last_err  = 0.0f;
    static int32_t       start_L   = 0;
    static int32_t       start_R   = 0;
    static int           inited    = 0;
    static uint8_t       prev_mask = 0;   /* 上一帧黑灯位图 */

    if (!inited) {
        start_L = encoder_get_count(LEFT_MOTOR_ID);
        start_R = encoder_get_count(RIGHT_MOTOR_ID);
        inited  = 1;
    }

    int err_raw = gray_to_error();

    /* 当前帧黑灯位图 */
    uint8_t cur_mask = 0;
    for (int i = 0; i < 8; i++) {
        if (car.gray[i] == 0U) cur_mask |= (uint8_t)(1U << i);
    }

    /* 距离 + 2帧内不同黑灯数检测 */
    if (state == RUN) {
        int32_t dL = encoder_get_count(LEFT_MOTOR_ID)  - start_L;
        int32_t dR = encoder_get_count(RIGHT_MOTOR_ID) - start_R;
        if (dL < 0) dL = -dL;
        if (dR < 0) dR = -dR;

        int32_t avg = (dL + dR) / 2;

        /* 4.5m 强制兜底停 */
        if (avg >= MAX_COUNTS) {
            state = DONE;
            car.left_duty  = 0;
            car.right_duty = 0;
            return;
        }

        /* 4.0m ~ 4.5m：2帧内 ≥4 个不同传感器压黑 → 停止 */
        if (avg >= TARGET_COUNTS) {
            uint8_t uni = cur_mask | prev_mask;   /* 2帧取并集 */
            int cnt = 0;
            for (int i = 0; i < 8; i++) {
                if (uni & (1U << i)) cnt++;
            }
            if (cnt >= STOP_UNIQ_BLACK) {
                state = DONE;
                car.left_duty  = 0;
                car.right_duty = 0;
                return;
            }
        }
        prev_mask = cur_mask;
    } else {
        car.left_duty  = 0;
        car.right_duty = 0;
        return;
    }

    /* ---- PD 控制（仅 RUN 状态执行到这里） ---- */

    /* EMA 平滑 */
    if (first_ema) {
        err_f     = (float)err_raw;
        first_ema = 0;
    } else {
        err_f = err_f * (1.0f - TRACE_EMA) + (float)err_raw * TRACE_EMA;
    }

    /* P + D */
    float P = TRACE_KP * err_f;
    float D = TRACE_KD * (err_f - last_err);
    last_err = err_f;

    int corr = (int)(P + D);

    /* 死区 */
    if (corr > -TRACE_DEAD && corr < TRACE_DEAD) corr = 0;

    /* L = BASE - corr,  R = BASE + corr */
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

    if (err > -g_hh.dead_deg && err < g_hh.dead_deg) err = 0.0f;
    if (err >  g_hh.clamp_deg) err =  g_hh.clamp_deg;
    if (err < -g_hh.clamp_deg) err = -g_hh.clamp_deg;

    float P = g_hh.kp * err;
    float D = -g_hh.kd * car.gz_dps;

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
