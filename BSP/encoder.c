#include "encoder.h"

// 索引 0 保留未用，1 对应 TB6612 B 通道（电机1），2 对应 A 通道（电机2）
static Encoder_t g_encoder[3] = {0};

void encoder_init(void)
{
    for (int i = 0; i < 3; i++) {
        g_encoder[i].count      = 0;
        g_encoder[i].last_count = 0;
        g_encoder[i].rpm        = 0.0f;
        g_encoder[i].speed_ms   = 0.0f;
    }
}

int32_t encoder_get_count(uint8_t motor_id)
{
    if (motor_id < 1 || motor_id > 2) return 0;
    return g_encoder[motor_id].count;
}

void encoder_clear_count(uint8_t motor_id)
{
    if (motor_id < 1 || motor_id > 2) return;
    g_encoder[motor_id].count = 0;
}

// 一倍频正交解码：A 相上升沿时读 B 相电平判断方向
// B = HIGH -> 正转，count++
// B = LOW  -> 反转，count--
void encoder_process_isr(uint8_t motor_id)
{
    if (motor_id == 1) {
        // 电机1编码器：BA(PA25)=A，BB(PA14)=B
        if (DL_GPIO_readPins(DC_MOTOR_BB_PORT, DC_MOTOR_BB_PIN)) {
            g_encoder[1].count++;
        } else {
            g_encoder[1].count--;
        }
    } else if (motor_id == 2) {
        // 电机2编码器：AA(PA26)=A，AB(PA27)=B
        if (DL_GPIO_readPins(DC_MOTOR_AB_PORT, DC_MOTOR_AB_PIN)) {
            g_encoder[2].count++;
        } else {
            g_encoder[2].count--;
        }
    }
}

// 周期采样：基于两次采样间的脉冲增量计算转速和轮速
void encoder_sample_speed(uint32_t sample_period_ms)
{
    if (sample_period_ms == 0U) return;

    const float time_s     = (float)sample_period_ms / 1000.0f;
    /* counts_rev 直接来自宏，避免重复乘法 */
    const float counts_rev = (float)COUNTS_PER_WHEEL_REV;        /* 1× = 364 */
    const float wheel_circ = PI_F * (float)WHEEL_DIAMETER_MM / 1000.0f;

    for (uint8_t i = 1; i <= 2; i++) {
        int32_t delta = g_encoder[i].count - g_encoder[i].last_count;
        g_encoder[i].last_count = g_encoder[i].count;

        /* rps = delta / (counts_rev * dt)
         *  1× 解码 364 count/rev（车轮转速）, 4× = 1456 → 切到 4× 时把 DECODE_MODE 改 4 */
        float rps = (float)delta / (counts_rev * time_s);
        g_encoder[i].rpm      = rps * 60.0f;       // 输出轴 rpm（车轮）
        g_encoder[i].speed_ms = rps * wheel_circ;  // m/s
    }
}

float encoder_get_rpm(uint8_t motor_id)
{
    if (motor_id < 1 || motor_id > 2) return 0.0f;
    return g_encoder[motor_id].rpm;
}

float encoder_get_speed_ms(uint8_t motor_id)
{
    if (motor_id < 1 || motor_id > 2) return 0.0f;
    return g_encoder[motor_id].speed_ms;
}
