# 小车 UART 接口一览

## 板上串口插座

| 插座 | Pin1 | Pin2 | Pin3 | Pin4 | 备注 |
|------|------|------|------|------|------|
| UART0 | +5V | RX(PA1) | TX(PA0) | GND | ⚠️ PA0/PA1 已用于 MPU6050 |
| UART1 | +5V | GND | RX(PB7) | TX(PB6) | ⚠️ PB6/PB7 已用于 KEY9/10 |
| **UART2** | **+5V** | **GND** | **RX(PA24)** | **TX(PA23)** | ✅ **当前接 MaixCAM** |
| UART3 | TX(PA23) | RX(PA24) | GND | 12V | ⚠️ 12V 供电，与 UART2 共用引脚 |
| UART4 | +5V | RX(PB3) | TX(PB2) | GND | ✅ 空闲可用 |
| UART5 | TX(PB6) | RX(PB7) | GND | 12V | ⚠️ 12V 供电，与 UART1 共用引脚 |

## 当前使用

| 插座 | 连接设备 | 实际引脚 | 外设 | 波特率 |
|------|---------|---------|------|--------|
| UART2 | MaixCAM Pro | PA23=TX, PA24=RX | MSPM0 UART2 | 9600 |

## MaixCAM Pro 接线

```
MaixCAM Pro              小车 UART2 插座
A19 (UART1_TX)     →     Pin 3 — RX (PA24)
A18 (UART1_RX)     →     Pin 4 — TX (PA23)
GND                →     Pin 2 — GND
```

## 通信协议

```
[0x6B] [0x5B] [0x5B] [CMD] [0xB3]
   ↑      ↑      ↑      ↑      ↑
 同步   帧头1  帧头2  命令   帧尾
                     0x00 = 无检测
                     0x01~0x08 = 数字1~8
```
