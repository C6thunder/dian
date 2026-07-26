/* ===========================================================================
 * camera.h — MaixCAM ↔ MSPM0
 * UART2: PA23=TX, PA24=RX, 9600-8N1
 * 协议: [0x6B][0x5B][0x5B][CMD][0xB3]  5 字节
 *   CMD=0x00 无, 0x01~0x08=数字1~8
 * =========================================================================*/
#ifndef CAMERA_H
#define CAMERA_H
#include <stdint.h>

#define CAM_FRAME_LEN  5U
#define CAM_SYNC       0x6BU
#define CAM_HEAD1      0x5BU
#define CAM_HEAD2      0x5BU
#define CAM_TAIL       0xB3U

typedef struct {
    uint8_t detected;
    uint8_t cmd;
    uint32_t last_frame_ms;
} camera_result_t;

void camera_init(void);
void camera_poll(void);
const camera_result_t *camera_get_result(void);

extern volatile uint32_t g_cam_rx_bytes;
extern volatile uint32_t g_cam_rx_frames;
#endif
