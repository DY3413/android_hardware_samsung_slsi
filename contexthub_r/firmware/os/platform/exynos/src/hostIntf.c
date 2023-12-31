/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <hostIntf.h>
#include <hostIntf_priv.h>
#include <variant.h>

const struct HostIntfComm *platHostIntfInit()
{
	const struct HostIntfComm *comm = hostIntfMailboxInit();
	return comm;
}

uint16_t platHwType(void)
{
	return PLATFORM_HW_TYPE;
}

uint16_t platHwVer(void)
{
	return PLATFORM_HW_VER;
}

uint16_t platBlVer(void)
{
	return PLATFORM_BL_VER;
}
