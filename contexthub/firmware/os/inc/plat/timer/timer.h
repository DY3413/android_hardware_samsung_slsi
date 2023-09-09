/*----------------------------------------------------------------------------
 *      Exynos SoC  -  TIMER
 *----------------------------------------------------------------------------
 *      Name:    timer.h
 *      Purpose: To expose TIMER APIs and define macros
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

#ifndef __TIMER_H__
#define __TIMER_H__

#include <csp_common.h>
#include <csp_assert.h>
#include <csp_printf.h>

void timerInit(void);
void timerSet(uint32_t msec);
void timerUnset(void);

void timerIRQHandler(void);

#endif  /* */