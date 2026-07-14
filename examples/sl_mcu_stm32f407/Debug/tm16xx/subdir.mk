################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../tm16xx/tm1638.c 

OBJS += \
./tm16xx/tm1638.o 

C_DEPS += \
./tm16xx/tm1638.d 


# Each subdirectory must supply rules for building sources it contributes
tm16xx/%.o tm16xx/%.su tm16xx/%.cyclo: ../tm16xx/%.c tm16xx/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"/home/lth/src/stm32-tm16xx/examples/sl_mcu_stm32f407/tm16xx" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-tm16xx

clean-tm16xx:
	-$(RM) ./tm16xx/tm1638.cyclo ./tm16xx/tm1638.d ./tm16xx/tm1638.o ./tm16xx/tm1638.su

.PHONY: clean-tm16xx

