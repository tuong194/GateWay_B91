################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../vendor/mesh_provision/app.c \
../vendor/mesh_provision/app_att.c \
../vendor/mesh_provision/app_buffer.c \
../vendor/mesh_provision/main.c 

OBJS += \
./vendor/mesh_provision/app.o \
./vendor/mesh_provision/app_att.o \
./vendor/mesh_provision/app_buffer.o \
./vendor/mesh_provision/main.o 

C_DEPS += \
./vendor/mesh_provision/app.d \
./vendor/mesh_provision/app_att.d \
./vendor/mesh_provision/app_buffer.d \
./vendor/mesh_provision/main.d 


# Each subdirectory must supply rules for building sources it contributes
vendor/mesh_provision/%.o: ../vendor/mesh_provision/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -D__TLSR_RISCV_EN__=1 -DCHIP_TYPE=CHIP_TYPE_9518 -D__PROJECT_MESH_PRO__=1 -I"/cygdrive/E/TUONG/TELINK/Telink_code_9xxx/GateWay/firmware" -I../drivers/B91 -I../vendor/Common -I../common -I"/cygdrive/E/TUONG/TELINK/Telink_code_9xxx/GateWay/firmware/vendor/common/mi_api/libs" -I"/cygdrive/E/TUONG/TELINK/Telink_code_9xxx/GateWay/firmware/vendor/common/mi_api/mijia_ble_api" -O2 -flto -g3 -Wall -mcpu=d25f -ffunction-sections -fdata-sections -mext-dsp -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fpack-struct -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


