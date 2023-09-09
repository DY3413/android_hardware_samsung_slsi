/*----------------------------------------------------------------------------
 *      Exynos SoC  -  GPIO
 *----------------------------------------------------------------------------
 *      Name:    gpio9610.c
 *      Purpose: To implement GPIO driver functions
 *      Rev.:    V1.00
 *----------------------------------------------------------------------------
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

#ifndef __GPIO9630_C__
#define __GPIO9630_C__

#include CSP_HEADER(gpio)
#include CSP_HEADER(cmgp)

#define    REG_GPIO_GPH0CON     (GPIO_BASE_ADDRESS + 0x00)
#define    REG_GPIO_GPH1CON     (GPIO_BASE_ADDRESS + 0x20)
#define    REG_GPIO_GPH2CON     (GPIO_BASE_ADDRESS + 0x40)

#define    REG_GPIO_GPM00CON    (GPIO_CMGP_BASE_ADDRESS + 0x00)
#define    REG_GPIO_GPM01CON    (GPIO_CMGP_BASE_ADDRESS + 0x20)
#define    REG_GPIO_GPM02CON    (GPIO_CMGP_BASE_ADDRESS + 0x40)
#define    REG_GPIO_GPM03CON    (GPIO_CMGP_BASE_ADDRESS + 0x60)
#define    REG_GPIO_GPM04CON    (GPIO_CMGP_BASE_ADDRESS + 0x80)
#define    REG_GPIO_GPM05CON    (GPIO_CMGP_BASE_ADDRESS + 0xA0)
#define    REG_GPIO_GPM06CON    (GPIO_CMGP_BASE_ADDRESS + 0xC0)
#define    REG_GPIO_GPM07CON    (GPIO_CMGP_BASE_ADDRESS + 0xE0)
#define    REG_GPIO_GPM08CON    (GPIO_CMGP_BASE_ADDRESS + 0x100)
#define    REG_GPIO_GPM09CON    (GPIO_CMGP_BASE_ADDRESS + 0x120)
#define    REG_GPIO_GPM10CON    (GPIO_CMGP_BASE_ADDRESS + 0x140)
#define    REG_GPIO_GPM11CON    (GPIO_CMGP_BASE_ADDRESS + 0x160)
#define    REG_GPIO_GPM12CON    (GPIO_CMGP_BASE_ADDRESS + 0x180)
#define    REG_GPIO_GPM13CON    (GPIO_CMGP_BASE_ADDRESS + 0x1A0)
#define    REG_GPIO_GPM14CON    (GPIO_CMGP_BASE_ADDRESS + 0x1C0)
#define    REG_GPIO_GPM15CON    (GPIO_CMGP_BASE_ADDRESS + 0x1E0)

#define    REG_GPIO_NWEINT_GPH0_CON        (GPIO_BASE_ADDRESS + 0x700)
#define    REG_GPIO_NWEINT_GPH1_CON        (GPIO_BASE_ADDRESS + 0x704)
#define    REG_GPIO_NWEINT_GPH2_CON        (GPIO_BASE_ADDRESS + 0x708)
#define    REG_GPIO_NWEINT_GPH0_FLTCON0    (GPIO_BASE_ADDRESS + 0x800)
#define    REG_GPIO_NWEINT_GPH0_FLTCON1    (GPIO_BASE_ADDRESS + 0x804)
#define    REG_GPIO_NWEINT_GPH1_FLTCON0    (GPIO_BASE_ADDRESS + 0x808)
#define    REG_GPIO_NWEINT_GPH2_FLTCON0    (GPIO_BASE_ADDRESS + 0x80C)
#define    REG_GPIO_NWEINT_GPH2_FLTCON1    (GPIO_BASE_ADDRESS + 0x810)

#define    REG_GPIO_NWEINT_GPM00_CON       (GPIO_CMGP_BASE_ADDRESS + 0x700)
#define    REG_GPIO_NWEINT_GPM01_CON       (GPIO_CMGP_BASE_ADDRESS + 0x704)
#define    REG_GPIO_NWEINT_GPM02_CON       (GPIO_CMGP_BASE_ADDRESS + 0x708)
#define    REG_GPIO_NWEINT_GPM03_CON       (GPIO_CMGP_BASE_ADDRESS + 0x70C)
#define    REG_GPIO_NWEINT_GPM04_CON       (GPIO_CMGP_BASE_ADDRESS + 0x710)
#define    REG_GPIO_NWEINT_GPM05_CON       (GPIO_CMGP_BASE_ADDRESS + 0x714)
#define    REG_GPIO_NWEINT_GPM06_CON       (GPIO_CMGP_BASE_ADDRESS + 0x718)
#define    REG_GPIO_NWEINT_GPM07_CON       (GPIO_CMGP_BASE_ADDRESS + 0x71C)
#define    REG_GPIO_NWEINT_GPM08_CON       (GPIO_CMGP_BASE_ADDRESS + 0x720)
#define    REG_GPIO_NWEINT_GPM09_CON       (GPIO_CMGP_BASE_ADDRESS + 0x724)
#define    REG_GPIO_NWEINT_GPM10_CON       (GPIO_CMGP_BASE_ADDRESS + 0x728)
#define    REG_GPIO_NWEINT_GPM11_CON       (GPIO_CMGP_BASE_ADDRESS + 0x72C)
#define    REG_GPIO_NWEINT_GPM12_CON       (GPIO_CMGP_BASE_ADDRESS + 0x730)
#define    REG_GPIO_NWEINT_GPM13_CON       (GPIO_CMGP_BASE_ADDRESS + 0x734)
#define    REG_GPIO_NWEINT_GPM14_CON       (GPIO_CMGP_BASE_ADDRESS + 0x738)
#define    REG_GPIO_NWEINT_GPM15_CON       (GPIO_CMGP_BASE_ADDRESS + 0x73C)
#define    REG_GPIO_NWEINT_GPM00_FLTCON0   (GPIO_CMGP_BASE_ADDRESS + 0x800)
#define    REG_GPIO_NWEINT_GPM01_FLTCON0   (GPIO_CMGP_BASE_ADDRESS + 0x804)
#define    REG_GPIO_NWEINT_GPM02_FLTCON0   (GPIO_CMGP_BASE_ADDRESS + 0x808)
#define    REG_GPIO_NWEINT_GPM03_FLTCON0   (GPIO_CMGP_BASE_ADDRESS + 0x80C)
#define    REG_GPIO_NWEINT_GPM04_FLTCON0   (GPIO_CMGP_BASE_ADDRESS + 0x810)
#define    REG_GPIO_NWEINT_GPM05_FLTCON0   (GPIO_CMGP_BASE_ADDRESS + 0x814)
#define    REG_GPIO_NWEINT_GPM06_FLTCON0   (GPIO_CMGP_BASE_ADDRESS + 0x818)
#define    REG_GPIO_NWEINT_GPM07_FLTCON0   (GPIO_CMGP_BASE_ADDRESS + 0x81C)
#define    REG_GPIO_NWEINT_GPM08_FLTCON0   (GPIO_CMGP_BASE_ADDRESS + 0x820)
#define    REG_GPIO_NWEINT_GPM09_FLTCON0   (GPIO_CMGP_BASE_ADDRESS + 0x824)
#define    REG_GPIO_NWEINT_GPM10_FLTCON0   (GPIO_CMGP_BASE_ADDRESS + 0x828)
#define    REG_GPIO_NWEINT_GPM11_FLTCON0   (GPIO_CMGP_BASE_ADDRESS + 0x82C)
#define    REG_GPIO_NWEINT_GPM12_FLTCON0   (GPIO_CMGP_BASE_ADDRESS + 0x830)
#define    REG_GPIO_NWEINT_GPM13_FLTCON0   (GPIO_CMGP_BASE_ADDRESS + 0x834)
#define    REG_GPIO_NWEINT_GPM14_FLTCON0   (GPIO_CMGP_BASE_ADDRESS + 0x838)
#define    REG_GPIO_NWEINT_GPM15_FLTCON0   (GPIO_CMGP_BASE_ADDRESS + 0x83C)

#define REG_DAT_OFFSET               0x4
#define REG_PUD_OFFSET               0x8
#define REG_DRV_OFFSET               0xC

#define REG_NWEINT_FLTCON_OFFSET     0x100
#define REG_NWEINT_MASK_OFFSET       0x200
#define REG_NWEINT_PEND_OFFSET       0x300

#define REG_CON_BITFIELD             (4)
#define REG_DAT_BITFIELD             (1)
#define REG_PUD_BITFIELD             (4)
#define REG_DRV_BITFIELD             (4)

#define REG_NWEINT_CON_BITFIELD      (4)
#define REG_NWEINT_FLTCON_BITFIELD   (8)
#define REG_NWEINT_MASK_BITFIELD     (1)
#define REG_NWEINT_PEND_BITFIELD     (1)

#define REG_FILTER_EN_SHIFT          (7)
#define REG_FILTER_SEL_SHIFT         (6)
#define REG_FILTER_WIDTH_BITFIELD    (6)

#define DEC_CHUB_GPIO(group, idx, fltcon) \
    .pin = group##_##idx, \
    .index = idx, \
    .irqNum = EXTINT_##group##_##idx, \
    .conBase = REG_GPIO_##group##CON, \
    .eintBase = REG_GPIO_NWEINT_##group##_CON, \
    .eintFltcon = REG_GPIO_NWEINT_##group##_FLTCON##fltcon

#define DEC_CMGP_GPIO(group, idx, fltcon) \
    .pin = group##_##idx, \
    .index = idx, \
    .irqNum = CMGP_##group##_IRQ, \
    .conBase = REG_GPIO_##group##CON, \
    .eintBase = REG_GPIO_NWEINT_##group##_CON, \
    .eintFltcon = REG_GPIO_NWEINT_##group##_FLTCON##fltcon

struct Gpio mGpioPinInfo[] = {
    /* CHUB BPIO */
    { DEC_CHUB_GPIO(GPH0, 0, 0) },
    { DEC_CHUB_GPIO(GPH0, 1, 0) },
    { DEC_CHUB_GPIO(GPH0, 2, 0) },
    { DEC_CHUB_GPIO(GPH0, 3, 0) },
    { DEC_CHUB_GPIO(GPH0, 4, 1) },
    { DEC_CHUB_GPIO(GPH0, 5, 1) },
    { DEC_CHUB_GPIO(GPH0, 6, 1) },
    { DEC_CHUB_GPIO(GPH0, 7, 1) },

    { DEC_CHUB_GPIO(GPH1, 0, 0) },
    { DEC_CHUB_GPIO(GPH1, 1, 0) },
    { DEC_CHUB_GPIO(GPH1, 2, 0) },
    { DEC_CHUB_GPIO(GPH1, 3, 0) },

    { DEC_CHUB_GPIO(GPH2, 0, 0) },
    { DEC_CHUB_GPIO(GPH2, 1, 0) },
    { DEC_CHUB_GPIO(GPH2, 2, 0) },
    { DEC_CHUB_GPIO(GPH2, 3, 0) },
    { DEC_CHUB_GPIO(GPH2, 4, 0) },
    { DEC_CHUB_GPIO(GPH2, 5, 0) },

    /* CMGP GPIO */
    { DEC_CMGP_GPIO(GPM00, 0, 0) },
    { DEC_CMGP_GPIO(GPM01, 0, 0) },
    { DEC_CMGP_GPIO(GPM02, 0, 0) },
    { DEC_CMGP_GPIO(GPM03, 0, 0) },
    { DEC_CMGP_GPIO(GPM04, 0, 0) },
    { DEC_CMGP_GPIO(GPM05, 0, 0) },
    { DEC_CMGP_GPIO(GPM06, 0, 0) },
    { DEC_CMGP_GPIO(GPM07, 0, 0) },
    { DEC_CMGP_GPIO(GPM08, 0, 0) },
    { DEC_CMGP_GPIO(GPM09, 0, 0) },
    { DEC_CMGP_GPIO(GPM10, 0, 0) },
    { DEC_CMGP_GPIO(GPM11, 0, 0) },
    { DEC_CMGP_GPIO(GPM12, 0, 0) },
    { DEC_CMGP_GPIO(GPM13, 0, 0) },
    { DEC_CMGP_GPIO(GPM14, 0, 0) },
    { DEC_CMGP_GPIO(GPM15, 0, 0) },
};

#endif

