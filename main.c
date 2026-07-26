/*
 * main.c — 顶层调度器
 *  - 流程：board_init() → while(1) { 推进 tick → 处理按键事件 → 消费各 flag → 显示 }
 *  - 所有传感器、控制数据都在 Car_t car 中流转（struct_typedef.h）
 *  - 所有时序由 soft_timer 调度
 */

#include "ti_msp_dl_config.h"
#include "BSP/Delay.h"
#include "ndrivers/oled.h"
#include <stdio.h>
#include "BSP/struct_typedef.h"
#include "BSP/soft_timer.h"
#include "BSP/board.h"
#include "BSP/control.h"
#include "BSP/encoder.h"
#include "BSP/motor.h"        /* LEFT_MOTOR_ID / RIGHT_MOTOR_ID */
#include "BSP/gw_grayscale_sensor.h"
#include "BSP/IMU/IMU.h"
#include "BSP/IMU/mpu6050.h"
/* v4.22：删 ndrivers/uart.h。模块是纯 I2C MPU6050（不是串口版），
 * 走 DL_I2C0 (PA28/PA31) 直接轮询寄存器，不再用任何 UART 路径。*/

static float g_pitch = 0, g_roll = 0, g_yaw = 0;
static float g_gx = 0, g_gy = 0, g_gz = 0;
/* v4.15：原始 int16 陀螺仪值，OLED 调试用 — 看 I2C 直读是否真的拿到寄存器 */
static int16_t g_gx_raw = 0, g_gy_raw = 0, g_gz_raw = 0;

/* v7：zG 噪声滤波
 *   实测 MPU6050 静止时 Gz 噪声 ~10°/s（典型 ±2000°/s 量程下 LSB=16.4），
 *   不滤波直接进 yaw_drift 累积会让直道"自己歪"→ 控制环在噪声上反复加 D。
 *   策略：先用死区（dead zone）把小于噪声底的值清零，
 *         再做 EMA（一阶低通）平滑。car.gz_dps 给控制环的全是滤波后值。
 *   调参起点：
 *     GZ_DEAD_ZONE_DPS = 0.5  ← |Gz|<0.5°/s 直接当 0（噪声底剔除）
 *     GZ_EMA_ALPHA      = 0.25 ← 越小滤波越狠；0.25 → ~3 帧平均（≈60ms）
 */
static float g_gz_filtered = 0.0f;
#define GZ_DEAD_ZONE_DPS   0.5f
#define GZ_EMA_ALPHA       0.25f

/* IMU 诊断：上次 IMU_getData / IMU_getGyro 返回码（0=成功, 1/2=失败）
 *    mpu_dmp_get_data: 0=OK  1=FIFO 读失败  2=无四元数
 *    mpu_get_gyro_reg: 0=OK  !0=I2C/寄存器读失败
 * OLED 显示丢码率 → 立刻看出 MPU6050/DMP 是否真的在工作。 */
static uint8_t g_imu_dmp_rc    = 0xFF;   /* 0xFF=未初始化 */
static uint8_t g_imu_gyro_rc   = 0xFF;
static uint32_t g_imu_read_cnt = 0;       /* 调用次数 */
static uint32_t g_imu_fail_cnt = 0;       /* 失败次数（任一函数不为 0）*/

/* ============================================================
 * 临时电机对比测试开关（2026-07-22）
 *   MOTOR_TEST_PHASE = 0  → 关闭，按 car.run_mode 正常运行
 *   MOTOR_TEST_PHASE = 1  → 自动循环，每 ~5s 切换：
 *        第一阶段只驱动 motor_id=1（测 R 轮本体能力）
 *        第二阶段只驱动 motor_id=2（测 L 轮本体能力）
 *        第三阶段两个都驱动（验证是否能走直，差距多少）
 *   MOTOR_TEST_DUTY：单/双驱时用的 PWM（建议 500~800）
 *
 *   用法：把 MOTOR_TEST_PHASE 改 1，重新烧录一次 = 自动跑完整对比。
 *   测试完毕改回 0 + 重新烧录，恢复 trace。
 * ============================================================ */
#define MOTOR_TEST_PHASE 0   /* 0=关闭  1=自动 3 段循环 */
#define MOTOR_TEST_DUTY  600U /* 测试 duty（建议 500~800，>2000 可能烧板） */
#if MOTOR_TEST_PHASE != 0
static const  uint16_t g_motor_test_duty = MOTOR_TEST_DUTY;
static uint32_t g_test_phase_start_ms    = 0;
static uint8_t  g_test_active_id         = 1; /* 1=only M1, 2=only M2, 3=both */
#endif

/* ---------- 显示辅助 ---------- */
static void oled_show_speed(uint8_t motor_id, uint8_t y)
{
    char line[32];
    float ms  = encoder_get_speed_ms(motor_id);
    int   ms_abs  = (int)((ms < 0 ? -ms : ms) * 100.0f + 0.5f);
    int   ms_int  = ms_abs / 100;
    int   ms_frac = ms_abs % 100;
    char  sign    = (ms < 0) ? '-' : ' ';

    /* 紧凑到 ~15 字符："M1: +0.45m/s"  15 chars × 6 = 90px < 128 ✓ */
    snprintf(line, sizeof(line), "M%d: %c%d.%02dm/s",
             motor_id, sign, ms_int, ms_frac);
    OLED_ShowString(0, y, (u8 *)line, 12);
}

/* 右上角模式徽章：放在最末行的最右角（短标签） */
static void oled_show_mode_badge(void)
{
    const char *label = "?";
    uint8_t x = 104U;
#if MOTOR_TEST_PHASE != 0
    /* 临时电机测试模式：在右上角显示当前阶段 */
    switch (g_test_active_id) {
        case 1: label = "T:M1";   x = 100U; break;   /* 4 chars */
        case 2: label = "T:M2";   x = 100U; break;
        case 3: label = "T:1+2";  x =  92U; break;
        default: label = "T:?";   x = 100U; break;
    }
#else
    switch (car.run_mode) {
        case 0:  label = "STOP";  break;   /* 4 chars */
        case 1:  label = "HOLD";  break;   /* 4 chars */
        case 2:  label = "TRACE"; break;   /* 5 chars */
        default: label = "?";     break;
    }
    x = (car.run_mode == 2) ? 98U : 104U;
#endif
    OLED_ShowString(x, 48, (u8 *)label, 12);
}

static void oled_show_count(uint8_t y)
{
    char line[32];
    int32_t c1 = encoder_get_count(LEFT_MOTOR_ID);
    int32_t c2 = encoder_get_count(RIGHT_MOTOR_ID);
    /* 紧凑到 ~24 字符："L:+12345 R:+1234  IMU:0/0"  → 24 chars × 6 = 144，溢出！
     * 改成 2 行 / 拆字段：先只显示编码器，IMU 单独一行。 */
    snprintf(line, sizeof(line), "L:%+5ld R:%+5ld",
             (long)c1, (long)c2);
    OLED_ShowString(0, y, (u8 *)line, 12);
}

/* 在 OLED 第 0 模式（速度屏）的 y=48 行尾追加 IMU 状态：
 *     "IMU D:0 G:0  fail:123/456"
 *   - D / G 是 IMU_getData / IMU_getGyro 的最新返回码（0=成功）
 *   - fail / total 统计丢码率，方便判断 FIFO 是否真的在进数据
 */
static void oled_imu_health(uint8_t y)
{
    /* 最坏约 42 字符："IMU D:255 G:255 fail:4294967295/4294967295"
     * 原 line[32] 被 snprintf 截断 → OLED 上看不到完整 fail 计数，
     * 是早期 IMU 全 0 故障无法诊断的原因之一。48 留余量。 */
    char line[48];
    snprintf(line, sizeof(line), "IMU D:%u G:%u fail:%lu/%lu",
             g_imu_dmp_rc, g_imu_gyro_rc,
             (unsigned long)g_imu_fail_cnt,
             (unsigned long)g_imu_read_cnt);
    OLED_ShowString(0, y, (u8 *)line, 12);
}

static void oled_show_imu(uint8_t y)
{
    char line[32];
    /* 紧凑到 ~14 字符："Yaw:  +90 P:+0"  → 把更重要的 yaw 放前，pitch 留作右侧 */
    snprintf(line, sizeof(line), "Y:%+5.0f P:%+4.0f",
             g_yaw, g_pitch);
    OLED_ShowString(0, y, (u8 *)line, 12);
}

/* v4.22 调试期间：每个屏的第一行强制显示 IMU 状态条。
 *   格式（紧凑 ≤21 字符，能塞进 128 px）：
 *     "U:字 W:HH D?G? cnt/fail"
 *   U:  K=OK  X=NG ← g_mpu6050_present
 *   W:  HH      ← g_mpu6050_who 原始字节
 *                   0x68 真 MPU6050 / 0x70 真 MPU6500（v4.12）
 *                   0xFF SDA 浮空（MPU 没接 / VCC 没上）
 *                   0x00 SDA 短路到地
 *                   其他 → 地址错（AD0 拉错）
 *   D:  0/1/2   ← IMU_getData 返回码（0 OK / 1 FIFO 失败 / 2 无四元数）
 *   G:  0/1     ← IMU_getGyro 返回码（0 OK / 1 I2C/寄存器读失败）
 *   cnt/fail    ← g_imu_read_cnt / g_imu_fail_cnt，mod 10000 防溢出
 *
 * 例：
 *   上电 OK 直读 → "U:K W:68 D0G0 1234/0"
 *   SDA 浮空    → "U:X W:FF D1G1 0/1234"
 *   AD0 拉错    → "U:X W:90 D1G1 5/200" */
static void oled_show_status_bar(void)
{
    char line[32];
    snprintf(line, sizeof(line), "U:%c W:%02X D%uG%u %u/%u",
             g_mpu6050_present ? 'K' :
             (g_mpu6050_who == 0xEEU) ? '?' : 'X',  /* v4.23：SKIP_IMU_INIT=1 时显示 ? */
             (unsigned)g_mpu6050_who,
             (unsigned)g_imu_dmp_rc,
             (unsigned)g_imu_gyro_rc,
             (unsigned)(g_imu_read_cnt % 10000U),
             (unsigned)(g_imu_fail_cnt % 10000U));
    OLED_ShowString(0, 0, (u8 *)line, 12);
}

static void oled_show_imu_big(void)
{
    /* 改成 12px 字体：4 行 × 12px = 占 y=0..47
     * 把 y=48..59 留给 oled_imu_health（12px），y=60..63 是空隙；
     * 原 16px 在 y=48 那行就会和 y=56 的 health 重叠 / 超出 64px 屏。 */
    char line[32];
    snprintf(line, sizeof(line), "P :%+6.1f",   g_pitch);
    OLED_ShowString(0, 0,  (u8 *)line, 12);
    snprintf(line, sizeof(line), "R :%+6.1f",   g_roll);
    OLED_ShowString(0, 12, (u8 *)line, 12);
    snprintf(line, sizeof(line), "Y :%+5.0f",   g_yaw);
    OLED_ShowString(0, 24, (u8 *)line, 12);
    snprintf(line, sizeof(line), "Gz:%+6.1f/s", g_gz);
    OLED_ShowString(0, 36, (u8 *)line, 12);
}

/* 第 3 模式（调试）：v4.6 简洁版 — 只显示最重要的 4 个指标
 *   行 0：M1:    850   ← 左轮 duty（PD 实际输出）
 *   行 1：M2:    850   ← 右轮 duty
 *   行 2：Gz:+0.0°/s  ← 陀螺仪 Z 轴角速度（判断车是否在拐）
 *   行 3：M2 e:+0.00  ← run_mode + 位置 err（cnt==0 时显示记忆方向）
 *
 * 用户反馈：OLED 屏幕小，要去掉 8 路原始 bar 屏。
 *
 * 看拐弯强度的判读：
 *   - 直行：M1 ≈ M2 ≈ 850，Gz ≈ 0
 *   - 进直角弯瞬间：M1/M2 里应该有一个被钳到 ~80（满刹），
 *                   另一个冲到 ≥1500，Gz 不为 0 且在拐弯方向
 *   - 如果 M1-M2 的差 < 500 duty → max_out 没拉到位
 *   - 如果 Gz 还是 0 但 M1/M2 差很大 → 电机/接线问题
 */
/* v11 精简调试屏：4 行 16px 间距，只显示循迹调试核心字段 */
static void oled_show_trace_dbg(void)
{
    char line[32];

    /* 行 0（y=0）：传感器可视化 bar + 原始值
     *   "##..##.. GR:18" — bar 从左到右=CH1..CH8，#=黑 .=白 */
    {
        char bar[9];
        for (uint8_t i = 0; i < 8; ++i) {
            bar[i] = (car.gray[i] == 0U) ? '#' : '.';
        }
        bar[8] = '\0';
        snprintf(line, sizeof(line), "%s GR:%02X", bar, (unsigned)car.gray_raw);
        OLED_ShowString(0, 0, (u8 *)line, 12);
    }

    /* 行 1（y=16）：error + gyro_damp */
    {
        float er = control_get_line_pos_err();
        float gd = control_get_gyro_damp();
        snprintf(line, sizeof(line), "e:%+4.0f d:%+4.0f", er, gd);
        OLED_ShowString(0, 16, (u8 *)line, 12);
    }

    /* 行 2（y=32）：L/R duty */
    snprintf(line, sizeof(line), "L:%-5u R:%-5u",
             (unsigned)car.left_duty, (unsigned)car.right_duty);
    OLED_ShowString(0, 32, (u8 *)line, 12);

    /* 行 3（y=48）：陀螺仪 */
    snprintf(line, sizeof(line), "Gz:%+5.1f/s", car.gz_dps);
    OLED_ShowString(0, 48, (u8 *)line, 12);
}

/* ---------- 主循环 ---------- */
int main(void)
{
    board_init();

    /* v4.9 删掉旧的"起始显示 4 行"——它的 y=0/16/32/48 坐标系和新的
     * 状态条+4 行内容（y=0/12/24/36/48）错位，第一个 50ms 就叠乱码。
     * 进入 while 之前先清屏，第一次 refresh 由主循环接管。
     * 同时把整个 main 改成每次 Refresh 前 OLED_Clear，
     * 保证未来加新屏不会再因为 y 坐标系错位叠到一起。 */
    OLED_Clear();
    OLED_Refresh();

    while (1) {
        /* SysTick 1ms 中断自动推进 g_tick_ms + soft_timer_tick()，
         * 主循环只剩"检查 flag → 干活"，不再 delay */
        __WFI();   // 等中断，省电；flag 触发由 SysTick 处理

        /* 注：当前硬件无按键，按键相关代码已移除。
         * 如以后加按键，恢复 main.c 里的 key_get_events()/key_clear_event() 即可。*/

        /* ----- 读 IMU（每 ~10ms，软定时器触发）-----
         *               但仍保留调用让 g_imu_dmp_rc 给 OLED 状态条看真实状态。
         *   IMU_getGyro  走 mpu6050_read_gyro_raw（v4.13 直读寄存器，
         *               绕开 DMP sensor mask） — 这是控制环实际用到的数据。
         * 控制环用 car.gz_dps，方向由模块安装决定，先取同号再现场标定符号。
         *
         * v7：Gz 噪声处理。静止实测 ~10°/s 噪声，会污染 yaw_drift 累积
         *     → 直道"自己歪"。先死区清零底噪，再 EMA 低通。 */
        if (soft_timer_is_timeout(SOFT_TIMER_READ_IMU)) {
            soft_timer_reset(SOFT_TIMER_READ_IMU);
            g_imu_read_cnt++;

            g_imu_dmp_rc  = IMU_getData(&g_pitch, &g_roll, &g_yaw);
            g_imu_gyro_rc = IMU_getGyro(&g_gx, &g_gy, &g_gz,
                                        &g_gx_raw, &g_gy_raw, &g_gz_raw);

            /* 死区：噪声底以下当 0；EMA：低通平滑 */
            float raw_dead = (g_gz > GZ_DEAD_ZONE_DPS) ? g_gz :
                             (g_gz < -GZ_DEAD_ZONE_DPS) ? g_gz : 0.0f;
            g_gz_filtered = g_gz_filtered * (1.0f - GZ_EMA_ALPHA)
                          + raw_dead     * GZ_EMA_ALPHA;

            /* 陀螺仪 Z 轴 = 车是否在拐：
             *   现场若发现拐弯方向反了，把这一行加负号。
             * 控制环拿到的是滤波后值；raw 在 g_gz 里保留给 OLED 显示。 */
            car.gz_dps = g_gz_filtered;

            if (g_imu_dmp_rc != 0U || g_imu_gyro_rc != 0U) {
                g_imu_fail_cnt++;
            }
        }

        /* ----- 算转速（每 ~50ms） ----- */
        if (soft_timer_is_timeout(SOFT_TIMER_SAMPLE_SPEED)) {
            soft_timer_reset(SOFT_TIMER_SAMPLE_SPEED);
            encoder_sample_speed(50);
            car.left_rpm   = encoder_get_rpm(LEFT_MOTOR_ID);
            car.right_rpm  = encoder_get_rpm(RIGHT_MOTOR_ID);
            car.left_ms    = encoder_get_speed_ms(LEFT_MOTOR_ID);
            car.right_ms   = encoder_get_speed_ms(RIGHT_MOTOR_ID);
            car.left_count  = encoder_get_count(LEFT_MOTOR_ID);
            car.right_count = encoder_get_count(RIGHT_MOTOR_ID);
        }

        /* ----- 控制环（每 ~10ms）按 car.run_mode 分派 ----- */
        if (soft_timer_is_timeout(SOFT_TIMER_HEADING_HOLD)) {
            soft_timer_reset(SOFT_TIMER_HEADING_HOLD);
#if MOTOR_TEST_PHASE != 0
            /* 临时电机独立对比测试：5 秒一换阶段 */
            if (g_test_phase_start_ms == 0U) g_test_phase_start_ms = g_tick_ms;
            uint32_t t = g_tick_ms - g_test_phase_start_ms;
            uint8_t  phase = (t / 5000U) % 3U;     /* 0/1/2 三段循环 */

            /* phase: 0 → M1 only,  1 → M2 only,  2 → both */
            uint16_t d1 = (phase == 0 || phase == 2) ? g_motor_test_duty : 0;
            uint16_t d2 = (phase == 1 || phase == 2) ? g_motor_test_duty : 0;
            motor_set_duty(1, d1);
            motor_set_duty(2, d2);
            car.left_duty  = d2;
            car.right_duty = d1;
            g_test_active_id = phase + 1;          /* 1/2/3 给 OLED 显示 */
#else
            switch (car.run_mode) {
                case 2: control_line_trace();  break;
                case 1: control_heading_hold(); break;
                default:
                    car.left_duty  = 0;
                    car.right_duty = 0;
                    break;
            }
            /* 数据源统一从 car.*_duty 出：control_line_trace / control_heading_hold
             * 都已经写 car.*_duty；default 分支 0 写到 car.*_duty 才能真正停电机
             * （之前 motor_set_duty 读的是 control.c 里的 static g_*，default 分支
             *  只清了 car.*，g_* 未清 → 电机保持上一次 PWM） */
            motor_set_duty(LEFT_MOTOR_ID,  car.left_duty);
            motor_set_duty(RIGHT_MOTOR_ID, car.right_duty);
#endif
        }

        /* ----- OLED 刷新（每 ~30ms） ----- */
        if (soft_timer_is_timeout(SOFT_TIMER_OLED_REFRESH)) {
            soft_timer_reset(SOFT_TIMER_OLED_REFRESH);

            /* v4.9：每次 refresh 前先 Clear，避免不同 mode y 坐标系错位叠一起。
             * 5 行 12px 网格（y=0/12/24/36/48），最后留 y=60..63 4px 空 buffer。 */
            OLED_Clear();

            char line[32];
            if (car.disp_mode == 0) {
                oled_show_status_bar();
                oled_show_speed(1, 12);
                oled_show_speed(2, 24);
                oled_show_count(36);
                oled_show_mode_badge();
            } else if (car.disp_mode == 1) {
                oled_show_status_bar();
                snprintf(line, sizeof(line), "P :%+6.1f", g_pitch);
                OLED_ShowString(0, 12, (u8 *)line, 12);
                snprintf(line, sizeof(line), "R :%+6.1f", g_roll);
                OLED_ShowString(0, 24, (u8 *)line, 12);
                snprintf(line, sizeof(line), "Y :%+5.0f", g_yaw);
                OLED_ShowString(0, 36, (u8 *)line, 12);
                snprintf(line, sizeof(line), "Gz:%+6.1f/s", g_gz);
                OLED_ShowString(0, 48, (u8 *)line, 12);
            } else {
                /* mode 2 = 精简调试屏：去状态条，4 行大字 */
                oled_show_trace_dbg();
            }
            OLED_Refresh();
        }

        /* ----- 读灰度（每 ~10ms） ----- */
        if (soft_timer_is_timeout(SOFT_TIMER_READ_GRAY)) {
            soft_timer_reset(SOFT_TIMER_READ_GRAY);
            car.gray_raw = gw_gray_serial_read();
            for (uint8_t i = 0; i < 8; ++i) {
                car.gray[i] = (car.gray_raw >> i) & 0x01U;
            }
        }
    }
}