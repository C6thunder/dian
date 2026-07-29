/* ===========================================================================
 * mpu6050.c — v4.23 重写
 *  - v4.17~v4.19 误用硬件 I2C0 → 不可靠；v4.22 改用硬件 I2C0 但模块其实是 serial U41
 *  - v4.23 修正：换上 I2C MPU6050 模块，焊到 PA0(SDA) / PA1(SCL)，
 *    走软件 I2C 位操作（DL_GPIO 切 SDA 方向）
 *  - OLED 还在硬件 I2C0 (PA28/PA31)，不冲突
 *
 *  - SysConfig 里有 GPIO5 = MPU6050_SW_I2C（PA0=MPU6050_SDA / PA1=MPU6050_SCL）
 *  - delay_us 5us 级别（BSP/Delay.h）
 *  - 开漏等效：用输出 + PULL_UP + 高驱动；SDA 切输入用 DL_GPIO_initDigitalInputFeatures
 * =========================================================================*/
#include "BSP/sensors/IMU/mpu6050.h"
#include "BSP/utils/Delay.h"
#include <math.h>

/* v4.22：暴露 WHO_AM_I 原始字节给 OLED 调试
 *   0x68 → 真 MPU6050 应答
 *   0x70 → 真 MPU6500 应答
 *   0x00 → SDA 被短路到地
 *   0xFF → 总线浮空赢上拉（MPU 没接 / NACK 持续）
 *   其他 → 地址错（AD0 拉错 / 挂错芯片）*/
uint8_t g_mpu6050_who = 0xFFU;

/* I2C 总线 7-bit 地址 → DL 期望的 (addr << 1)
 * MPU6050 AD0=GND → 0x68；AD0=VDD → 0x69。我们 AD0 默认拉地，所以 0x68 */
#define MPU6050_I2C_ADDR_8BIT    (MPU6050_ADDR << 1)

/* ---------- 软件 I2C pin 定义（来自 SysConfig GPIO5 = MPU6050_SW_I2C） ----------
 *  SysConfig 生成的名字规则：
 *    - GPIO PORT (port 是 GPIO group 共享的，不是某个 pin 的属性):
 *        MPU6050_SW_I2C_PORT            ← 没有 _SDA / _SCL 后缀
 *    - 每个 pin：MPU6050_SW_I2C_<pin name>_PIN / _IOMUX
 *  PA0 / PA1 在同一 GPIO 上跑软件 I2C，因此 SDA / SCL 共享同一 PORT，PIN/IOMUX 各自。 */
#define MPU_SDA_PORT   MPU6050_SW_I2C_PORT
#define MPU_SDA_PIN    MPU6050_SW_I2C_MPU6050_SDA_PIN
#define MPU_SDA_IOMUX  MPU6050_SW_I2C_MPU6050_SDA_IOMUX
#define MPU_SCL_PORT   MPU6050_SW_I2C_PORT
#define MPU_SCL_PIN    MPU6050_SW_I2C_MPU6050_SCL_PIN
#define MPU_SCL_IOMUX  MPU6050_SW_I2C_MPU6050_SCL_IOMUX

/* I2C 总线频率目标 100 kHz，半周期 ~5us。开漏模拟用 4us 留点余量。 */
#define I2C_HALF_PERIOD_US   4U

/* ---------- 方向切换 ----------
 * v4.23 fix：SDA 必须开漏（HiZ_ENABLE），否则主机驱动高、从机 MPU6050 拉低 ACK 时
 * 直接打穿 → ACK 永远读 1（看起来 NACK）→ WHO_AM_I 失败 → MPU 永远初始化不了。
 * SCL 主机独享，保持 push-pull。 */
static inline void sda_to_input(void)
{
    /* 切到输入：从机可以主动拉低 ACK；上拉电阻仍开启 */
    DL_GPIO_initDigitalInputFeatures(MPU_SDA_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
}

static inline void sda_to_output(void)
{
    /* 切到输出，但开漏（HiZ_ENABLE）：DOUT=1 时高阻浮空 = 由上拉拉高；
     * DOUT=0 时主动拉低。这样 sda_high()/sda_low() 走同一组寄存器，
     * 不需要每次切方向。 */
    DL_GPIO_initDigitalOutputFeatures(MPU_SDA_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_ENABLE);
    DL_GPIO_enableOutput(MPU_SDA_PORT, MPU_SDA_PIN);
}

static inline void scl_to_output(void)
{
    /* SCL 主机独占，push-pull 即可（速度更稳） */
    DL_GPIO_initDigitalOutput(MPU_SCL_IOMUX);
    DL_GPIO_enableOutput(MPU_SCL_PORT, MPU_SCL_PIN);
}

static inline void sda_high(void)
{
    DL_GPIO_setPins(MPU_SDA_PORT, MPU_SDA_PIN);
}

static inline void sda_low(void)
{
    DL_GPIO_clearPins(MPU_SDA_PORT, MPU_SDA_PIN);
}

static inline void scl_high(void)
{
    DL_GPIO_setPins(MPU_SCL_PORT, MPU_SCL_PIN);
}

static inline void scl_low(void)
{
    DL_GPIO_clearPins(MPU_SCL_PORT, MPU_SCL_PIN);
}

static inline uint8_t sda_read(void)
{
    return (DL_GPIO_readPins(MPU_SDA_PORT, MPU_SDA_PIN) != 0U) ? 1U : 0U;
}

/* ---------- 软件 I2C 基本时序 ---------- */

static void i2c_start(void)
{
    /* SDA 高、SCL 高 → SDA 拉低（START） */
    sda_high();
    delay_us(I2C_HALF_PERIOD_US);
    scl_high();
    delay_us(I2C_HALF_PERIOD_US);
    sda_low();
    delay_us(I2C_HALF_PERIOD_US);
    scl_low();
    delay_us(I2C_HALF_PERIOD_US);
}

static void i2c_stop(void)
{
    /* SDA 低、SCL 低 → SCL 高 → SDA 高（STOP） */
    sda_low();
    delay_us(I2C_HALF_PERIOD_US);
    scl_high();
    delay_us(I2C_HALF_PERIOD_US);
    sda_high();
    delay_us(I2C_HALF_PERIOD_US);
}

/* 写 1 字节；返回 0 = ACK，1 = NACK */
static uint8_t i2c_write_byte(uint8_t byte)
{
    sda_to_output();
    for (uint8_t i = 0; i < 8; i++) {
        if (byte & 0x80U) sda_high();
        else sda_low();
        delay_us(I2C_HALF_PERIOD_US);
        scl_high();
        delay_us(I2C_HALF_PERIOD_US);
        scl_low();
        delay_us(I2C_HALF_PERIOD_US);
        byte <<= 1;
    }
    /* 读 ACK：SDA 切输入，slave 拉低 = ACK */
    sda_to_input();
    sda_high();
    delay_us(I2C_HALF_PERIOD_US);
    scl_high();
    delay_us(I2C_HALF_PERIOD_US);
    uint8_t ack = sda_read();
    scl_low();
    delay_us(I2C_HALF_PERIOD_US);
    sda_to_output();
    return ack;   /* 0 = ACK, 1 = NACK */
}

/* 读 1 字节；send_ack = 1 主机发 ACK，0 主机发 NACK */
static uint8_t i2c_read_byte(uint8_t send_ack)
{
    uint8_t byte = 0;
    sda_to_input();
    sda_high();
    for (uint8_t i = 0; i < 8; i++) {
        scl_high();
        delay_us(I2C_HALF_PERIOD_US);
        byte = (uint8_t)((byte << 1) | sda_read());
        scl_low();
        delay_us(I2C_HALF_PERIOD_US);
    }
    sda_to_output();
    if (send_ack) sda_low();   /* ACK */
    else          sda_high();  /* NACK */
    delay_us(I2C_HALF_PERIOD_US);
    scl_high();
    delay_us(I2C_HALF_PERIOD_US);
    scl_low();
    delay_us(I2C_HALF_PERIOD_US);
    return byte;
}

/* ---------- 顶层 API（与 v4.22 接口完全一致） ---------- */

uint8_t mpu6050_init(void)
{
    /* 上电让内部 regulator 稳定 */
    delay_ms(50);

    /* 软件 I2C 时序前先确保 SDA/SCL 都在高（PULL_UP 起效） */
    sda_to_output();
    scl_to_output();
    sda_high();
    scl_high();
    delay_us(I2C_HALF_PERIOD_US);

    /* WHO_AM_I 探测 */
    g_mpu6050_who = 0xFFU;
    i2c_start();
    if (i2c_write_byte(MPU6050_I2C_ADDR_8BIT) != 0U) {
        i2c_stop();
        return 0;
    }
    if (i2c_write_byte(MPU6050_REG_WHO_AM_I) != 0U) {
        i2c_stop();
        return 0;
    }
    i2c_start();
    if (i2c_write_byte((uint8_t)(MPU6050_I2C_ADDR_8BIT | 1U)) != 0U) {
        i2c_stop();
        return 0;
    }
    g_mpu6050_who = i2c_read_byte(0);   /* NACK */
    i2c_stop();

    if (!WHO_AM_I_ACCEPTED(g_mpu6050_who)) {
        return 0;
    }

    /* 清 SLEEP 位（默认上电 SLEEP=1，复位后采样静止）。
     * 选 CLKSEL=0（内部 8 MHz osc，足够采样 100Hz）。 */
    mpu6050_write_byte(MPU6050_REG_PWR_MGMT_1, 0x00);
    delay_ms(10);

    /* 采样率：1 kHz / (1+SMPLRT_DIV)。SMPLRT_DIV=9 → 100 Hz。 */
    mpu6050_write_byte(MPU6050_REG_SMPLRT_DIV, 9);

    /* DLPF=3：accel BW 44 Hz，gyro BW 42 Hz。 */
    mpu6050_write_byte(MPU6050_REG_CONFIG, 3);

    /* 陀螺 FS=0x18 → ±2000 °/s（最高量程，灵敏度 16.4 LSB/(°/s)） */
    mpu6050_write_byte(MPU6050_REG_GYRO_CONFIG, 0x18);

    /* accel FS=0x00 → ±2g（最高灵敏度 16384 LSB/g） */
    mpu6050_write_byte(MPU6050_REG_ACCEL_CONFIG, 0x00);

    return 1;
}

/* v4.23：单字节寄存器写 [reg, val] */
uint8_t mpu6050_write_byte(uint8_t reg, uint8_t val)
{
    uint8_t ok = 1;

    i2c_start();
    if (i2c_write_byte(MPU6050_I2C_ADDR_8BIT) != 0U) { ok = 0; goto out; }
    if (i2c_write_byte(reg) != 0U) { ok = 0; goto out; }
    if (i2c_write_byte(val) != 0U) { ok = 0; goto out; }
out:
    i2c_stop();
    return ok;
}

/* v4.23：单字节寄存器读 */
uint8_t mpu6050_read_byte(uint8_t reg, uint8_t *val)
{
    if (!val) return 0;

    i2c_start();
    if (i2c_write_byte(MPU6050_I2C_ADDR_8BIT) != 0U) {
        i2c_stop();
        return 0;
    }
    if (i2c_write_byte(reg) != 0U) {
        i2c_stop();
        return 0;
    }
    i2c_start();
    if (i2c_write_byte((uint8_t)(MPU6050_I2C_ADDR_8BIT | 1U)) != 0U) {
        i2c_stop();
        return 0;
    }
    *val = i2c_read_byte(0);   /* NACK（最后一个字节） */
    i2c_stop();
    return 1;
}

/* v4.23：连续寄存器 burst 读 */
static uint8_t mpu6050_read_regs(uint8_t reg, uint8_t *buf, uint8_t len)
{
    if (!buf || len == 0) return 0;

    i2c_start();
    if (i2c_write_byte(MPU6050_I2C_ADDR_8BIT) != 0U) {
        i2c_stop();
        return 0;
    }
    if (i2c_write_byte(reg) != 0U) {
        i2c_stop();
        return 0;
    }
    i2c_start();
    if (i2c_write_byte((uint8_t)(MPU6050_I2C_ADDR_8BIT | 1U)) != 0U) {
        i2c_stop();
        return 0;
    }
    for (uint8_t i = 0; i < len; i++) {
        /* 最后一个字节之前都发 ACK */
        buf[i] = i2c_read_byte((i + 1U < len) ? 1U : 0U);
    }
    i2c_stop();
    return 1;
}

/* 14 字节一次性读：accel(6) + temp(2) + gyro(6) */
void mpu6050_read(MPU6050_Data_t *data)
{
    uint8_t buf[14];

    if (!mpu6050_read_regs(MPU6050_REG_ACCEL_XOUT_H, buf, 14)) {
        return;
    }

    data->ax  = (int16_t)((buf[0]  << 8) | buf[1]);
    data->ay  = (int16_t)((buf[2]  << 8) | buf[3]);
    data->az  = (int16_t)((buf[4]  << 8) | buf[5]);
    data->temp = (int16_t)((buf[6] << 8) | buf[7]);
    data->gx  = (int16_t)((buf[8]  << 8) | buf[9]);
    data->gy  = (int16_t)((buf[10] << 8) | buf[11]);
    data->gz  = (int16_t)((buf[12] << 8) | buf[13]);

    data->ax_g   = (float)data->ax / MPU6050_ACCEL_SENS;
    data->ay_g   = (float)data->ay / MPU6050_ACCEL_SENS;
    data->az_g   = (float)data->az / MPU6050_ACCEL_SENS;
    data->gx_dps = (float)data->gx / MPU6050_GYRO_SENS;
    data->gy_dps = (float)data->gy / MPU6050_GYRO_SENS;
    data->gz_dps = (float)data->gz / MPU6050_GYRO_SENS;
    data->temp_c = (float)data->temp / 340.0f + 36.53f;

    data->pitch_accel = atan2f(-data->ax_g,
        sqrtf(data->ay_g * data->ay_g + data->az_g * data->az_g)) * 57.29578f;
    data->roll_accel  = atan2f(data->ay_g, data->az_g) * 57.29578f;
}

uint8_t mpu6050_read_gyro_raw(int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t buf[6];
    if (!mpu6050_read_regs(MPU6050_REG_GYRO_XOUT_H, buf, 6)) return 0;
    if (gx) *gx = (int16_t)((buf[0] << 8) | buf[1]);
    if (gy) *gy = (int16_t)((buf[2] << 8) | buf[3]);
    if (gz) *gz = (int16_t)((buf[4] << 8) | buf[5]);
    return 1;
}

/* 互补滤波：gyro 积分 + accel 校准 */
void mpu6050_update_attitude(MPU6050_Data_t *data, float dt)
{
    const float alpha = 0.98f;
    if (dt <= 0.0f) dt = 0.01f;

    data->pitch = alpha * (data->pitch + data->gy_dps * dt) +
                  (1.0f - alpha) * data->pitch_accel;
    data->roll  = alpha * (data->roll  + data->gx_dps * dt) +
                  (1.0f - alpha) * data->roll_accel;
    data->yaw  += data->gz_dps * dt;
}
