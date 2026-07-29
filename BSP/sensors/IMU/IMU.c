/* ===========================================================================
 * IMU.c
 *  - DMP 高层封装（基于 BSP/eMPL/inv_mpu.c + inv_mpu_dmp_motion_driver.c）
 *  - IMU_init() 会调 mpu_init() + mpu_dmp_init()（InvenSense MotionDriver API）
 *  - IMU_getData() 调 mpu_dmp_get_data() 拿欧拉角
 *
 * v4.23：底层 I2C 改软件位操作（PA0/PA1），接口不变
 *  - mpu6050_read_gyro_raw / mpu6050_write_byte / mpu6050_read_byte 行为不变
 *  - 这里只调底层 API，pin 改动对上层透明
 * =========================================================================*/
#include "BSP/sensors/IMU/IMU.h"
#include "BSP/sensors/eMPL/inv_mpu.h"
#include "BSP/sensors/eMPL/inv_mpu_dmp_motion_driver.h"
#include "BSP/sensors/IMU/mpu6050.h"
#include "BSP/struct_typedef.h"   // g_tick_ms
#include "BSP/utils/Delay.h"

/* v4.16.2：PWR_MGMT_1 / PWR_MGMT_2 在 wakeup 后的实际值，给 OLED 状态条读 */
uint8_t g_pwr_mgmt_1 = 0xFF;
uint8_t g_pwr_mgmt_2 = 0xFF;

uint8_t IMU_init(void)
{
    /* 先把底层寄存器都配好（唤醒、DLPF、量程、采样率） */
    if (!mpu6050_init()) return 0;

    /* InvenSense MotionDriver 初始化 */
    if (mpu_init() != 0)   return 0;

    /* 启用 DMP，加载 3KB 固件，配置 FIFO 速率 100Hz
     * 注意：MPU6500（W:0x70）没有可用 DMP 固件，这一调用必失败。
     *       不影响后续：后面我们手动绕过 eMPL 走直读寄存器。 */
    (void)mpu_dmp_init();   /* 不管返码 */

    /* v4.16【关键】eMPL mpu_init() 末尾 (line 874) 调 mpu_set_sensors(0)，
     * 它把 PWR_MGMT_1 = BIT_SLEEP=0x40（休眠）+ PWR_MGMT_2 = 0x3F
     * （X/Y/Z gyro + Accel 全部 STBY）。本来 mpu_dmp_init() 成功后
     * eMPL 会再调 mpu_set_sensors(INV_XYZ_GYRO|...) 唤醒它，但
     * MPU6500 的 DMP 路径走不通，唤醒这一动作永远到不了。
     *
     * 后果：MPU 实质在睡觉，0x43-0x48 寄存器永远不更新，gz_raw = 0。
     *
     * 修法：不管 DMP 成败，强制把芯片拉醒：
     *   PWR_MGMT_1 = 0x00  ← SLEEP=0, CLKSEL=0（内部 8MHz osc，足够）
     *   PWR_MGMT_2 = 0x00  ← XYZ gyro + Accel 全部解除 standby
     * 直接走我们的 mpu6050_write_byte，绕过 eMPL 的 sensor mask 检查。 */
    if (mpu6050_write_byte(MPU6050_REG_PWR_MGMT_1, 0x01) == 0) return 0;
    if (mpu6050_write_byte(MPU6050_REG_PWR_MGMT_2, 0x00) == 0) return 0;

    /* 等 100ms 让 PLL 锁 + 时钟稳定 */
    delay_ms(100);

    /* 重新设一次采样率 + 量程：mpu_init 设过但 mpu_dmp_init 可能改写，
     * 保险起见再写一遍。 */
    if (mpu6050_write_byte(0x19 /* SMPLRT_DIV */, 9)   == 0) return 0;  /* 100Hz */
    if (mpu6050_write_byte(0x1A /* CONFIG     */, 3)   == 0) return 0;  /* DLPF=3 */
    if (mpu6050_write_byte(0x1B /* GYRO_CONFIG */, 0x18)== 0) return 0;  /* ±2000°/s */

    /* v4.16.2：回读 PWR_MGMT_1 / PWR_MGMT_2 确认写入真的生效 */
    uint8_t p1 = 0xFF, p2 = 0xFF;
    mpu6050_read_byte(MPU6050_REG_PWR_MGMT_1, &p1);
    mpu6050_read_byte(MPU6050_REG_PWR_MGMT_2, &p2);
    /* 暴露给 OLED 状态条用 */
    g_pwr_mgmt_1 = p1;
    g_pwr_mgmt_2 = p2;

    return 1;
}

uint8_t IMU_getData(float *pitch, float *roll, float *yaw)
{
    /* v4.16：MPU6500 没 DMP 固件，DMP 欧拉角永远拿不到。
     * 继续维持 mpu_dmp_get_data 调用（保持 eMPL 路径在编译过线），
     * 返回码给上层：1=FIFO 空 / 2=四元数还没就绪 */
    uint8_t rc = mpu_dmp_get_data(pitch, roll, yaw);
    return rc;
}

uint8_t IMU_getGyro(float *gx, float *gy, float *gz,
                    int16_t *gx_raw, int16_t *gy_raw, int16_t *gz_raw)
{
    int16_t data[3] = {0};
    /* v4.13：直接读 0x43 寄存器，不再走 mpu_get_gyro_reg
     * 原因：mpu_get_gyro_reg 内部检查 st.chip_cfg.sensors & INV_XYZ_GYRO，
     * 这要求 DMP 初始化成功。对 MPU6500 模块（用户的实际硬件，
     * WHO_AM_I = 0x70）来说 DMP 路径不通，所以 gz 永远为 0。
     * 走 mpu6050_read_gyro_raw 直接读寄存器，跳过 sensor mask 检查 */
    if (!mpu6050_read_gyro_raw(&data[0], &data[1], &data[2])) return 1;
    /* v4.15：把原始 int16 也吐出去，OLED 看 I2C 到底有没有拿到数据 */
    if (gx_raw) *gx_raw = data[0];
    if (gy_raw) *gy_raw = data[1];
    if (gz_raw) *gz_raw = data[2];
    /* 量程 ±2000°/s → 16.4 LSB/(°/s) */
    *gx = (float)data[0] / 16.4f;
    *gy = (float)data[1] / 16.4f;
    *gz = (float)data[2] / 16.4f;
    return 0;
}