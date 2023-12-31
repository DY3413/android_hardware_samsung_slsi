/*
 * Copyright (C) 2020 Samsung Electronics Co., Ltd.
 *
 * Contact: Boojin Kim <boojin.kim@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _MAILBOX_CHUB_IPC_HW_H
#define _MAILBOX_CHUB_IPC_HW_H

#include "ipc_type.h"

/*  mailbox Registers */
#define REG_MAILBOX_MCUCTL (0x000)
#define REG_MAILBOX_INTGR0 (0x008)
#define REG_MAILBOX_INTCR0 (0x00C)
#define REG_MAILBOX_INTMR0 (0x010)
#define REG_MAILBOX_INTSR0 (0x014)
#define REG_MAILBOX_INTMSR0 (0x018)
#define REG_MAILBOX_INTGR1 (0x01C)
#define REG_MAILBOX_INTCR1 (0x020)
#define REG_MAILBOX_INTMR1 (0x024)
#define REG_MAILBOX_INTSR1 (0x028)
#define REG_MAILBOX_INTMSR1 (0x02C)

#define REG_MAILBOX_ISSR0 (0x100)
#define REG_MAILBOX_ISSR1 (0x104)
#define REG_MAILBOX_ISSR2 (0x108)
#define REG_MAILBOX_ISSR3 (0x10c)

#define IPC_HW_IRQ_MAX (16)

#define IPC_HW_READ_STATUS(base) \
	__raw_readl((base) + REG_MAILBOX_INTSR0)
#define IPC_HW_READ_STATUS1(base) \
	__raw_readl((base) + REG_MAILBOX_INTSR1)
#define IPC_HW_READ_PEND(base, irq) \
	(__raw_readl((base) + REG_MAILBOX_INTSR1) & (1 << (irq)))
#define IPC_HW_CLEAR_PEND(base, irq) \
	__raw_writel(1 << (irq), (base) + REG_MAILBOX_INTCR0)
#define IPC_HW_CLEAR_PEND1(base, irq) \
	__raw_writel(1 << (irq), (base) + REG_MAILBOX_INTCR1)
#define IPC_HW_WRITE_SHARED_REG(base, num, data) \
	__raw_writel((data), (base) + REG_MAILBOX_ISSR0 + (num) * 4)
#define IPC_HW_READ_SHARED_REG(base, num) \
	__raw_readl((base) + REG_MAILBOX_ISSR0 + (num) * 4)
#define IPC_HW_GEN_INTERRUPT_GR1(base, num) \
	__raw_writel(1 << (num), (base) + REG_MAILBOX_INTGR1)
#define IPC_HW_GEN_INTERRUPT_GR0(base, num) \
	__raw_writel(1 << ((num) + 16), (base) + REG_MAILBOX_INTGR0)
#define IPC_HW_SET_MCUCTL(base, val) \
	__raw_write32((val), (base) + REG_MAILBOX_MCUCTL)

enum ipc_hw_dir {
	IPC_DST,
	IPC_SRC,
};

struct ipc_owner_ctrl {
	enum ipc_hw_dir src;
	void *base;
};

#define IRQ_HW_MAX (16)
#define MB0_BIT_OFFSET (IRQ_HW_MAX)

enum ipc_mb_id {
	IPC_SRC_MB0,
	IPC_DST_MB1,
};

int ipc_hw_start_bit(enum ipc_mb_id which_mb);
void ipc_hw_set_mcuctrl(void *base, unsigned int val);
void ipc_hw_write_shared_reg(void *base, unsigned int val, int num);
unsigned int ipc_hw_read_shared_reg(void *base, int num);

unsigned int ipc_hw_read_int_gen_reg(void *base, enum ipc_mb_id which_mb);
void ipc_hw_gen_interrupt(void *base, enum ipc_mb_id which_mb, int irq);
void ipc_hw_clear_all_int_pend_reg(void *base, enum ipc_mb_id which_mb);
void ipc_hw_clear_int_pend_reg(void *base, enum ipc_mb_id which_mb, int irq);
unsigned int ipc_hw_read_int_status_reg_all(void *base,
					    enum ipc_mb_id which_mb);
unsigned int ipc_hw_read_int_status_reg(void *base, enum ipc_mb_id which_mb,
					int irq);
#endif
