#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#find build target
PLATFORM ?= stm32
CHIP ?= stm32f411
CPU ?= cortexm4
VARIANT ?= lunchbox
DEBUG ?= -DDEBUG
OUT := out/nanohub/$(VARIANT)

#bad words
BADWORDS += strcpy strcat atoi
BADWORDS += "rsaPrivOp=RSA private ops must never be compiled into firmware."

NANOHUB_ROOT := ..

#find makefiles
MAKE_PLAT = os/platform/$(PLATFORM)/$(PLATFORM).mk
MAKE_CPU = os/cpu/$(CPU)/$(CPU).mk

ifndef VARIANT_PATH
VARIANT_PATH := variant/$(VARIANT)
endif

ifndef MAKE_VAR
MAKE_VAR = $(VARIANT_PATH)/$(VARIANT).mk
endif

#top make target
SRCS_os :=
SRCS_bl :=
DELIVERABLES :=

.PHONY: real all
real: all

#include makefiles for plat and cpu
include $(MAKE_PLAT)
include $(MAKE_CPU)
include $(MAKE_VAR)

FLAGS += -Ios/algos
FLAGS += -Ios/cpu/$(CPU)/inc
FLAGS += -Ios/inc
FLAGS += -Ios/inc/chre
FLAGS += -Ios/platform/$(PLATFORM)/inc
FLAGS += -I$(VARIANT_PATH)/inc
FLAGS += -Iexternal/freebsd/inc
FLAGS += -I$(NANOHUB_ROOT)/lib/include
FLAGS += -I../../../../system/chre/chre_api/include/chre_api
FLAGS += -Wall -Werror
ifeq ($(EXYNOS_CONTEXTHUB), YES)
FLAGS += -I$(VARIANT_PATH)/inc/variant
FLAGS += -Ios/inc/plat
FLAGS += -Ios/inc/plat/cmgp
FLAGS += -Ios/inc/plat/cmu
FLAGS += -Ios/inc/plat/cpu
FLAGS += -Ios/inc/plat/csp
FLAGS += -Ios/inc/plat/gpio
FLAGS += -Ios/inc/plat/i2c
FLAGS += -Ios/inc/plat/mct
FLAGS += -Ios/inc/plat/mailbox
FLAGS += -Ios/inc/plat/dmailbox
FLAGS += -Ios/inc/plat/pwm
FLAGS += -Ios/inc/plat/pwr
FLAGS += -Ios/inc/plat/rtc
FLAGS += -Ios/inc/plat/spi
FLAGS += -Ios/inc/plat/sysreg
FLAGS += -Ios/inc/plat/usi
FLAGS += -Ios/inc/plat/wdt
FLAGS += -Ios/inc/plat/adc
FLAGS += -Ios/inc/plat/pdma
FLAGS += -Ios/inc/plat/timer
FLAGS += -Ios/inc/plat/uart
FLAGS += -Ios/inc/plat/csis
FLAGS += -Ios/inc/plat/slif
endif

FLAGS += -Wall -Werror -fshort-double -fdiagnostics-color

#help avoid commmon embedded C mistakes
FLAGS += -Wmissing-declarations -Wlogical-op -Waddress -Wempty-body -Wno-pointer-arith -Wenum-compare -Wdouble-promotion -Wfloat-equal -Wshadow -fno-strict-aliasing

OSFLAGS += -g -ggdb3 -D_OS_BUILD_ -Os
OSFLAGS += -DCHIP=$(CHIP)
OSFLAGS_os += -DUSE_PRINTF_FLAG_CHARS

#debug mode
FLAGS += $(DEBUG)

include firmware_conf.mk

FLAGS += $(COMMON_FLAGS)

ifeq ($(SUPPORT_EXT_APP), YES)
FLAGS += -DSUPPORT_EXT_APP
#bootloader pieces
SRCS_bl += $(NANOHUB_ROOT)/lib/nanohub/sha2.c $(NANOHUB_ROOT)/lib/nanohub/rsa.c $(NANOHUB_ROOT)/lib/nanohub/aes.c os/core/seos.c
endif

#frameworks
SRCS_os += os/core/spi.c
SRCS_os += os/core/printf.c os/core/timer.c os/core/seos.c os/core/heap.c os/core/slab.c os/core/trylock.c
#SRCS_os += os/core/hostIntf.c os/core/hostIntfI2c.c os/core/hostIntfSpi.c os/core/nanohubCommand.c os/core/sensors.c os/core/syscall.c
SRCS_os += os/core/hostIntf.c os/core/nanohubCommand.c os/core/sensors.c os/core/syscall.c
SRCS_os += os/core/eventQ.c os/core/appSec.c os/core/simpleQ.c os/core/floatRt.c os/core/nanohub_chre.c
SRCS_os += os/algos/ap_hub_sync.c
ifeq ($(SUPPORT_EXT_APP), YES)
SRCS_os += os/core/osApi.c
endif
ifeq ($(EXYNOS_CONTEXTHUB),)
SRCS_bl += os/core/bl.c
endif

#some help for bootloader
SRCS_bl += os/core/printf.c

SRCS_os += $(NANOHUB_ROOT)/lib/nanohub/softcrc.c
SRCS_os += $(NANOHUB_ROOT)/lib/nanohub/sha2.c

#extra deps
DEPS += $(wildcard inc/*.h)
DEPS += $(wildcard ../inc/*.h)
DEPS += $(wildcard ../inc/chre/*.h)
DEPS += $(wildcard $(VARIANT_PATH)/inc/variant/*.h)
DEPS += firmware.mk firmware_conf.mk $(MAKE_PLAT) $(MAKE_CPU) $(MAKE_VAR)
DELIVERABLES += $(OUT)/full.bin

all: $(DELIVERABLES)

$(OUT)/bl.unchecked.elf: $(SRCS_bl) $(DEPS)
	mkdir -p $(dir $@)
	$(GCC) -o $@ $(SRCS_bl) $(OSFLAGS) $(OSFLAGS_bl) $(FLAGS)

$(OUT)/os.unchecked.elf: $(SRCS_os) $(DEPS)
	mkdir -p $(dir $@)
	$(GCC) -o $@ $(SRCS_os) $(OSFLAGS) $(OSFLAGS_os) $(FLAGS)

$(OUT)/%.checked.elf : $(OUT)/%.unchecked.elf  symcheck.sh
	mkdir -p $(dir $@)
	./symcheck.sh $< $@ $(BADWORDS)

$(OUT)/full.bin: $(BL_FILE) $(OS_FILE)
	mkdir -p $(dir $@)
	cat $(BL_FILE) $(OS_FILE) > $@

clean:
	rm -rf $(OUT)

.SECONDARY: $(OUT)/os.checked.elf
