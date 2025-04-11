################################################################################
# Automatically-generated file. Do not edit!
################################################################################

ifndef ANDESIGHT_ROOT
ANDESIGHT_ROOT=/cygdrive/C/Andestech/AndeSight_RDS_v323
endif

ifndef CROSS_COMPILE
CROSS_COMPILE=riscv32-elf-
endif

ifndef SECONDARY_OUTPUT_PATH
SECONDARY_OUTPUT_PATH=output
endif

$(shell mkdir -p $(SECONDARY_OUTPUT_PATH))

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include 3rd-party/freertos-V5/subdir.mk
-include 3rd-party/freertos-V5/portable/GCC/RISC-V/subdir.mk
-include 3rd-party/freertos-V5/portable/MemMang/subdir.mk
-include application/app/subdir.mk
-include application/audio/subdir.mk
-include application/keyboard/subdir.mk
-include application/print/subdir.mk
-include application/usbstd/subdir.mk
-include boot/B91/subdir.mk
-include common/subdir.mk
-include common/usb_dbg/subdir.mk
-include drivers/B91/subdir.mk
-include drivers/B91/ext_driver/subdir.mk
-include drivers/B91/flash/subdir.mk
-include proj/common/subdir.mk
-include vendor/common/subdir.mk
-include vendor/common/chip_adapt_layer/subdir.mk
-include vendor/common/mi_api/subdir.mk
-include vendor/common/mi_api/libs/subdir.mk
-include vendor/common/mi_api/libs/third_party/micro-ecc/subdir.mk
-include vendor/common/mi_api/mi_vendor/subdir.mk
-include vendor/common/mi_api/mijia_ble_api/subdir.mk
-include vendor/common/nl_api/subdir.mk
-include vendor/mesh_provision/subdir.mk
-include vendor/user_app/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
BUILD_ARTIFACT_FILE_BASE_NAME = B91_mesh_gw

LINKER_OUTPUTS = B91_mesh_gw.elf

GPROF_OUT += \
../gmon.sum ../gmon.out \

PWR_OUT += \
$(SECONDARY_OUTPUT_PATH)/pwr \

GCOV_OUT += \
$(foreach subdir,$(SUBDIRS),$(wildcard $(subdir)/*.gcda) $(wildcard $(subdir)/*.gcno)) \

SYMBOL_OUTPUTS += \
$(SECONDARY_OUTPUT_PATH)/symbol.txt \

READELF_OUTPUTS += \
$(SECONDARY_OUTPUT_PATH)/readelf.txt \

OBJDUMP_OUTPUTS += \
$(SECONDARY_OUTPUT_PATH)/objdump.txt \

SIZE_OUTPUTS += \
$(SECONDARY_OUTPUT_PATH)/.PHONY.size \


# All Target
all: B91_mesh_gw.elf secondary-outputs

# Tool invocations
B91_mesh_gw.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Andes C Linker'
	$(CROSS_COMPILE)gcc -O2 -nostartfiles -static -L"/cygdrive/E/TELINK/Telink_code_9xxx/GateWay/firmware/proj_lib" -flto -g3 -mcpu=d25f -ffunction-sections -fdata-sections -mext-dsp -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fpack-struct -T"/cygdrive/E/TELINK/Telink_code_9xxx/GateWay/firmware/boot.link" -Wl,--gc-sections -mext-dsp -ldsp -lm -o "B91_mesh_gw.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

$(foreach subdir,$(SUBDIRS),$(wildcard $(subdir)/*.gcda) $(wildcard $(subdir)/*.gcno)): B91_mesh_gw.elf $(OBJS) $(USER_OBJS)
../gmon.sum ../gmon.out: B91_mesh_gw.elf $(OBJS) $(USER_OBJS)
$(SECONDARY_OUTPUT_PATH)/pwr: B91_mesh_gw.elf $(OBJS) $(USER_OBJS)

$(SECONDARY_OUTPUT_PATH)/symbol.txt: $(LINKER_OUTPUTS)
	@echo 'Invoking: NM (symbol listing)'
	$(CROSS_COMPILE)nm -n -l -C "B91_mesh_gw.elf" > $(SECONDARY_OUTPUT_PATH)/symbol.txt
	@echo 'Finished building: $@'
	@echo ' '

$(SECONDARY_OUTPUT_PATH)/readelf.txt: $(LINKER_OUTPUTS)
	@echo 'Invoking: Readelf (ELF info listing)'
	$(CROSS_COMPILE)readelf -a "B91_mesh_gw.elf" > $(SECONDARY_OUTPUT_PATH)/readelf.txt
	@echo 'Finished building: $@'
	@echo ' '

$(SECONDARY_OUTPUT_PATH)/objdump.txt: $(LINKER_OUTPUTS)
	@echo 'Invoking: Objdump (disassembly)'
	$(CROSS_COMPILE)objdump -x -d -C "B91_mesh_gw.elf" > $(SECONDARY_OUTPUT_PATH)/objdump.txt
	@echo 'Finished building: $@'
	@echo ' '

: $(LINKER_OUTPUTS)
	@echo 'Invoking: Objcopy (object content copy)'
	$(CROSS_COMPILE)objcopy -S -O binary "B91_mesh_gw.elf"
	@echo 'Finished building: $@'
	@echo ' '

$(SECONDARY_OUTPUT_PATH)/.PHONY.size: $(LINKER_OUTPUTS)
	@echo 'Invoking: Size (section size listing)'
	$(CROSS_COMPILE)size  "B91_mesh_gw.elf" | tee $(SECONDARY_OUTPUT_PATH)/.PHONY.size
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(GPROF_OUT)$(PWR_OUT)$(SIZE_OUTPUTS)$(READELF_OUTPUTS)$(EXECUTABLES)$(OBJDUMP_OUTPUTS)$(OBJS)$(SYMBOL_OUTPUTS)$(S_UPPER_DEPS)$(GCOV_OUT)$(C_DEPS) B91_mesh_gw.elf
	-@echo ' '

post-build:
	-"E:\TELINK\Telink_code_9xxx\GateWay\firmware/tl_check_fw.bat"   B91_mesh_gw
	-@echo ' '

secondary-outputs: $(SYMBOL_OUTPUTS) $(READELF_OUTPUTS) $(OBJDUMP_OUTPUTS) $(SIZE_OUTPUTS)

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
