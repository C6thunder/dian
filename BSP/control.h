/*
 * control.h — 简洁 PD 循迹 + 航向保持
 */
#ifndef CONTROL_H
#define CONTROL_H

#include "struct_typedef.h"

void control_init(void);
void control_heading_hold(void);
void control_line_trace(void);

#endif /* CONTROL_H */
