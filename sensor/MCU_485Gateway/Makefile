######################################
# 目标
######################################
TARGET = MCU_485Gateway

######################################
# 构建目标的目录
######################################
BUILD_DIR = build_gcc

######################################
# 工具链
######################################
PREFIX = arm-none-eabi-
ifdef GCC_PATH
CC  = $(GCC_PATH)/$(PREFIX)gcc
AS  = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP  = $(GCC_PATH)/$(PREFIX)objcopy
SZ  = $(GCC_PATH)/$(PREFIX)size
else
CC  = $(PREFIX)gcc
AS  = $(PREFIX)gcc -x assembler-with-cpp
CP  = $(PREFIX)objcopy
SZ  = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

######################################
# C 源文件
######################################
# --- 应用层 ---
C_SOURCES =  \
user/main.c \
user/stm32f10x_it.c

# --- CMSIS / 启动 ---
C_SOURCES += \
start/core_cm3.c \
start/system_stm32f10x.c

# --- 业务驱动层 (model/) ---
C_SOURCES += \
model/CRC/crc.c \
model/USR_K5/Net_USRK5.c \
model/delay/delay.c \
model/led/led.c \
model/modbus/modbus.c \
model/mqtt/mqtt.c \
model/uart/uart.c \
model/timer/timer.c

# 注意：目录名含括号，需要用变量绕过 Make 解析
MAX3485_DIR = model/max3485esa(umw)
C_SOURCES += $(MAX3485_DIR)/max3485.c

# --- MQTT 协议栈 (mqtt_lib/) ---
C_SOURCES += \
mqtt_lib/MQTTConnectClient.c \
mqtt_lib/MQTTConnectServer.c \
mqtt_lib/MQTTDeserializePublish.c \
mqtt_lib/MQTTFormat.c \
mqtt_lib/MQTTPacket.c \
mqtt_lib/MQTTSerializePublish.c \
mqtt_lib/MQTTSubscribeClient.c \
mqtt_lib/MQTTSubscribeServer.c \
mqtt_lib/MQTTUnsubscribeClient.c \
mqtt_lib/MQTTUnsubscribeServer.c

# --- STM32 标准外设库 (library/) ---
C_SOURCES += \
library/misc.c \
library/stm32f10x_adc.c \
library/stm32f10x_bkp.c \
library/stm32f10x_can.c \
library/stm32f10x_cec.c \
library/stm32f10x_crc.c \
library/stm32f10x_dac.c \
library/stm32f10x_dbgmcu.c \
library/stm32f10x_dma.c \
library/stm32f10x_exti.c \
library/stm32f10x_flash.c \
library/stm32f10x_fsmc.c \
library/stm32f10x_gpio.c \
library/stm32f10x_i2c.c \
library/stm32f10x_iwdg.c \
library/stm32f10x_pwr.c \
library/stm32f10x_rcc.c \
library/stm32f10x_rtc.c \
library/stm32f10x_sdio.c \
library/stm32f10x_spi.c \
library/stm32f10x_tim.c \
library/stm32f10x_usart.c \
library/stm32f10x_wwdg.c

######################################
# 汇编源文件
######################################
ASM_SOURCES = \
start/startup_stm32f10x_hd_gcc.s

######################################
# 头文件搜索路径
######################################
C_INCLUDES =  \
-Istart \
-Iuser \
-Imodel \
-Imodel/CRC \
-Imodel/USR_K5 \
-Imodel/delay \
-Imodel/led \
-I"model/max3485esa(umw)" \
-Imodel/modbus \
-Imodel/mqtt \
-Imodel/uart \
-Imqtt_lib \
-Ilibrary

######################################
# 预处理宏定义
######################################
C_DEFS =  \
-DUSE_STDPERIPH_DRIVER \
-DSTM32F10X_HD

######################################
# 编译参数
######################################
# MCU 标志
CPU = -mcpu=cortex-m3
FPU =
FLOAT-ABI =
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# C 编译参数
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) -Wall -fdata-sections -ffunction-sections
ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif
# 优化等级
OPT = -Og
CFLAGS += $(OPT)
# 依赖文件生成
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

# 汇编参数
ASFLAGS = $(MCU) -Wall -fdata-sections -ffunction-sections

######################################
# 链接参数
######################################
LDSCRIPT = STM32F103C8Tx_FLASH.ld
LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) \
          -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

######################################
# 构建规则
######################################
# 默认目标
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

# 列出所有 C 源文件的 .o 目标 (max3485 有单独规则，需排除)
C_SOURCES_FILTERED = $(filter-out $(MAX3485_DIR)/%,$(C_SOURCES))
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES_FILTERED:.c=.o)))
OBJECTS += $(BUILD_DIR)/max3485.o
vpath %.c $(sort $(dir $(C_SOURCES_FILTERED)))

# 列出所有汇编源文件的 .o 目标
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/max3485.o: $(MAX3485_DIR)/max3485.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/max3485.lst "$<" -o "$@"

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir -p $@

######################################
# 清理
######################################
clean:
	-rm -fR $(BUILD_DIR)

######################################
# 烧录 (OpenOCD + ST-Link, 自动重试)
######################################
flash: $(BUILD_DIR)/$(TARGET).bin
	@for i in 1 2 3 4 5; do \
		echo "Flash attempt $$i/5..."; \
		openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
			-c "adapter speed 400" \
			-c "program $< 0x08000000 verify reset exit" && echo "Flash SUCCESS" && exit 0; \
		echo "Attempt $$i failed, retrying..."; \
		sleep 1; \
	done; \
	echo "Flash FAILED after 5 attempts"; exit 1

######################################
# 依赖文件
######################################
-include $(wildcard $(BUILD_DIR)/*.d)
