################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/Third_Party/FreeRTOS_Plus_TCP/portable/BufferManagement/BufferAllocation_2.c 

OBJS += \
./Middlewares/Third_Party/FreeRTOS_Plus_TCP/portable/BufferManagement/BufferAllocation_2.o 

C_DEPS += \
./Middlewares/Third_Party/FreeRTOS_Plus_TCP/portable/BufferManagement/BufferAllocation_2.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Third_Party/FreeRTOS_Plus_TCP/portable/BufferManagement/%.o Middlewares/Third_Party/FreeRTOS_Plus_TCP/portable/BufferManagement/%.su Middlewares/Third_Party/FreeRTOS_Plus_TCP/portable/BufferManagement/%.cyclo: ../Middlewares/Third_Party/FreeRTOS_Plus_TCP/portable/BufferManagement/%.c Middlewares/Third_Party/FreeRTOS_Plus_TCP/portable/BufferManagement/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F429xx -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Drivers/enc28j60 -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/Third_Party/FreeRTOS_Plus_TCP/include -I../Middlewares/Third_Party/FreeRTOS_Plus_TCP/portable/Compiler/GCC -Og -ffunction-sections -fdata-sections -Wall -Wno-unused-variable -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-Third_Party-2f-FreeRTOS_Plus_TCP-2f-portable-2f-BufferManagement

clean-Middlewares-2f-Third_Party-2f-FreeRTOS_Plus_TCP-2f-portable-2f-BufferManagement:
	-$(RM) ./Middlewares/Third_Party/FreeRTOS_Plus_TCP/portable/BufferManagement/BufferAllocation_2.cyclo ./Middlewares/Third_Party/FreeRTOS_Plus_TCP/portable/BufferManagement/BufferAllocation_2.d ./Middlewares/Third_Party/FreeRTOS_Plus_TCP/portable/BufferManagement/BufferAllocation_2.o ./Middlewares/Third_Party/FreeRTOS_Plus_TCP/portable/BufferManagement/BufferAllocation_2.su

.PHONY: clean-Middlewares-2f-Third_Party-2f-FreeRTOS_Plus_TCP-2f-portable-2f-BufferManagement

