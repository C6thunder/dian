#ifndef MOTOR_H
#define MOTOR_H

#define PI 3.14159
//编码器线数
#define MOTOR_BIANMAQI 324
//轮胎直径mm
#define MOTOR_WHEEL_D 65
#include "ti_msp_dl_config.h"

/* ========== 左右电机通道映射 ==========
 * 当前 PCB：电机 1 通道（AIN1/AIN2 + BA/BB）实际焊在车体**右**侧，
 *         电机 2 通道（BIN1/BIN2 + AA/AB）实际焊在车体**左**侧。
 * 因此软件要表达的"左轮"用 motor_id=2，"右轮"用 motor_id=1。
 *
 * 后续如果硬件重排，请把下面两个宏互换即可。
 *   - LEFT_MOTOR_ID  物理左轮对应 motor_id
 *   - RIGHT_MOTOR_ID 物理右轮对应 motor_id
 */
#define LEFT_MOTOR_ID   2U
#define RIGHT_MOTOR_ID  1U

void motor_init(uint8_t motor_id);
void motor_set_duty(uint8_t motor_id, uint32_t duty);
void motor_set_direction(uint8_t motor_id, uint8_t direction);

#endif //MOTOR_H
