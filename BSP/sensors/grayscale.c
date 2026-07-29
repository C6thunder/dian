/* ===========================================================================
 * grayscale.c
 *  - 8 路灰度传感器 — 8-wire parallel（辅助板 V1.3 出厂默认）
 *  - 8 个 GPIO floating input 直读，无 CLK/DAT 时序
 *  - 接线 + 引脚约定见 gw_grayscale_sensor.h
 *  - 8 个 PIN/IOMUX 宏由 SysConfig 生成（empty.syscfg → GRAY1..GRAY8）
 *  - 注：当前 8 个 pin 都在 GPIOB 上，所以模块级 GRAY_PORT = GPIOB；
 *        若以后分到 GPIOA/B 两个端口，得换成 per-pin _PORT 宏（GRAY_GRAYx_PORT）
 * =========================================================================*/
#include "BSP/sensors/gw_grayscale_sensor.h"
#include "ti_msp_dl_config.h"

/* SysConfig 生成的端口/位掩码表（按 CH1..CH8 顺序对应 bit0..bit7） */
typedef struct {
    GPIO_Regs *port;
    uint32_t   pin;
} GrayPin_t;

static const GrayPin_t kGrayPins[8] = {
    { GRAY_PORT, GRAY_GRAY1_PIN },   /* CH1 → bit0 */
    { GRAY_PORT, GRAY_GRAY2_PIN },   /* CH2 → bit1 */
    { GRAY_PORT, GRAY_GRAY3_PIN },   /* CH3 → bit2 */
    { GRAY_PORT, GRAY_GRAY4_PIN },   /* CH4 → bit3 */
    { GRAY_PORT, GRAY_GRAY5_PIN },   /* CH5 → bit4 */
    { GRAY_PORT, GRAY_GRAY6_PIN },   /* CH6 → bit5 */
    { GRAY_PORT, GRAY_GRAY7_PIN },   /* CH7 → bit6 */
    { GRAY_PORT, GRAY_GRAY8_PIN },   /* CH8 → bit7 */
};

/* 一次性读 8 路灰度 → 拼成 1 byte
 * bit i = 1 表示 CH(i+1) 亮（白/无黑线），0 表示压在黑线上
 * 主循环每 20ms 调一次，结果写到 car.gray_raw + car.gray[8] */
uint8_t gw_gray_read(void)
{
    uint8_t v = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        if (DL_GPIO_readPins(kGrayPins[i].port, kGrayPins[i].pin)) {
            v |= (uint8_t)(1U << i);
        }
    }
    return v;
}

/* 老接口 alias（保留兼容） */
uint8_t gw_gray_serial_read(void)
{
    return gw_gray_read();
}