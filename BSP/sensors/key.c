#include "BSP/sensors/key.h"
#include "BSP/motor/encoder.h"

// 按键事件标志（在 ISR 置位，主循环处理完清零）
//   bit0: KEY11(PB0) 按下 → 请求航向重置
//   bit1: KEY12(PB1) 按下 → 切换 OLED 显示模式
static volatile uint8_t key_event_flags = 0;

void key_clear_event(uint8_t mask)
{
    key_event_flags &= (uint8_t)~mask;
}

uint8_t key_get_events(void)
{
    return key_event_flags;
}

uint8_t get_key_state(uint32_t key) {
    uint32_t high_bits = DL_GPIO_readPins(KEY_PORT, key);
    if((high_bits & key) != 0) return 1;
    else return 0;
}

void GROUP1_IRQHandler(void)
{
    /* 按键板事件（GPIOB 共享）
       KEY9/PB6、KEY10/PB7：旧键位未接，吞掉 pending
       KEY11/PB0：接按键板 SW1 → 航向重置
       KEY12/PB1：接按键板 SW2 → 显示模式切换 */
    uint32_t key_pending = DL_GPIO_getPendingInterrupt(KEY_PORT);
    if (key_pending & KEY_KEY11_IIDX) {
        key_event_flags |= KEY_EVENT_YAW_RESET;
    }
    if (key_pending & KEY_KEY12_IIDX) {
        key_event_flags |= KEY_EVENT_DISPLAY_TOGGLE;
    }
    (void)key_pending;

    /* 编码器事件（共享 GPIOA 端口，AA=电机2，BA=电机1） */
    uint32_t motor_a_pending = DL_GPIO_getPendingInterrupt(GPIOA);
    if (motor_a_pending & DC_MOTOR_AA_IIDX) {
        encoder_process_isr(2);
    }
    if (motor_a_pending & DC_MOTOR_BA_IIDX) {
        encoder_process_isr(1);
    }
}