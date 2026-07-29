#ifndef KEY_H
#define KEY_H

#include "ti_msp_dl_config.h"

uint8_t get_key_state(uint32_t key);

// 按键事件标志（位掩码）
#define KEY_EVENT_YAW_RESET      0x01U   // bit0: 请求航向重置
#define KEY_EVENT_DISPLAY_TOGGLE 0x02U   // bit1: 切换 OLED 显示模式
#define KEY_EVENT_MODE_CYCLE     0x04U   // bit2: 切换 run_mode（0=停 / 1=航向保持 / 2=循迹）

uint8_t key_get_events(void);
void    key_clear_event(uint8_t mask);

#endif