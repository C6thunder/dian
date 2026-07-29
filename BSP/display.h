/*
 * display.h — OLED 显示模块
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include "BSP/struct_typedef.h"

/* 设置外部 IMU 诊断变量（由 sensors 模块写，display 模块读） */
void display_set_imu_diag(uint8_t dmp_rc, uint8_t gyro_rc,
                          uint32_t read_cnt, uint32_t fail_cnt);

/* 设置外部 IMU 姿态变量 */
void display_set_imu_pose(float pitch, float roll, float yaw,
                          float gx, float gy, float gz);

/* 按 car.disp_mode 刷新 OLED */
void display_refresh(void);

#endif
