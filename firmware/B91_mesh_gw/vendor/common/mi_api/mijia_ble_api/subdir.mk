################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../vendor/common/mi_api/mijia_ble_api/mible_api.c \
../vendor/common/mi_api/mijia_ble_api/mible_mcu.c \
../vendor/common/mi_api/mijia_ble_api/mible_mesh_api.c 

OBJS += \
./vendor/common/mi_api/mijia_ble_api/mible_api.o \
./vendor/common/mi_api/mijia_ble_api/mible_mcu.o \
./vendor/common/mi_api/mijia_ble_api/mible_mesh_api.o 

C_DEPS += \
./vendor/common/mi_api/mijia_ble_api/mible_api.d \
./vendor/common/mi_api/mijia_ble_api/mible_mcu.d \
./vendor/common/mi_api/mijia_ble_api/mible_mesh_api.d 


# Each subdirectory must supply rules for building sources it contributes
vendor/common/mi_api/mijia_ble_api/%.o: ../vendor/common/mi_api/mijia_ble_api/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -D__TLSR_RISCV_EN__=1 -DCHIP_TYPE=CHIP_TYPE_9518 -D__PROJECT_MESH_PRO__=1 -I"/cygdrive/E/TUONG/TELINK/Telink_code_9xxx/GateWay/firmware" -I../drivers/B91 -I../vendor/Common -I../common -I"/cygdrive/E/TUONG/TELINK/Telink_code_9xxx/GateWay/firmware/vendor/common/mi_api/libs" -I"/cygdrive/E/TUONG/TELINK/Telink_code_9xxx/GateWay/firmware/vendor/common/mi_api/mijia_ble_api" -O2 -flto -g3 -Wall -mcpu=d25f -ffunction-sections -fdata-sections -mext-dsp -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fpack-struct -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


