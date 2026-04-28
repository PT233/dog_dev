# UART IDLE 与解析器重置

本文档描述 STM32 侧当前的收包策略。

## 1. 收包方式

文件：`stm32_keil/Core/Src/uart_rx_task.c`

当前流程：

1. `HAL_UART_Receive_DMA()` 启动 DMA 接收
2. 关闭半传输中断
3. 开启 `UART_IT_IDLE`
4. `USART1_IRQHandler` 里通过 `UartRxTask_NotifyFromIdleIrq()` 唤醒任务
5. 任务根据 DMA 写指针消费新增字节

## 2. 帧解析状态

解析器按字节推进，识别：

- 帧头 `0xAA 0x55`
- `cmd_id`
- `payload_len`
- `crc16`
- 帧尾 `0x0D`

## 3. 什么时候重置解析器

出现以下情况会清空当前帧缓存：

- 第二个字节不是 `0x55`
- 预期长度小于最小帧长或超过 `UART_MAX_FRAME_LEN`
- 当前缓存长度超过上限
- 收到完整长度后帧尾不对

## 4. 为什么需要这个机制

- 串口读流不是按帧到达
- IDLE 中断只表示“当前一段数据结束”，不保证内容合法
- 解析器必须能从垃圾字节中自恢复

## 5. 现状

当前实现已经具备：

- 从乱流中找回帧头
- CRC 校验失败直接丢弃
- 非 `ACTIVE` 状态下拒绝执行舵机控制
