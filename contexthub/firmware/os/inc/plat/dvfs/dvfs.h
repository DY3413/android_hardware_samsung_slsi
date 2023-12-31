/*----------------------------------------------------------------------------
 *      Exynos SoC  -  DVFS
 *----------------------------------------------------------------------------
 *      Name:    dvfs.h
 *      Purpose: To expose DVFS APIs and define macros
 *      Rev.:    V1.00
 *---------------------------------------------------------------------------
 *
 * Copyright (C) 2021 Sukmin Kang
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

#ifndef __EXYNOS_DVFS_H__
#define __EXYNOS_DVFS_H__

#include <stdint.h>

void dvfsInit(void);
void dvfs_IRQHandler(void);
int dvfsRequestFreq(uint32_t clk);

#endif
