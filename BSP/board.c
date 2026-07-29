/* ===========================================================================
 * board.c
 *  - 上电初始化总入口
 *  - SysTick 1ms 中断推进 g_tick_ms + soft_timer_tick()
 * =========================================================================*/
#include "BSP/board.h"
#include "ti_msp_dl_config.h"
#include <core_cm0plus.h>      // SysTick_Config / SysTick_IRQn
#include "BSP/utils/Delay.h"
#include "ndrivers/oled.h"
#include "BSP/motor/motor.h"
#include "BSP/motor/encoder.h"
#include "BSP/sensors/IMU/mpu6050.h"
#include "BSP/sensors/key.h"
#include "BSP/utils/soft_timer.h"
#include "BSP/control/control.h"
#include "BSP/sensors/gw_grayscale_sensor.h"
#include "BSP/sensors/camera.h"
#include "BSP/sensors/IMU/IMU.h"
/* v4.23：模块是 I2C MPU6050，焊到 PA0(SDA)/PA1(SCL)，软件 I2C 位操作。
 *  - OLED 还在硬件 I2C0 (PA28/PA31)，0x3C
 *  - MPU6050 在 PA0/PA1 软件 I2C，0x68，地址不冲突
 *  - 软件 I2C 由 mpu6050.c 里实现，不依赖 I2C0_INST
 *  - v4.22 误用硬件 I2C0 读 serial U41 → 放弃；MPU6050_SDA/SCL 现在是软件 GPIO。
 *  - v4.23 同步把 SERVO 从 PA0 (TIMA0_CCP0) 移到 PA15 (TIMA0_CCP2)，
 *    释放 PA0 给 MPU6050_SDA。LQFP-64(PM) 上 PA15 是 TIMA0_CCP2（不是 CCP3），
 *    选用 CCP2 与用户硬件表"舵机1 PWM: PA15 TIM0_CH2"吻合。 */

/* v4.9：上电 IMU 自检结果（board_init 末尾填） */
volatile uint8_t g_mpu6050_present = 0U;
volatile uint8_t g_dmp_present     = 0U;

/* v4.23 诊断：跳过 IMU_init。怀疑软件 I2C 卡死导致 main loop 进不去时打开。
 *   0 = 正常流程（调 IMU_init）
 *   1 = 跳过 IMU_init，只测 motor / OLED / SysTick 是否正常 */
#ifndef SKIP_IMU_INIT
#define SKIP_IMU_INIT  0
#endif

/* v4.11：g_mpu6050_who 的真正定义在 mpu6050.c（mpu6050_init() 里赋值），
 * 这里通过 include 把它带进来；所以 board.h 也重新 extern 它给 main.c 看。 */

/* 前向声明 */
static void board_systick_init(void);

uint8_t board_init(void)
{
    SYSCFG_DL_init();

    OLED_Init();
    OLED_ColorTurn(0);
    OLED_DisplayTurn(0);
    OLED_Clear();

    NVIC_EnableIRQ(KEY_INT_IRQN);        // GROUP1 共享：按键（当前未接）+ 编码器
    NVIC_EnableIRQ(DC_MOTOR_INT_IRQN);   // 编码器（冗余保险，GROUP1 已含）

    DL_Timer_startCounter(SERVO_INST);
    DL_Timer_setCaptureCompareValue(SERVO_INST, 50, GPIO_SERVO_C2_IDX);

    encoder_init();
    motor_init(1);
    motor_init(2);

    camera_init();   /* MaixCAM UART2 (PA23/PA24) */

    /* v4.22：调 IMU_init() 走完整 eMPL 链路（mpu_init → mpu_dmp_init →
     * 强制唤醒 PWR_MGMT_1/2 → 直读寄存器 bypass）。谁返 0 就
     * OLED 状态条 W:0xHH 给诊断信息。
     * v4.23 诊断：SKIP_IMU_INIT=1 时跳过（用于排查"轮子不转"是不是
     *         软件 I2C 卡死导致的）。 */
#if SKIP_IMU_INIT
    g_mpu6050_present = 0U;
    g_dmp_present     = 0U;
    g_mpu6050_who     = 0xEEU;   /* 标记"跳过" */
#else
    if (IMU_init()) {
        g_mpu6050_present = 1U;
        /* DMP 在 MPU6500 (0x70) 上无固件，但 v4.16 IMU.c 直读寄存器
         * bypass 后 Gz 数据可用，整体 IMU 链路视作 OK。 */
        g_dmp_present = 1U;
    } else {
        g_mpu6050_present = 0U;
        g_dmp_present     = 0U;
        /* g_mpu6050_who 由 IMU_init / mpu6050_init 内部赋值 */
    }
#endif

    /* 左右通道已对调（见 motor.h 的 LEFT_MOTOR_ID / RIGHT_MOTOR_ID 注释） */
    motor_set_direction(LEFT_MOTOR_ID,  1);
    motor_set_direction(RIGHT_MOTOR_ID, 1);
    motor_set_duty(LEFT_MOTOR_ID,  0U);   /* 启动时不转，等待控制环接管 */
    motor_set_duty(RIGHT_MOTOR_ID, 0U);

    control_init();
    car.run_mode = 2;     /* 默认进入循迹模式（TRACE） */
    car.disp_mode = 2;    /* v4.14 默认屏：trace_dbg（M1/M2 duty + Gz + mode+err） */
    soft_timer_init();
    soft_timer_repeat_init(SOFT_TIMER_READ_IMU,     10);
    soft_timer_repeat_init(SOFT_TIMER_SAMPLE_SPEED, 50);
    soft_timer_repeat_init(SOFT_TIMER_HEADING_HOLD, 10);
    soft_timer_repeat_init(SOFT_TIMER_OLED_REFRESH, 30);
    soft_timer_repeat_init(SOFT_TIMER_READ_GRAY,    10);
    soft_timer_repeat_init(SOFT_TIMER_READ_CAMERA, 20);
    for (int i = 0; i < SOFT_TIMER_MAX; ++i) soft_timer_start((soft_timer_type_t)i);

    board_systick_init();   // 1ms SysTick，main 不再 delay

    OLED_Clear();
    return 0;
}

/* —— SysTick 1ms 中断：推进 ms 时基 + 软定时器 ———————————————— */
void SysTick_Handler(void)
{
    g_tick_ms++;
    soft_timer_tick();
}

/* 由 main 在 board_init() 末尾调用一次 */
static void board_systick_init(void)
{
    /* CPUCLK_FREQ = 80MHz → 1ms 需要 80000 个 SysTick 计数 */
    SysTick_Config(CPUCLK_FREQ / 1000U);
    /* 优先级 = 2（数值小 = 高），不能抢编码器/电机 ISR */
    NVIC_SetPriority(SysTick_IRQn, 2);
}

/* 主循环若想手动推进 tick，仍可调（备用） */
void board_tick_inc(void)
{
    g_tick_ms++;
    soft_timer_tick();
}