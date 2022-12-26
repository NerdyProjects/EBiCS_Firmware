
# Compilation tools
CC := arm-none-eabi-gcc
ARMAR :=  arm-none-eabi-ar
RM := rm -rf

# Compilation flags for Cortex-M3
CFLAGS := -mcpu=cortex-m3 \
 -mthumb \
 -Wsign-compare \
 -Wdouble-promotion \
 -Os \
 -g3 \
 -DNDEBUG \
 -Wall -Wextra \
 -mfloat-abi=soft \
 -ffunction-sections \
 -fdata-sections \
 '-D__weak=__attribute__((weak))' \
 '-D__packed=__attribute__((__packed__))' \
 -DUSE_HAL_DRIVER \
 -DSTM32F103x6

 CMSIS_DSP := Drivers/CMSIS_DSP

CMSIS_CORE_INCLUDES := Drivers/CMSIS/Core/Include 

# Sources
SRCS := $(CMSIS_DSP)/Source/BasicMathFunctions/BasicMathFunctions.c \
 $(CMSIS_DSP)/Source/CommonTables/CommonTables.c \
 $(CMSIS_DSP)/Source/InterpolationFunctions/InterpolationFunctions.c \
 $(CMSIS_DSP)/Source/BayesFunctions/BayesFunctions.c \
 $(CMSIS_DSP)/Source/MatrixFunctions/MatrixFunctions.c \
 $(CMSIS_DSP)/Source/ComplexMathFunctions/ComplexMathFunctions.c \
 $(CMSIS_DSP)/Source/QuaternionMathFunctions/QuaternionMathFunctions.c \
 $(CMSIS_DSP)/Source/ControllerFunctions/ControllerFunctions.c \
 $(CMSIS_DSP)/Source/SVMFunctions/SVMFunctions.c \
 $(CMSIS_DSP)/Source/DistanceFunctions/DistanceFunctions.c \
 $(CMSIS_DSP)/Source/StatisticsFunctions/StatisticsFunctions.c \
 $(CMSIS_DSP)/Source/FastMathFunctions/FastMathFunctions.c \
 $(CMSIS_DSP)/Source/SupportFunctions/SupportFunctions.c \
 $(CMSIS_DSP)/Source/FilteringFunctions/FilteringFunctions.c \
 $(CMSIS_DSP)/Source/TransformFunctions/TransformFunctions.c \
 $(CMSIS_DSP)/Source/WindowFunctions/WindowFunctions.c \
 Src/FOC.c \
 Src/display_bafang.c \
 Src/display_kingmeter.c \
 Src/display_kunteng.c \
 Src/eeprom.c \
 Src/main.c \
 Src/print.c \
 Src/stm32f1xx_hal_msp.c \
 Src/stm32f1xx_it.c \
 Src/system_stm32f1xx_Bootloader.c \
 Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c \
 Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_adc.c \
 Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_adc_ex.c \
 Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c \
 Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c \
 Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c \
 Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c \
 Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c \
 Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c \
 Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c \
 Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c \
 Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c \
 Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim.c \
 Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim_ex.c \
 Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c 

S_SRCS = \
Startup/startup_stm32f103x6.s 

# Each subdirectory must supply rules for building sources it contributes 
build/%.o: Startup/%.s
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Assembler'
	@echo $(PWD)
	arm-none-eabi-as -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

# Includes
DSP_INCLUDES := $(CMSIS_DSP)/Include \
  $(CMSIS_DSP)/PrivateInclude 

# Compilation flags for include folders
INC_FLAGS := $(addprefix -I,$(DSP_INCLUDES))
INC_FLAGS += $(addprefix -I,$(CMSIS_CORE_INCLUDES))
INC_FLAGS += $(addprefix -I,Drivers/CMSIS/Device/ST/STM32F1xx/Include)
INC_FLAGS += $(addprefix -I,Drivers/STM32F1xx_HAL_Driver/Inc)
INC_FLAGS += $(addprefix -I,Inc)
CFLAGS += $(INC_FLAGS)

# Output folder for build products
BUILDDIR := ./build

OBJECTS := $(SRCS:%=$(BUILDDIR)/%.o)

OBJECTS += \
build/startup_stm32f103x6.o 
	

$(BUILDDIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

# All Target
all: LishuiFOC_01.elf

# Tool invocations
LishuiFOC_01.elf: $(OBJECTS) STM32F103C6Tx_FLASH_Bootloader.ld
	@echo 'Building target: $@'
	@echo 'Invoking: MCU GCC Linker'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -L $(SRC_PATH) -specs=nosys.specs -specs=nano.specs -T"STM32F103C6Tx_FLASH_Bootloader.ld" -Wl,-Map=output.map -Wl,--gc-sections -o "build/EBiCS_Firmware.elf" $(OBJECTS) -lm
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(BUILDDIR)
	-@echo ' '

post-build:
	-@echo 'Generating hex and Printing size information:'
	arm-none-eabi-objcopy -O ihex "build/EBiCS_Firmware.elf" "build/EBiCS_Firmware.hex"
	arm-none-eabi-objcopy -O binary "build/EBiCS_Firmware.elf" "build/EBiCS_Firmware.bin"
	arm-none-eabi-size "build/EBiCS_Firmware.elf"
	stat "build/EBiCS_Firmware.bin"
	-@echo ' '


.PHONY: all clean dependents
.SECONDARY: post-build

