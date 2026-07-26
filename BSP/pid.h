/* ===========================================================================
 * pid.h
 *  - DJI 风格通用 PID（移植自参考项目，POSITION / DELTA / POSITION_ANGLE 三种模式）
 *  - PID_calc 参数顺序：(pid, ref, set) 与参考一致（注意：ref 在前，set 在后）
 * =========================================================================*/
#ifndef PID_H
#define PID_H

#include "struct_typedef.h"

enum PID_MODE {
    PID_POSITION = 0,           // 普通位置式 PID
    PID_DELTA,                  // 增量式 PID
    PID_POSITION_ANGLE,         // 角度环位置式 PID（自动过零 ±180°）
};

typedef struct {
    uint8_t mode;
    fp32    Kp, Ki, Kd;
    fp32    max_out;            // 总输出限幅
    fp32    max_iout;           // 积分项限幅

    fp32    set, fdb;
    fp32    out;
    fp32    Pout, Iout, Dout;
    fp32    Dbuf[3];            // 微分项历史（0=本次 1=上次 2=上上次）
    fp32    error[3];           // 误差历史（0=本次 1=上次 2=上上次）
} pid_type_def;

/* mode: PID_POSITION / PID_DELTA / PID_POSITION_ANGLE
 * kp/ki/kd 通过 kpid[3] = {kp, ki, kd} 传入 */
void PID_init(pid_type_def *pid, uint8_t mode, const fp32 kpid[3],
              fp32 max_out, fp32 max_iout);

/* 计算一次 PID 输出：
 *   ref = 反馈值（如实际 yaw / 编码器计数值）
 *   set = 目标值（如 0 / 期望速度）
 *   返回：本次输出（已限幅） */
fp32 PID_calc(pid_type_def *pid, fp32 ref, fp32 set);

/* 清空 PID 内部状态（切模式 / 急停时用） */
void PID_clear(pid_type_def *pid);

#endif /* PID_H */