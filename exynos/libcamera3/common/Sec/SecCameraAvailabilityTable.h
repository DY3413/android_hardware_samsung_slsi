/*
** Copyright 2015, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef SEC_CAMERA_AVAILABILITY_TABLE_H
#define SEC_CAMERA_AVAILABILITY_TABLE_H

#include "SecCameraVendorTags.h"

#ifdef SAMSUNG_CONTROL_METERING
static int32_t AVAILABLE_VENDOR_METERING_MODES[] =
{
    (int32_t)SAMSUNG_ANDROID_CONTROL_METERING_MODE_CENTER,
    (int32_t)SAMSUNG_ANDROID_CONTROL_METERING_MODE_SPOT,
    (int32_t)SAMSUNG_ANDROID_CONTROL_METERING_MODE_MATRIX,
    (int32_t)SAMSUNG_ANDROID_CONTROL_METERING_MODE_MANUAL,
    (int32_t)SAMSUNG_ANDROID_CONTROL_METERING_MODE_WEIGHTED_CENTER,
    (int32_t)SAMSUNG_ANDROID_CONTROL_METERING_MODE_WEIGHTED_SPOT,
    (int32_t)SAMSUNG_ANDROID_CONTROL_METERING_MODE_WEIGHTED_MATRIX,
};
#endif

#ifdef SAMSUNG_OIS
static int32_t AVAILABLE_VENDOR_OIS_MODES[] =
{
    (int32_t)SAMSUNG_ANDROID_LENS_OPTICAL_STABILIZATION_OPERATION_MODE_PICTURE,
    (int32_t)SAMSUNG_ANDROID_LENS_OPTICAL_STABILIZATION_OPERATION_MODE_VIDEO,
};
#endif

#ifdef SAMSUNG_RTHDR
static int32_t AVAILABLE_VENDOR_HDR_MODES[] =
{
    (int32_t)SAMSUNG_ANDROID_CONTROL_LIVE_HDR_MODE_OFF,
    (int32_t)SAMSUNG_ANDROID_CONTROL_LIVE_HDR_MODE_ON,
    (int32_t)SAMSUNG_ANDROID_CONTROL_LIVE_HDR_MODE_AUTO,
};
#endif

static int32_t AVAILABLE_VENDOR_FLIP_MODES[] =
{
    (int32_t)SAMSUNG_ANDROID_SCALER_FLIP_MODE_NONE,
    (int32_t)SAMSUNG_ANDROID_SCALER_FLIP_MODE_HFLIP,
};

static int32_t AVAILABLE_VENDOR_AWB_MODES[] =
{
    (int32_t)ANDROID_CONTROL_AWB_MODE_OFF,
    (int32_t)ANDROID_CONTROL_AWB_MODE_AUTO,
    (int32_t)ANDROID_CONTROL_AWB_MODE_INCANDESCENT,
    (int32_t)ANDROID_CONTROL_AWB_MODE_FLUORESCENT,
    (int32_t)ANDROID_CONTROL_AWB_MODE_DAYLIGHT,
    (int32_t)ANDROID_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT,
    (int32_t)SAMSUNG_ANDROID_CONTROL_AWB_MODE_CUSTOM_K,
};

static uint8_t AVAILABLE_VENDOR_AE_MODES_FLASH_MODES[] =
{
    (uint8_t)SAMSUNG_ANDROID_CONTROL_AE_MODE_ON_AUTO_SCREEN_FLASH,
    (uint8_t)SAMSUNG_ANDROID_CONTROL_AE_MODE_ON_ALWAYS_SCREEN_FLASH,
    (uint8_t)SAMSUNG_ANDROID_CONTROL_AE_MODE_OFF_ALWAYS_FLASH,
};

#ifdef SUPPORT_MULTI_AF
static uint8_t AVAILABLE_VENDOR_MULTI_AF_MODE[] =
{
    (uint8_t)SAMSUNG_ANDROID_CONTROL_MULTI_AF_MODE_OFF,
    (uint8_t)SAMSUNG_ANDROID_CONTROL_MULTI_AF_MODE_ON,
};
#endif

#ifdef SAMSUNG_OT
static int32_t AVAILABLE_VENDOR_AF_MODES_OBJECT_TRACKING[] =
{
    (int32_t)SAMSUNG_ANDROID_CONTROL_AF_MODE_OBJECT_TRACKING_PICTURE,
    (int32_t)SAMSUNG_ANDROID_CONTROL_AF_MODE_OBJECT_TRACKING_VIDEO,
};
#endif

#ifdef SAMSUNG_FIXED_FACE_FOCUS
static uint8_t AVAILABLE_VENDOR_AF_MODES_FIXED_FACE_FOCUS[] =
{
    (int32_t)SAMSUNG_ANDROID_CONTROL_AF_MODE_FIXED_FACE,
};
#endif

static uint8_t AVAILABLE_VENDOR_EFFECTS[] =
{
    (uint8_t)SAMSUNG_ANDROID_CONTROL_EFFECT_MODE_BEATUTY,
};

static int32_t AVAILABLE_VENDOR_FEATURES[] =
{
#ifdef SAMSUNG_LLS_DEBLUR
    (int32_t)SAMSUNG_ANDROID_CONTROL_AVAILABLE_FEATURE_LLS_CAPTURE,
#endif
#ifdef SAMSUNG_FEATURE_SHUTTER_NOTIFICATION
    (int32_t)SAMSUNG_ANDROID_CONTROL_AVAILABLE_FEATURE_SHUTTER_NOTIFICATION,
#endif
};
#endif
