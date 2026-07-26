/* ===========================================================================
 * gw_grayscale_sensor.h
 *  - 8 路灰度传感器 — 8-wire parallel（辅助板 V1.3 出厂默认模式）
 *  - 主控 8 个 GPIO floating input，**直接读**高/低
 *  - **无 CLK、无 DAT、无时序**——辅助板内部固件已经把 8 路 IR 转成 8 个独立 0/1 数字输出
 *
 * 接线（8 线 + GND 必接）：
 *   辅助板 CH1 ──→ PB27 (H56 pin 1)  → bit0
 *   辅助板 CH2 ──→ PB26 (H56 pin 2)  → bit1
 *   辅助板 CH3 ──→ PB23 (H56 pin 3)  → bit2
 *   辅助板 CH4 ──→ PB13 (H56 pin 4)  → bit3
 *   辅助板 CH5 ──→ PB12 (H56 pin 5)  → bit4
 *   辅助板 CH6 ──→ PB16 (H56 pin 7)  → bit5   (跳 H56 pin 6 = PA11，UART RX 占的)
 *   辅助板 CH7 ──→ PB15 (H56 pin 9)  → bit6   (跳 H56 pin 8 = PA10，UART TX 占的)
 *   辅助板 CH8 ──→ PB5  (H56 pin 10) → bit7
 *   辅助板 GND ──→ 主控任意 GND（**共地必接**）
 *
 * 用法（已对接 board_init + SOFT_TIMER_READ_GRAY 20ms 触发）：
 *   - main 里看 car.gray[0..7] / car.gray_raw（bit0..7 = CH1..CH8，1=白/亮，0=黑/暗）
 *   - 主动调用：gw_gray_read() → 8bit 原始值
 *   - 老接口 gw_gray_serial_read() 是 alias（保留兼容）
 * =========================================================================*/
#ifndef GW_GRAYSCALE_SENSOR_H
#define GW_GRAYSCALE_SENSOR_H

#include <stdint.h>

/* 一次性读 8 路灰度（位 0..7 = 通道 1..8），返回 0..0xFF
 * 实测极性：返回值 1 = 白底（无黑线），0 = 压在黑线上
 * （若以后换辅助板极性反了，改 control.c 里的 GRAY_INVERT 即可）*/
uint8_t gw_gray_read(void);

/* 老接口 alias（保留兼容，新代码请用 gw_gray_read） */
uint8_t gw_gray_serial_read(void);

#endif /* GW_GRAYSCALE_SENSOR_H */