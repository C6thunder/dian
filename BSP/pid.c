/* ===========================================================================
 * pid.c
 *  - DJI 通用 PID 实现（移植自参考项目 pid.c，去掉中英文双语注释）
 * =========================================================================*/
#include "pid.h"
#include <stddef.h>     /* NULL */

#define LIMIT(input, max) do {            \
    if      (input >  max) input =  max;  \
    else if (input < -max) input = -max;  \
} while (0)

void PID_init(pid_type_def *pid, uint8_t mode, const fp32 kpid[3],
              fp32 max_out, fp32 max_iout)
{
    if (pid == NULL || kpid == NULL) return;

    pid->mode     = mode;
    pid->Kp       = kpid[0];
    pid->Ki       = kpid[1];
    pid->Kd       = kpid[2];
    pid->max_out  = max_out;
    pid->max_iout = max_iout;

    pid->Dbuf[0] = pid->Dbuf[1] = pid->Dbuf[2] = 0.0f;
    pid->error[0] = pid->error[1] = pid->error[2] = 0.0f;
    pid->Pout = pid->Iout = pid->Dout = pid->out = 0.0f;
}

fp32 PID_calc(pid_type_def *pid, fp32 ref, fp32 set)
{
    if (pid == NULL) return 0.0f;

    pid->error[2] = pid->error[1];
    pid->error[1] = pid->error[0];
    pid->set = set;
    pid->fdb = ref;
    pid->error[0] = set - ref;

    if (pid->mode == PID_POSITION || pid->mode == PID_POSITION_ANGLE) {
        if (pid->mode == PID_POSITION_ANGLE) {
            /* 过零处理：把目标角差限制到 ±180° */
            if (pid->error[0] >  180.0f) pid->error[0] -= 360.0f;
            if (pid->error[0] < -180.0f) pid->error[0] += 360.0f;
        }

        pid->Pout = pid->Kp * pid->error[0];
        pid->Iout += pid->Ki * pid->error[0];
        pid->Dbuf[2] = pid->Dbuf[1];
        pid->Dbuf[1] = pid->Dbuf[0];
        pid->Dbuf[0] = (pid->error[0] - pid->error[1]);
        pid->Dout = pid->Kd * pid->Dbuf[0];
        LIMIT(pid->Iout, pid->max_iout);
        pid->out = pid->Pout + pid->Iout + pid->Dout;
        LIMIT(pid->out, pid->max_out);
    }
    else if (pid->mode == PID_DELTA) {
        pid->Pout = pid->Kp * (pid->error[0] - pid->error[1]);
        pid->Iout = pid->Ki * pid->error[0];
        pid->Dbuf[2] = pid->Dbuf[1];
        pid->Dbuf[1] = pid->Dbuf[0];
        pid->Dbuf[0] = (pid->error[0] - 2.0f * pid->error[1] + pid->error[2]);
        pid->Dout = pid->Kd * pid->Dbuf[0];
        pid->out += pid->Pout + pid->Iout + pid->Dout;
        LIMIT(pid->out, pid->max_out);
    }
    return pid->out;
}

void PID_clear(pid_type_def *pid)
{
    if (pid == NULL) return;

    pid->error[0] = pid->error[1] = pid->error[2] = 0.0f;
    pid->Dbuf[0] = pid->Dbuf[1] = pid->Dbuf[2] = 0.0f;
    pid->out = pid->Pout = pid->Iout = pid->Dout = 0.0f;
    pid->fdb = pid->set = 0.0f;
}