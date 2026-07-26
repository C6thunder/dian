/* ===========================================================================
 * struct_typedef.h
 *  - 常用基础类型别名
 *  - 项目全局数据结构（Car 整车状态）
 * =========================================================================*/
#ifndef STRUCT_TYPEDEF_H
#define STRUCT_TYPEDEF_H

#include <stdint.h>
#include <stdbool.h>

typedef bool    bool_t;
typedef float   fp32;
typedef double  fp64;

/* 整车状态（最高层，所有控制/显示都围绕它） */
typedef struct {
    /* 当前航向基准（按 SW1 重置，或上电为 0） */
    float yaw_ref;

    /* IMU 实时姿态（由 IMU 模块每帧更新） */
    float pitch;
    float roll;
    float yaw;            // 相对 yaw_ref，可正可负，绕 Z 累积
    float gz_dps;         // 陀螺 Z 角速度（°/s），用于 PD 的 D 项

    /* 编码器 */
    int32_t left_count;
    int32_t right_count;
    fp32    left_rpm;
    fp32    right_rpm;
    fp32    left_ms;
    fp32    right_ms;

    /* 电机输出（CCR，0..4000） */
    uint16_t left_duty;
    uint16_t right_duty;

    /* 灰度传感器（8 路数字 0/1） */
    uint8_t gray[8];
    uint8_t gray_raw;     // 8 bit-packed

    /* 运行模式 */
    uint8_t run_mode;     // 0=停  1=直行  2=循迹（灰度）  ...
    uint8_t disp_mode;    // OLED 显示模式（0=全信息 1=IMU 大字）

} Car_t;

extern Car_t car;

/* 软件定时器帧号（毫秒）：main 循环每帧 +1，做时间基准 */
extern volatile uint32_t g_tick_ms;

#endif /* STRUCT_TYPEDEF_H */
