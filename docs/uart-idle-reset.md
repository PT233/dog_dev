# PA9 低电平与 USART1 IDLE 中断复位问题排查总结

日期：2026-04-27

## 现象

STM32F103 固件启动后，逻辑分析仪观察到 USART1_TX，也就是 PA9，长期保持低电平。设备表现为无法正常进入业务任务，串口也没有稳定输出。

临时测试循环中，PA9 又能保持高电平，因此最初看起来像是 FreeRTOS idle task、调度器、PA9 配置或外部硬件导致的问题。

## 初始判断

PA9 是 USART1_TX。只要程序执行到 `MX_USART1_UART_Init()` 并且 USART1 保持 enable，TX idle 状态应为高电平。

因此 PA9 长期低电平通常不是“串口空闲电平配置错”，而是下面几类问题之一：

- 固件没有真正跑到 USART 初始化
- 初始化后很快复位，PA9 回到默认 floating input 状态
- 程序卡死在中断或错误处理里
- 板上实际运行的不是刚修改后的固件
- 外部电路把 PA9 强行拉低

## 排查路径

### 1. 检查 PA9 初始化

`stm32_fw/Core/Src/usart.c` 中 PA9 配置为：

```c
GPIO_InitStruct.Pin = GPIO_PIN_9;
GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
```

这表示 TX 是复用推挽输出。配置本身没有发现问题。

PA10 RX 后续也被配置为复用输入并上拉，避免 RX 空闲态漂浮：

```c
GPIO_InitStruct.Pin = GPIO_PIN_10;
GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT;
GPIO_InitStruct.Pull = GPIO_PULLUP;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
```

### 2. 排除 FreeRTOS idle task

检查 `FreeRTOSConfig.h` 后确认：

- `configUSE_IDLE_HOOK = 0`
- 没有启用 tickless idle

所以问题不是 idle task 主动改变 GPIO 或让芯片休眠。

但 FreeRTOS 是间接相关的：如果调度器没有机会切到 Safety 任务，IWDG 就不会被刷新，最终导致周期性复位。

### 3. 定位到 USART1 中断空窗期

当时 `usart.c` 里在 `HAL_UART_Init()` 后立即打开了 IDLE 中断：

```c
__HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
```

但真正启动 DMA 接收是在 `Task_UART_RX()` 里：

```c
HAL_UART_Receive_DMA(&huart1, s_uart_dma_buf, UART_RX_DMA_BUF_LEN);
```

这两个动作之间存在启动空窗：

1. `MX_USART1_UART_Init()` 打开 USART1 和 IDLE interrupt
2. `MX_IWDG_Init()` 打开独立看门狗
3. 初始化 FreeRTOS
4. 创建任务
5. 调度器启动
6. `Task_UART_RX()` 才调用 `HAL_UART_Receive_DMA()`

如果这段窗口里 PA10 拾到噪声，或者 USART 产生 RXNE/ORE/IDLE 状态，`USART1_IRQHandler()` 会提前进入。

当时中断处理逻辑是：

```c
if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET)
{
  __HAL_UART_CLEAR_IDLEFLAG(&huart1);
  UartRxTask_NotifyFromIdleIrq();
}

HAL_UART_IRQHandler(&huart1);
```

问题在于：DMA RX 尚未启动时，`huart1.RxState` 仍可能是 `HAL_UART_STATE_READY`。HAL UART IRQ 在这种状态下不一定会按预期消费接收数据并清掉异常状态，可能留下 RXNE/ORE 之类的挂起条件。

结果就是 USART1 IRQ 反复触发，中断风暴占满 CPU，FreeRTOS 任务无法正常运行，Safety 任务无法喂 IWDG，最终形成约 1 秒一次的复位循环。

### 4. 解释 PA9 为什么看起来一直低

复位时，PA9 会回到 STM32 默认输入浮空状态。逻辑分析仪在这种状态下常常读成低电平。

如果固件不断复位，就会出现：

```text
复位阶段：PA9 floating，LA 多数显示 LOW
初始化阶段：PA9 被配置为 USART1_TX，idle HIGH
故障阶段：中断风暴，Safety 任务喂不了狗
IWDG 复位：PA9 再次回到 floating
```

如果逻辑分析仪采样窗口或缩放比例不合适，就容易把这种周期性的“低-短暂高-复位”误看成 PA9 一直低。

### 5. 解释测试循环为什么 PA9 会高

测试循环卡在初始化之后，没有进入完整 FreeRTOS 业务路径，或者测试代码中持续刷新看门狗，因此不会形成同样的复位循环。

只要 USART1 初始化后没有被复位打断，PA9 就会保持 USART TX idle high。

## 最终修复

采用两个修复点。

### 修复 A：延后打开 IDLE 中断

从 `stm32_fw/Core/Src/usart.c` 和 `stm32_keil/Core/Src/usart.c` 删除初始化阶段的：

```c
__HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
```

改为在 DMA RX 真正启动后再打开 IDLE 中断：

```c
if (HAL_UART_Receive_DMA(&huart1, s_uart_dma_buf, UART_RX_DMA_BUF_LEN) != HAL_OK)
{
  Error_Handler();
}

__HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
__HAL_UART_CLEAR_IDLEFLAG(&huart1);
__HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
```

这样保证 IDLE 中断只会在 UART DMA 接收链路准备好之后触发。

### 修复 B：USART1 IRQ 中主动清 ORE

在 `stm32_fw/Core/Src/stm32f1xx_it.c` 和 `stm32_keil/Core/Src/stm32f1xx_it.c` 中，IDLE 处理前增加：

```c
if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE) != RESET)
{
  __HAL_UART_CLEAR_OREFLAG(&huart1);
}
```

这可以防止启动噪声或异常接收造成 ORE 挂住 IRQ。

## 修改文件

- `stm32_fw/Core/Src/usart.c`
- `stm32_fw/Core/Src/uart_rx_task.c`
- `stm32_fw/Core/Src/stm32f1xx_it.c`
- `stm32_keil/Core/Src/usart.c`
- `stm32_keil/Core/Src/uart_rx_task.c`
- `stm32_keil/Core/Src/stm32f1xx_it.c`

## 验证

本地编译验证：

```bash
make -C stm32_fw -j4
```

结果通过，生成：

- `stm32_fw/build/STM32.elf`
- `stm32_fw/build/STM32.hex`
- `stm32_fw/build/STM32.bin`

静态检查：

```bash
git diff --check -- stm32_fw/Core/Src/usart.c \
  stm32_fw/Core/Src/uart_rx_task.c \
  stm32_fw/Core/Src/stm32f1xx_it.c
```

结果通过。

烧录新固件后，PA9 恢复 USART TX idle high，故障消失。

## 额外教训

改完代码并编译通过，不等于板子已经运行新固件。修复后如果 PA9 仍低，第一优先级应确认：

1. 是否烧录了新生成的 `stm32_fw/build/STM32.bin`
2. JLink/OpenOCD 是否 verify 成功
3. 是否烧错了 Keil/GCC 其中一份工程产物
4. `BOOT0` 是否为低，芯片是否从用户 Flash 启动
5. `NRST` 是否仍在周期性跳变
6. PA9 外部连接是否强拉低

这次最后确认“现在好了”，说明核心问题确实在 USART1 IDLE 中断过早打开，以及异常接收标志没有被充分兜底清理。

## 后续建议

- 类似 UART IDLE + DMA 接收结构中，IDLE interrupt 应始终在 DMA RX 启动成功后再 enable。
- 中断处理里对 ORE/FE/NE 等异常标志可以做更完整的统计和清理，便于现场诊断。
- 启动阶段可以临时增加一个 GPIO heartbeat，用来区分“没进 app”“进 app 后复位”和“任务调度异常”。
- 烧录脚本输出中保留 firmware build time 或 git hash，避免现场误测旧固件。
