/*----------------------------------------------------------------------------
 *      Exynos SoC  -  WDT
 *----------------------------------------------------------------------------
 *      Name:    wdtDrv.h
 *      Purpose: To expose wdt driver functions
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

#ifndef __WDT_DRV_H__
#define __WDT_DRV_H__

#include <csp_common.h>

void wdtDrvInit(uint32_t mSec, uint32_t intEn, uint32_t resetEn);
void wdtDrvEnable(void);
void wdtDrvDisable(void);
void wdtDrvSetTime(uint32_t mSec);
void wdtDrvEnableInterrupt(void);
void wdtDrvDisableInterrupt(void);
void wdtDrvEnableReset(void);
void wdtDrvDisableReset(void);
void wdtDrvClose(void);
void wdtDrvPendClear(void);

#endif
