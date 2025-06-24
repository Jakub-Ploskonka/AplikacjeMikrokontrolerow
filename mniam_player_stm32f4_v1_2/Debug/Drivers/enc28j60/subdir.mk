################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/enc28j60/enc28j60_eth_phy.c \
../Drivers/enc28j60/enc28j60_net_interface.c 

OBJS += \
./Drivers/enc28j60/enc28j60_eth_phy.o \
./Drivers/enc28j60/enc28j60_net_interface.o 

C_DEPS += \
./Drivers/enc28j60/enc28j60_eth_phy.d \
./Drivers/enc28j60/enc28j60_net_interface.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/enc28j60/%.o Drivers/enc28j60/%.su Drivers/enc28j60/%.cyclo: ../Drivers/enc28j60/%.c Drivers/enc28j60/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F429xx -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Drivers/enc28j60 -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/Third_Party/FreeRTOS_Plus_TCP/include -I../Middlewares/Third_Party/FreeRTOS_Plus_TCP/portable/Compiler/GCC -Og -ffunction-sections -fdata-sections -Wall -Wno-unused-variable -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-enc28j60

clean-Drivers-2f-enc28j60:
	-$(RM) ./Drivers/enc28j60/enc28j60_eth_phy.cyclo ./Drivers/enc28j60/enc28j60_eth_phy.d ./Drivers/enc28j60/enc28j60_eth_phy.o ./Drivers/enc28j60/enc28j60_eth_phy.su ./Drivers/enc28j60/enc28j60_net_interface.cyclo ./Drivers/enc28j60/enc28j60_net_interface.d ./Drivers/enc28j60/enc28j60_net_interface.o ./Drivers/enc28j60/enc28j60_net_interface.su

.PHONY: clean-Drivers-2f-enc28j60

