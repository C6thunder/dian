/* ===========================================================================
 * camera.c — UART2 由 SysConfig 管理，这里只做 NVIC + ISR + 协议解析
 * 协议: [0x6B][0x5B][0x5B][CMD][0xB3]
 * =========================================================================*/
#include "BSP/sensors/camera.h"
#include "BSP/struct_typedef.h"
#include "ti_msp_dl_config.h"

/* ---- 协议常量 ---- */
#define CAM_SYNC   0x6BU
#define CAM_HEAD1  0x5BU
#define CAM_HEAD2  0x5BU
#define CAM_TAIL   0xB3U

/* ---- Ring Buffer ---- */
#define CAM_RB_SIZE  64U
static volatile uint8_t  g_rb[CAM_RB_SIZE];
static volatile uint16_t g_rb_w, g_rb_r;

/* ---- 帧解析 ---- */
typedef enum { S_IDLE, S_BODY } st_t;
static st_t    g_st = S_IDLE;
static uint8_t g_buf[4];
static uint8_t g_bi;

/* ---- 调试计数 ---- */
volatile uint32_t g_cam_rx_bytes  = 0;
volatile uint32_t g_cam_rx_frames = 0;
static camera_result_t g_res;

static void parse(void)
{
    uint8_t cmd = g_buf[2];
    g_cam_rx_frames++;
    g_res.last_frame_ms = g_tick_ms;
    if (cmd >= 0x01 && cmd <= 0x08) {
        g_res.detected = 1;
        g_res.cmd      = cmd;
    } else {
        g_res.detected = 0;
    }
}

/* ---- 初始化: 硬件由 SYSCFG_DL_MaixCAM_init() 完成，这里只开 NVIC ---- */
void camera_init(void)
{
    NVIC_ClearPendingIRQ(MaixCAM_INST_INT_IRQN);
    NVIC_EnableIRQ(MaixCAM_INST_INT_IRQN);
    NVIC_SetPriority(MaixCAM_INST_INT_IRQN, 3);
    g_rb_w = g_rb_r = 0;
    g_st = S_IDLE;
}

/* ---- 主循环: 直读 FIFO + 消费 ringbuf ---- */
static void proc_byte(uint8_t b)
{
    g_cam_rx_bytes++;
    DL_GPIO_togglePins(GPIOB, LED_LED1_PIN);
    switch (g_st) {
    case S_IDLE:
        if (b == CAM_SYNC) { g_st = S_BODY; g_bi = 0; }
        break;
    case S_BODY:
        g_buf[g_bi++] = b;
        if (g_bi >= 4) {
            g_st = S_IDLE;
            if (g_buf[0]==CAM_HEAD1 && g_buf[1]==CAM_HEAD2 && g_buf[3]==CAM_TAIL)
                parse();
        }
        break;
    }
}

void camera_poll(void)
{
    while (!DL_UART_Main_isRXFIFOEmpty(MaixCAM_INST))
        proc_byte(DL_UART_Main_receiveData(MaixCAM_INST));

    uint16_t w = g_rb_w;
    while (g_rb_r != w) {
        proc_byte(g_rb[g_rb_r]);
        g_rb_r = (uint16_t)(g_rb_r + 1U) % CAM_RB_SIZE;
    }
}

const camera_result_t *camera_get_result(void) { return &g_res; }

/* ---- ISR（对齐参考项目 MaixCAM_INST_IRQHandler）---- */
void UART2_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(MaixCAM_INST)) {
    case DL_UART_MAIN_IIDX_RX: {
        uint8_t b = DL_UART_Main_receiveData(MaixCAM_INST);
        uint16_t n = (uint16_t)(g_rb_w + 1U) % CAM_RB_SIZE;
        if (n != g_rb_r) { g_rb[g_rb_w] = b; g_rb_w = n; }
        break;
    }
    default: break;
    }
}
