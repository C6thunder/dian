/* ===========================================================================
 * control.c — 8 路灰度循迹 + 航向保持
 *
 * 单环 PD + 陀螺阻尼：error → PD → 减 gyro 阻尼 → 电机差速
 * =========================================================================*/
#include "control.h"
#include "struct_typedef.h"
#include "motor.h"

#ifndef GRAY_INVERT
#define GRAY_INVERT  0
#endif

#define DIRECTION_FLIP  1

#define TRACE_KP    38
#define TRACE_KI    0.0f
#define TRACE_KD    60
#define TRACE_EMA   0.2f  /* ~5帧渐进 */
#define TRACE_KY    0.5f
#define TRACE_GYRO_DEAD  5
#define TRACE_D_CLAMP  800
#define TRACE_I_CLAMP  400

static const int G_BASE_SPEED = 700;
#define TRACE_DUTY_MIN  150
#define TRACE_MAX_DELTA  400
#define MOTOR_TRIM_L    0    /* 左轮偏置：左快则负，左慢则正 */
#define MOTOR_TRIM_R    0    /* 右轮偏置 */

static float    g_error_ema  = 0.0f; /* EMA 平滑值 */
static float    g_error_i    = 0.0f; /* I 累积 */
static int      g_last_error = 0;
static int      g_first_frame = 1;
static int      g_last_pid_out = 0;   /* 输出增量平滑 */
static int      g_last_valid  = 0;  /* 丢线记忆方向 */
static int      g_error_now  = 0;
static int      g_gyro_damp  = 0;
static uint16_t g_left       = 0;
static uint16_t g_right      = 0;

/* ===== Get_Error() 连续加权位置 ===== */
static int Get_Error(void)
{
    /* 传感器位置：左边正，右边负。err>0→线在左→左转 */
    static const int pos[8] = {40, 30, 15, 5, -5, -15, -30, -40};

    int sum_pos = 0, sum_cnt = 0;
    for (int i = 0; i < 8; ++i) {
        int black = (GRAY_INVERT ? (car.gray[i] != 0U)
                                 : (car.gray[i] == 0U));
        if (black) {
            sum_pos += pos[i];
            sum_cnt++;
        }
    }

    if (sum_cnt == 0) return 0;   /* 全白 */
    if (sum_cnt == 8) return 0;   /* 全黑 */

    return sum_pos / sum_cnt;     /* 加权平均，连续值 */
}

/* ===== 单环 PD + 陀螺阻尼 ===== */
void control_line_trace(void)
{
    int error_raw = Get_Error();

    /* 丢线记忆：全白时用最后方向 */
    if (error_raw == 0 && g_last_valid != 0) {
        error_raw = g_last_valid;
    } else if (error_raw != 0) {
        g_last_valid = error_raw;
    }

    /* EMA 平滑：离散跳变 → 多帧渐进过渡 */
    g_error_ema = g_error_ema * (1.0f - TRACE_EMA)
                + (float)error_raw * TRACE_EMA;
    int error_smooth = (int)(g_error_ema + 0.5f);

    /* 死区：加宽滤微抖 */
    if (error_smooth >= -4 && error_smooth <= 4) error_smooth = 0;
    g_error_now = error_smooth;

    int P = TRACE_KP * error_smooth;

    /* D 也用平滑值差分，和 P 同步渐进 */
    int D_raw = g_first_frame ? 0 : TRACE_KD * (error_smooth - g_last_error);
    g_first_frame = 0;
    if (D_raw >  TRACE_D_CLAMP) D_raw =  TRACE_D_CLAMP;
    if (D_raw < -TRACE_D_CLAMP) D_raw = -TRACE_D_CLAMP;
    int D = D_raw;

    g_last_error = error_smooth;

    /* I：累积误差，死区内清零防饱卷 */
    if (error_smooth == 0) {
        g_error_i = 0.0f;
    } else {
        g_error_i += TRACE_KI * error_smooth * 0.01f;  /* dt=10ms=0.01s */
        if (g_error_i >  TRACE_I_CLAMP) g_error_i =  TRACE_I_CLAMP;
        if (g_error_i < -TRACE_I_CLAMP) g_error_i = -TRACE_I_CLAMP;
    }
    int I = (int)g_error_i;

    /* 陀螺阻尼：死区滤噪声 */
    float gz = car.gz_dps;
    if (gz > -TRACE_GYRO_DEAD && gz < TRACE_GYRO_DEAD) gz = 0.0f;
    float gyro_damp = TRACE_KY * gz;
    g_gyro_damp = (int)gyro_damp;

    int pid_out_raw = (int)((P + I + D - gyro_damp) / 2.0f);

    /* 输出增量平滑：限制每帧变化 ≤ MAX_DELTA，防猛加猛减 */
    int delta = pid_out_raw - g_last_pid_out;
    if (delta >  TRACE_MAX_DELTA) delta =  TRACE_MAX_DELTA;
    if (delta < -TRACE_MAX_DELTA) delta = -TRACE_MAX_DELTA;
    int pid_out = g_last_pid_out + delta;
    g_last_pid_out = pid_out;

#if DIRECTION_FLIP
    int sL = G_BASE_SPEED - pid_out;
    int sR = G_BASE_SPEED + pid_out;
#else
    int sL = G_BASE_SPEED + pid_out;
    int sR = G_BASE_SPEED - pid_out;
#endif

    if (sL < TRACE_DUTY_MIN) sL = TRACE_DUTY_MIN;
    if (sR < TRACE_DUTY_MIN) sR = TRACE_DUTY_MIN;

    motor_set_direction(LEFT_MOTOR_ID,  1U);
    motor_set_duty(LEFT_MOTOR_ID,  (uint16_t)sL);
    motor_set_direction(RIGHT_MOTOR_ID, 1U);
    motor_set_duty(RIGHT_MOTOR_ID, (uint16_t)sR);

    g_left  = (uint16_t)sL;
    g_right = (uint16_t)sR;
    car.left_duty  = g_left;
    car.right_duty = g_right;
}

/* ===== 航向保持 ===== */
static HeadingCfg_t g_hcfg;

void control_heading_hold(void)
{
    float err = car.yaw;
    if (err >  g_hcfg.clamp_deg) err =  g_hcfg.clamp_deg;
    if (err < -g_hcfg.clamp_deg) err = -g_hcfg.clamp_deg;

    float p = 0.0f, d = 0.0f;
    if (err > g_hcfg.dead_deg || err < -g_hcfg.dead_deg) {
        p =  g_hcfg.kp * err;
        d = -g_hcfg.kd * car.gz_dps;
    }
    float corr = p + d;

    int base = (int)g_hcfg.base_duty;
    int l = base - (int)corr;
    int r = base + (int)corr;

    if (l < (int)g_hcfg.duty_min) l = (int)g_hcfg.duty_min;
    if (l > (int)g_hcfg.duty_max) l = (int)g_hcfg.duty_max;
    if (r < (int)g_hcfg.duty_min) r = (int)g_hcfg.duty_min;
    if (r > (int)g_hcfg.duty_max) r = (int)g_hcfg.duty_max;

    g_left  = (uint16_t)l;
    g_right = (uint16_t)r;
    car.left_duty  = g_left;
    car.right_duty = g_right;

    motor_set_direction(LEFT_MOTOR_ID,  1U);
    motor_set_duty(LEFT_MOTOR_ID,  g_left);
    motor_set_direction(RIGHT_MOTOR_ID, 1U);
    motor_set_duty(RIGHT_MOTOR_ID, g_right);
}

/* ===== 初始化 + getter ===== */
void control_init(void)
{
    g_hcfg.base_duty = 900U;
    g_hcfg.duty_min  = 400U;
    g_hcfg.duty_max  = 2400U;
    g_hcfg.kp        = 30.0f;
    g_hcfg.kd        = 1.5f;
    g_hcfg.dead_deg  = 1.0f;
    g_hcfg.clamp_deg = 30.0f;

    g_error_ema   = 0.0f;
    g_error_i     = 0.0f;
    g_last_error  = 0;
    g_first_frame  = 1;
    g_last_pid_out = 0;
    g_last_valid   = 0;
    g_error_now    = 0;
    g_gyro_damp  = 0;
    g_left = g_right = 0;
}

float control_get_line_pos_err(void)       { return (float)g_error_now; }
float control_get_line_error_raw(void)      { return (float)g_error_now; }
float control_get_gyro_damp(void)          { return (float)g_gyro_damp; }
float control_get_line_valid_err(void)     { return (float)g_error_now; }
float control_get_yaw_drift(void)          { return 0.0f; }
uint32_t control_get_lost_ms(void)         { return 0U; }
int    control_is_lost(void)               { return 0; }
int    control_get_last_valid_error(void)   { return 0; }
uint16_t control_get_left_duty(void)       { return g_left;  }
uint16_t control_get_right_duty(void)      { return g_right; }
