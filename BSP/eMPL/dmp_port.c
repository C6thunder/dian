/* ===========================================================================
 * dmp_port.c
 *  - eMPL 平台适配：把 inv_mpu.c 用到的 MPU6050 WriteReg/ReadData + mget_ms 映射到本项目
 *  - 这层不要依赖任何 inv_mpu.c 的内部
 * =========================================================================*/
#include "dmp_port.h"
#include "BSP/struct_typedef.h"   // g_tick_ms
#include "ti_msp_dl_config.h"

/* 内部：把位操作 I2C 的 raw 寄存器读写重新暴露为 inv_mpu 的 API。
 * 我们的 mpu6050.c 已经做过 i2c_start/i2c_send_byte... 这里用 SysConfig 生成的
 * I2C 句柄，但 MSPM0 的 I2C0 已被 OLED 占用，所以走 GPIO 位操作。
 * 这里直接复用 mpu6050.c 的位操作：它的函数都是 static，要靠写 reg 一次多字节。 */
#include "BSP/IMU/mpu6050.h"

#define MPU_ADDR   0x68U

/* 把多字节写到 MPU6050（i2c_start + i2c_send_byte 地址/寄存器/数据 + i2c_stop）
 * 约定：0 = 成功，非 0 = 失败（InvenSense 库标准）
 * mpu6050_write_byte() 实际返回 1=成功 / 0=失败，所以这里用 == 0 判断失败 */
static char port_write(uint8_t reg, uint8_t len, const uint8_t *data)
{
    /* 为简化：直接复用 mpu6050 写寄存器接口（单字节版）。
     * 对 DMP 流程来说，多数 write 是单字节 register + 1~N data，足够。 */
    for (uint8_t i = 0; i < len; ++i) {
        if (mpu6050_write_byte(reg + i, data[i]) == 0) return 1;
    }
    return 0;
}

/* 把多字节从 MPU6050 读出 */
static char port_read(uint8_t reg, uint8_t len, uint8_t *data)
{
    /* 暂无连续读实现 → 退化为逐字节读 */
    for (uint8_t i = 0; i < len; ++i) {
        if (mpu6050_read_byte(reg + i, &data[i]) == 0) return 1;
    }
    return 0;
}

char MPU6050_WriteReg(uint8_t addr, uint8_t regaddr, uint8_t num, uint8_t *regdata)
{
    (void)addr;
    return port_write(regaddr, num, regdata);
}

char MPU6050_ReadData(uint8_t addr, uint8_t regaddr, uint8_t num, uint8_t *Read)
{
    (void)addr;
    return port_read(regaddr, num, Read);
}

void mget_ms(unsigned long *count)
{
    *count = (unsigned long)g_tick_ms;
}