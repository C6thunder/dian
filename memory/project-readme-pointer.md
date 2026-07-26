---
name: project-readme-pointer
description: "Pointer to the project's portable context file (readme.md) so it can be recalled quickly"
metadata: 
  node_type: memory
  type: project
  originSessionId: 0d00e125-1389-4764-9148-f4595d7d8630
---

本项目的完整上下文（硬件 / 引脚 / 工具链 / 踩坑记录 / 已实现功能 / 待办）保存在：

`C:/Users/thunder/workspace_ccstheia/09_PWM_SERVO/readme.md`

**Why:** 每次新会话不必重新读 `empty.syscfg` / `main.c` / `mpu6050.c` 一堆文件来推断状态。这份 readme 是便携版，复制到另一台电脑的 Claude 也能读懂。

**How to apply:** 当需要确认当前引脚分配、按键功能映射、构建命令、`DUTY_*` / `HEADING_*` 参数值、或"为什么这里这么写"的解释时，先读 readme.md，再决定要不要看源码细节。

相关：[[ti-mspm0g3507-pitfalls]] 记录 SysConfig 那些坑（I2C 引脚复用、宏名顺序、PA10/PA11 单向）。