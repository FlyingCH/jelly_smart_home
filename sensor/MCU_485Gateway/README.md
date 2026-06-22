# PowerAcquisition Project

> **jelly 智能家居传感器 — 电量采集嵌入式固件工程**

***

## 目录

- [代码目录层级](#代码目录层级)
- [工程概述](#工程概述)
- [硬件平台](#硬件平台)
- [软件架构](#软件架构)
- [已实现模块](#已实现模块)
- [编译与烧录](#编译与烧录)
- [调试方法](#调试方法)
- [编码规范](#编码规范)

***

## 代码目录层级

```
PowerAcquisition_project/
│
├── .cmsis/                              # CMSIS 核心标准文件（ARM 提供）
│   └── include/                         #   CMSIS-Core 头文件集合
│       ├── core_cm3.h                   #     Cortex-M3 内核寄存器定义
│       ├── cmsis_compiler.h             #     编译器适配层
│       ├── cmsis_armcc.h                #     ARMCC 编译器扩展
│       └── ...                          #     其余 Cortex-M 系列头文件
│
├── .eide/                               # EIDE (Embedded IDE) VSCode 插件配置
│   ├── eide.yml                         #   项目定义（虚拟文件夹/源码映射）
│   ├── env.ini                          #   环境变量
│   └── files.options.yml                #   文件级编译选项
│
├── .vscode/                             # VSCode 工作区配置
│   ├── tasks.json                       #   构建任务定义
│   └── c_cpp_properties.json            #   IntelliSense 配置
│
├── start/                               # ★ 启动层 (CMSIS + System)
│   ├── startup_stm32f10x_md.s           #   汇编启动文件（中等容量 MD）
│   ├── core_cm3.c / .h                  #   Cortex-M3 内核访问层
│   ├── system_stm32f10x.c / .h          #   系统时钟初始化 (SystemInit)
│   └── stm32f10x.h                      #   芯片外设寄存器/中断号定义
│
├── library/                             # ★ 驱动层 (STM32 标准外设库 V3.6.x)
│   ├── misc.c / .h                      #   NVIC 中断管理 / SysTick 配置
│   ├── stm32f10x_rcc.c / .h             #   时钟复位控制 (RCC)
│   ├── stm32f10x_gpio.c / .h            #   通用输入输出 (GPIO)
│   ├── stm32f10x_usart.c / .h           #   通用同步异步收发器 (USART)
│   ├── stm32f10x_adc.c / .h             #   模数转换器 (ADC)
│   ├── stm32f10x_dma.c / .h             #   直接存储器访问 (DMA)
│   ├── stm32f10x_tim.c / .h             #   定时器 (TIM1~TIM4)
│   ├── stm32f10x_spi.c / .h             #   串行外设接口 (SPI)
│   ├── stm32f10x_i2c.c / .h             #   I2C 总线
│   ├── stm32f10x_can.c / .h             #   CAN 总线
│   ├── stm32f10x_exti.c / .h            #   外部中断 (EXTI)
│   ├── stm32f10x_flash.c / .h           #   片内 Flash 操作
│   ├── stm32f10x_pwr.c / .h             #   电源管理 (PWR)
│   ├── stm32f10x_rtc.c / .h             #   实时时钟 (RTC)
│   ├── stm32f10x_bkp.c / .h             #   备份寄存器 (BKP)
│   ├── stm32f10x_iwdg.c / .h            #   独立看门狗 (IWDG)
│   ├── stm32f10x_wwdg.c / .h            #   窗口看门狗 (WWDG)
│   ├── stm32f10x_crc.c / .h             #   CRC 校验
│   ├── stm32f10x_dac.c / .h             #   数模转换器 (DAC)
│   ├── stm32f10x_dbgmcu.c / .h          #   调试支持 (DBGMCU)
│   ├── stm32f10x_fsmc.c / .h            #   可变静态存储控制器 (FSMC)
│   └── stm32f10x_sdio.c / .h            #   SDIO 接口
│
├── model/                               # ★ 业务模块层 (外设驱动封装)
│   ├── uart/                            #   串口驱动（USART1 / USART2 / USART3）
│   │   ├── uart.h                       #     串口驱动头文件（含 uart_port_t 枚举）
│   │   └── uart.c                       #     串口驱动源文件（初始化/收发/ISR）
│   └── delay/                           #   延时驱动（us / ms / s 三级延时）
│       ├── delay.h                      #     延时模块头文件
│       └── delay.c                      #     延时模块源文件（DWT + SysTick 实现）
│
├── user/                                # ★ 应用层 (用户代码)
│   ├── main.c                           #   主函数入口（定时发送电量采集指令）
│   ├── stm32f10x_it.c                   #   中断服务函数集合
│   ├── stm32f10x_it.h                   #   中断函数声明
│   └── stm32f10x_conf.h                 #   外设库裁剪开关
│
├── Objects/                             # 编译产物 (自动生成)
│   ├── project.axf                      #   可执行文件 (带调试信息)
│   ├── project.hex                      #   HEX 烧录文件
│   ├── project.sct                      #   链接脚本 (散列加载描述)
│   ├── project.lnp                      #   链接器输入文件
│   ├── project.map                      #   内存映射文件
│   └── *.o / *.crf / *.d               #   各模块目标文件
│
├── Listings/                            # 编译列表文件 (自动生成)
│   ├── project.map                      #   完整内存映射
│   └── startup_stm32f10x_md.lst         #   启动文件反汇编列表
│
├── DebugConfig/                         # 调试器配置
│   └── Target_1_STM32F103C8.dbgconf     #   DBGMCU 寄存器调试配置
│
├── project.uvprojx                      # Keil MDK 工程文件
├── project.uvoptx                       # Keil 工程选项
├── project.uvguix.14689                 # Keil 用户布局
├── project.code-workspace               # VSCode 多根工作区
├── .clang-format                        # C 代码格式化规则
├── .gitignore                           # Git 忽略规则
│
└── README.md                            # 本文档
```

***

## 工程概述

| 项目        | 内容                                    |
| --------- | ------------------------------------- |
| **产品线**   | jelly 智能家居传感器                         |
| **工程名称**  | PowerAcquisition（电量采集）                |
| **目标芯片**  | STM32F103C8T6                         |
| **内核**    | ARM Cortex-M3 @ 72MHz                 |
| **Flash** | 64 KB（0x08000000 \~ 0x0800FFFF）       |
| **RAM**   | 20 KB（0x20000000 \~ 0x20004FFF）       |
| **固件库**   | STM32 标准外设库 (StdPeriph Driver) V3.6.x |
| **编译器**   | ARMCC V5.06 (ARM Compiler 5)          |
| **IDE**   | Keil MDK + VSCode (EIDE 插件)           |
| **调试器**   | J-Link / ST-Link，支持 SWD 接口            |
| **预定义宏**  | `USE_STDPERIPH_DRIVER` `STM32F10X_MD` |

***

## 硬件平台

### 晶振配置

| 晶振      | 频率         | 用途                            |
| ------- | ---------- | ----------------------------- |
| **HSE** | 8 MHz      | 主时钟源，经 PLL ×9 得到 72MHz SYSCLK |
| **LSE** | 32.768 kHz | 预留 RTC 实时时钟（暂未使用）             |

### 引脚资源

| 外设         | 引脚   | 功能     | 时钟总线         | 备注        |
| ---------- | ---- | ------ | ------------ | --------- |
| USART1\_TX | PA9  | 串口1 发送 | APB2 (72MHz) | 复用推挽输出    |
| USART1\_RX | PA10 | 串口1 接收 | APB2 (72MHz) | 浮空输入，中断接收 |
| USART2\_TX | PA2  | 串口2 发送 | APB1 (36MHz) | 复用推挽输出    |
| USART2\_RX | PA3  | 串口2 接收 | APB1 (36MHz) | 浮空输入，中断接收 |
| USART3\_TX | PB10 | 串口3 发送 | APB1 (36MHz) | 复用推挽输出    |
| USART3\_RX | PB11 | 串口3 接收 | APB1 (36MHz) | 浮空输入，中断接收 |

### 时钟树

```
HSE (8MHz) ──→ PLL (×9) ──→ SYSCLK (72MHz)
                               ├── HCLK  (72MHz) ──→ Cortex-M3 内核 / DWT / SysTick
                               ├── PCLK2 (72MHz) ──→ APB2 (USART1 / GPIOA~E / SPI1 / ADC1)
                               └── PCLK1 (36MHz) ──→ APB1 (USART2~3 / I2C / SPI2 / TIM2~4)
```

***

## 软件架构

采用经典的 **裸机分层架构**，各层职责分离、单向依赖：

```
┌──────────────────────────────────────────────────┐
│                    user/                          │  ← 应用层
│          main.c  /  stm32f10x_it.c               │    业务逻辑、状态机、任务调度
├──────────────────────────────────────────────────┤
│                    model/                         │  ← 业务模块层
│         uart / delay / (后续扩展...)             │    外设驱动封装、协议解析
├──────────────────────────────────────────────────┤
│                   library/                        │  ← 标准外设库层
│    stm32f10x_usart / gpio / tim / adc / ...     │    STM32 官方 V3.6 标准外设驱动
├──────────────────────────────────────────────────┤
│                    start/                         │  ← 启动层
│  startup / core_cm3 / system_stm32f10x          │    中断向量表、系统时钟、CMSIS Core
└──────────────────────────────────────────────────┘
```

### 依赖规则

- `user`    → 可引用 `model` / `library` / `start`
- `model`   → 可引用 `library` / `start`
- `library` → 可引用 `start`
- `start`   → 不依赖其他层

***

## 已实现模块

### 1. USART 串口驱动 (`model/uart/`)

支持 **三个独立串口**：USART1（PA9/PA10）、USART2（PA2/PA3）、USART3（PB10/PB11），全部使用相同的中断收发机制。

#### 特性

| 项目    | 配置                        |
| ----- | ------------------------- |
| 波特率   | 9600 bps                  |
| 数据位   | 8                         |
| 停止位   | 1                         |
| 校验位   | 无                         |
| 硬件流控  | 无                         |
| TX 缓冲 | 环形缓冲区，128 字节              |
| RX 缓冲 | 环形缓冲区，256 字节              |
| 接收方式  | RXNE 中断逐字节接收 + IDLE 中断帧检测 |
| 发送方式  | TXE 中断连续发送，非阻塞            |

#### 数据结构

```c
typedef struct {
    uint8_t  tx_buf[UART_TX_BUF_SIZE];    /* 发送环形缓冲区 */
    volatile uint16_t tx_head;            /* 头指针（ISR 读取位置） */
    volatile uint16_t tx_tail;            /* 尾指针（用户写入位置） */
    volatile uint8_t  tx_busy;            /* 发送忙标志 */

    uint8_t  rx_buf[UART_RX_BUF_SIZE];    /* 接收环形缓冲区 */
    volatile uint16_t rx_head;            /* 头指针（用户读取位置） */
    volatile uint16_t rx_tail;            /* 尾指针（ISR 写入位置） */
    volatile uint8_t  rx_complete;        /* 帧接收完成标志 */
    volatile uint16_t rx_frame_len;       /* 当前帧字节数 */
} uart_dev_t;
```

#### 串口选择枚举

```c
typedef enum {
    uart1 = 1,    /* USART1: TX=PA9,  RX=PA10 */
    uart2 = 2,    /* USART2: TX=PA2,  RX=PA3  */
    uart3 = 3     /* USART3: TX=PB10, RX=PB11 */
} uart_port_t;
```

#### API

| 函数     | USART1                  | USART2                  | USART3                  | 功能                        |
| ------ | ----------------------- | ----------------------- | ----------------------- | ------------------------- |
| 初始化    | `uart1_init()`          | `uart2_init()`          | `uart3_init()`          | 初始化串口（9600-8-N-1，TX+中断RX） |
| 发送字符串  | `uart1_send_string()`   | `uart2_send_string()`   | `uart3_send_string()`   | 发送以 \0 结尾的字符串             |
| 发送十六进制 | `uart1_send_hex()`      | `uart2_send_hex()`      | `uart3_send_hex()`      | 发送任意二进制数据                 |
| 读取接收   | `uart1_recv_data()`     | `uart2_recv_data()`     | `uart3_recv_data()`     | 从接收缓冲区取出数据                |
| 清除标志   | `uart1_clear_rx_flag()` | `uart2_clear_rx_flag()` | `uart3_clear_rx_flag()` | 清除帧完成标志                   |

**统一发送接口**（通过枚举选择串口）：

```c
uint8_t Serial_transmission_Selection(uart_port_t port, const uint8_t *data, uint16_t len);
```

| 参数      | 说明                               |
| ------- | -------------------------------- |
| `port`  | 串口选择：`uart1` / `uart2` / `uart3` |
| `data`  | 待发送数据指针                          |
| `len`   | 数据长度（字节）                         |
| **返回值** | `0` = 成功，`1` = 参数错误              |

#### 全局设备实例

```c
extern uart_dev_t g_uart1_dev;    /* USART1 状态 */
extern uart_dev_t g_uart2_dev;    /* USART2 状态 */
extern uart_dev_t g_uart3_dev;    /* USART3 状态 */
```

#### 中断处理流程

```
数据到达 RX ──→ RXNE ISR 逐字节写入 rx_buf (tail++)
                    │
         ┌──────────┴──────────┐
         │  持续收到数据        │
         └──────────┬──────────┘
                    │
总线空闲（一个字节时间内无数据）──→ IDLE ISR
                    │
         计算 rx_frame_len
         置 rx_complete = 1
                    │
         用户层 main 循环检测 rx_complete
         调用 uartX_recv_data() 取出数据
         调用 uartX_clear_rx_flag() 清除标志
```

#### 内部实现

提取了两个 `static` 参数化内部函数，三个串口复用代码：

```c
static void _uart_tx_write(USART_TypeDef *usart, uart_dev_t *dev, const uint8_t *data, uint16_t len);
static void _uart_isr_handler(USART_TypeDef *usart, uart_dev_t *dev);
```

三个 ISR 只做参数分发：

```c
void USART1_IRQHandler(void) { _uart_isr_handler(USART1, &g_uart1_dev); }
void USART2_IRQHandler(void) { _uart_isr_handler(USART2, &g_uart2_dev); }
void USART3_IRQHandler(void) { _uart_isr_handler(USART3, &g_uart3_dev); }
```

***

### 2. 延时模块 (`model/delay/`)

#### 特性

| 延时级别   | 实现机制                       | 精度    | 最大范围     |
| ------ | -------------------------- | ----- | -------- |
| **us** | DWT 周期计数器 (CYCCNT @ 72MHz) | ±1 us | \~59.6 s |
| **ms** | SysTick 1ms 中断累加计数         | ±1 ms | \~49.7 天 |
| **s**  | delay\_ms(1000) 循环         | ±1 ms | 无上限      |

#### API

```c
void delay_init(void);        /* 初始化：SysTick 1ms + DWT CYCCNT */
void delay_us(uint32_t us);   /* 微秒级阻塞延时 */
void delay_ms(uint32_t ms);   /* 毫秒级阻塞延时 */
void delay_s(uint32_t s);     /* 秒级阻塞延时 */
```

#### DWT 使用说明

DWT (Data Watchpoint and Trace) 是 Cortex-M3 内核内置硬件计数器，**无需占用任何外设定时器**。`delay_us()` 利用 `DWT->CYCCNT` 寄存器在 72MHz 下以 **1 时钟周期 = 13.89ns** 的精度做微秒等待：

```
72MHz 下：delay_us(N) = N × 72 个时钟周期的忙等待
```

***

### 3. 当前 main.c 运行逻辑

```c
int main(void)
{
    uint8_t power_cmd[] = {0x83, 0x06, 0x00, 0x80, 0x01, 0x55, 0x57, 0xAF};

    delay_init();                          /* SysTick 1ms + DWT us */
    uart1_init();                          /* USART1: 9600-8-N-1 */

    uart1_send_string("PowerAcquisition System Ready\r\n");

    while (1) {
        uart1_send_hex(power_cmd, sizeof(power_cmd));     /* 发送电量采集指令 */
        delay_ms(10000);                                   /* 间隔 10s */
    }
}
```

***

## 编译与烧录

### Keil MDK 编译

1. 打开 `project.uvprojx`
2. 选择 Target: `Target 1`
3. 点击 **Build (F7)** 编译
4. 产物目录：`Objects/project.hex` `Objects/project.axf`

### VSCode + EIDE 编译

1. 安装 VSCode 插件 `cl.eide`
2. 打开工作区 `project.code-workspace`
3. 点击 EIDE 面板中的 **Build**

### 烧录

- **J-Link / ST-Link**：Keil 中 `Flash → Download (F8)` 直接下载
- **第三方工具**：使用 `Objects/project.hex` 配合 STM32CubeProgrammer / J-Flash

***

## 调试方法

### 1. 硬件断点调试

- 使用 **J-Link / ST-Link** 连接 SWDIO / SWCLK / GND
- 在 Keil 中进入 **Debug Session (Ctrl+F5)**
- 设置断点，查看变量，单步执行

### 2. HardFault 排查

当程序进入 HardFault\_Handler 时：

```
1. 在 HardFault_Handler 中设断点
2. 查看 SCB 寄存器:
   - CFSR  (Configurable Fault Status Register) → 区分 MMFAR/BFAR/UFSR
   - HFSR  (HardFault Status Register)          → 确认是否 FORCED
   - MMFAR / BFAR                               → 定位非法地址
3. 通过 Call Stack 窗口回溯调用链
```

### 3. 逻辑分析仪 / 示波器

| 测量点               | 信号         | 验证内容                   |
| ----------------- | ---------- | ---------------------- |
| PA9 (USART1\_TX)  | 9600bps 串口 | 验证波特率（每位约 104us）和数据帧格式 |
| PA10 (USART1\_RX) | 接收数据       | 确认外部设备发送时序             |

### 4. 串口助手验证

- 使用 USB-TTL 模块连接 PA9(TX) / PA10(RX) / GND
- 波特率 9600, 8-N-1，观察发送的十六进制指令 `83 06 00 80 01 55 57 AF`

***

## 编码规范

### 命名规则

| 类别   | 风格                     | 示例                                    |
| ---- | ---------------------- | ------------------------------------- |
| 函数   | `snake_case`           | `uart1_init()`, `uart1_send_string()` |
| 变量   | `snake_case`           | `rx_complete`, `tx_busy`              |
| 全局变量 | `g_` 前缀 + `snake_case` | `g_uart1_dev`, `g_uart2_dev`          |
| 宏/常量 | `UPPER_CASE`           | `UART_TX_BUF_SIZE`                    |
| 结构体  | `snake_case_t`         | `uart_dev_t`                          |
| 枚举   | `snake_case_t`         | `uart_port_t`                         |
| 枚举成员 | `snake_case`           | `uart1`, `uart2`, `uart3`             |

### 通用原则

- 使用 `stdint.h` 固定宽度类型：`uint8_t` `uint16_t` `uint32_t`
- ISR 与用户代码共享变量必须加 `volatile` 修饰
- ISR 保持简短，只置标志位，在 main 循环中处理业务逻辑
- 禁止动态内存分配（`malloc` / `free`），仅使用静态数组和环形缓冲
- 所有整数常量加 `u` 后缀明确无符号类型：`1u` `0u` `256u`
- 空指针入参必须校验防御

***

*文档最后更新：2026-05-08*

<br />

<br />

<br />

<br />

<br />

<br />

STM32网关温湿度消息发布,发布主题：/Multisensor/{devcd}/data
```json
{
  "devcd":"STM32_485Gateway",
  "devtype":"temp_humi_sensor",
  "nodeid":"thsensor_1",
  "data":{
    "temp":26.67,
    "humi":65.60
  }
}
```

STM32网关电力消息发布,发布主题：/Multisensor/{devcd}/data
```json
{
  "devcd":"STM32_485Gateway",
  "devtype":"electricity_sensor",
  "nodeid":"elsensor_33",
  "data":{
    "sw":1,              // 开关状态
    "current":10.00,     // 电流
    "voltage":230.00,    // 电压
    "power":1000.00,     // 功率
    "temp":26.67,        // 断路器内部温度
    "factor":90,         // 功率因素
    "energy":1000000.00  // 电能
  }
}
```

STM32网关烟雾传感器消息发布,发布主题：/Multisensor/{devcd}/data
```json
{
  "devcd":"STM32_485Gateway",
  "devtype":"smoke_sensor",
  "nodeid":"smsensor_1",
  "data":{
    "smoke":100,
    "temp":26.67,
    "humi":65.60
  }
}
```



<br />

<br />

<br />

