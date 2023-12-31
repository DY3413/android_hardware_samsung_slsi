/*
**
** Copyright 2017, Samsung Electronics Co. LTD
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

/*#define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraSensorInfo"
#include <log/log.h>

#include "ExynosCameraSensorInfo.h"

namespace android {

struct ExynosCameraSensorInfoBase *createExynosCameraSensorInfo(int cameraId)
{
    char sensorName[20] = {'\0',};
    struct ExynosCameraSensorInfoBase *sensorInfo = NULL;
    int sensorId = getSensorId(cameraId);
    if (sensorId < 0) {
        ALOGE("[CAM_ID(%d)]-ERR(%s[%d]):Inavalid sensorId %d",
                cameraId, __FUNCTION__, __LINE__, sensorId);
        sensorId = SENSOR_NAME_NOTHING;
    }

    switch (sensorId) {
    case SENSOR_NAME_S5K2L7:
        sensorInfo = new ExynosCameraSensor2L7();
        snprintf(sensorName, sizeof(sensorName), "S5K2L7");
        break;
    case SENSOR_NAME_S5K2P8:
        sensorInfo = new ExynosCameraSensor2P8();
        snprintf(sensorName, sizeof(sensorName), "S5K2P8");
        break;
    case SENSOR_NAME_S5K3M3:
        sensorInfo = new ExynosCameraSensor3M3(sensorId);
        snprintf(sensorName, sizeof(sensorName), "S5K3M3");
        break;
    case SENSOR_NAME_SAK2L3:
        sensorInfo = new ExynosCameraSensor2L3(sensorId);
        snprintf(sensorName, sizeof(sensorName), "SAK2L3");
        break;
    case SENSOR_NAME_IMX333:
        sensorInfo = new ExynosCameraSensorIMX333_2L2(sensorId);
        snprintf(sensorName, sizeof(sensorName), "IMX333");
        break;
    case SENSOR_NAME_S5K2L2:
        sensorInfo = new ExynosCameraSensorIMX333_2L2(sensorId);
        snprintf(sensorName, sizeof(sensorName), "S5K2L2");
        break;
    case SENSOR_NAME_IMX320:
        sensorInfo = new ExynosCameraSensorIMX320_3H1(sensorId);
        snprintf(sensorName, sizeof(sensorName), "IMX320");
        break;
    case SENSOR_NAME_S5K3H1:
        sensorInfo = new ExynosCameraSensorIMX320_3H1(sensorId);
        snprintf(sensorName, sizeof(sensorName), "S5K3H1");
        break;
    case SENSOR_NAME_S5K5F1:
        sensorInfo = new ExynosCameraSensorS5K5F1(sensorId);
        snprintf(sensorName, sizeof(sensorName), "S5K5F1");
        break;
    case SENSOR_NAME_S5KRPB:
        sensorInfo = new ExynosCameraSensorS5KRPB(sensorId);
        snprintf(sensorName, sizeof(sensorName), "RPB");
        break;
    case SENSOR_NAME_S5K2P7SQ:
        sensorInfo = new ExynosCameraSensorS5K2P7SQ(sensorId);
        snprintf(sensorName, sizeof(sensorName), "S5K2P7SQ");
		break;
    case SENSOR_NAME_S5K2T7SX:
        sensorInfo = new ExynosCameraSensorS5K2T7SX(sensorId);
        snprintf(sensorName, sizeof(sensorName), "S5K2T7SX");
        break;
    case SENSOR_NAME_S5K6B2:
        sensorInfo = new ExynosCameraSensor6B2(sensorId);
        snprintf(sensorName, sizeof(sensorName), "6B2");
        break;
    default:
        android_printAssert(NULL, LOG_TAG, "[CAM_ID(%d)]-ASSERT(%s[%d]):Unknown sensorId %d",
                cameraId, __FUNCTION__, __LINE__, sensorId);
        break;
    }

    ALOGI("[CAM_ID(%d)]-INFO(%s[%d]):sensorId %d name %s",
            cameraId, __FUNCTION__, __LINE__, sensorId, sensorName);

    return sensorInfo;
}

ExynosCameraSensor2L7::ExynosCameraSensor2L7() : ExynosCameraSensor2L7Base()
{
    /* Use ExynosCameraSensor2L7Base Constructor */
};

ExynosCameraSensor2P8::ExynosCameraSensor2P8() : ExynosCameraSensor2P8Base()
{
    /* Use ExynosCameraSensorS5K2P8Base Constructor */
};

ExynosCameraSensorIMX333_2L2::ExynosCameraSensorIMX333_2L2(int sensorId) : ExynosCameraSensorIMX333_2L2Base(sensorId)
{
    /* Use ExynosCameraSensorIMX333_2L2Base Constructor */
};

ExynosCameraSensorIMX320_3H1::ExynosCameraSensorIMX320_3H1(int sensorId) : ExynosCameraSensorIMX320_3H1Base(sensorId)
{
    /* Use ExynosCameraSensorIMX320_3H1Base Constructor */
};

ExynosCameraSensor2L3::ExynosCameraSensor2L3(int sensorId) : ExynosCameraSensor2L3Base(sensorId)
{
    /* Optional capabilities : vendor feature */
    supportedCapabilities |= (CAPABILITIES_PRIVATE_REPROCESSING | CAPABILITIES_RAW |
                                CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO);
};

ExynosCameraSensor3M3::ExynosCameraSensor3M3(int sensorId) : ExynosCameraSensor3M3Base(sensorId)
{
    /* Use ExynosCameraSensorS5K3M3Base Constructor */
};

ExynosCameraSensorS5K5F1::ExynosCameraSensorS5K5F1(int sensorId) : ExynosCameraSensorS5K5F1Base(sensorId)
{
    gain = 20;                      // 2.0;
    exposureTime = 332 * 100000;    // 33.2ms;
    ledCurrent = 5;                 // 450mA
    ledPulseDelay = 0 * 100000;     // 0ms
    ledPulseWidth = 240 * 100000;   // 24ms
    ledMaxTime = 10 * 1000;         // 10s;

    gainRange[MIN] = 1;
    gainRange[MAX] = 160;
    ledCurrentRange[MIN] = 1;                // 0mA
    ledCurrentRange[MAX] = 10;               // 950mA
    ledPulseDelayRange[MIN] = 0 * 100000;    // 0.0ms
    ledPulseDelayRange[MAX] = 1000 * 100000; // 100.0ms
    ledPulseWidthRange[MIN] = 0 * 100000;    // 0.0ms
    ledPulseWidthRange[MAX] = 333 * 100000;  // 33.3ms
    ledMaxTimeRange[MIN] = 1 * 1000;         // 1s
    ledMaxTimeRange[MAX] = 10 * 1000;        // 10s
};

ExynosCameraSensorS5KRPB::ExynosCameraSensorS5KRPB(int sensorId) : ExynosCameraSensorS5KRPBBase(sensorId)
{
    /* Use ExynosCameraSensorS5KRPBBase Constructor */
};

ExynosCameraSensorS5K2P7SQ::ExynosCameraSensorS5K2P7SQ(int sensorId) : ExynosCameraSensor2P7SQBase(sensorId)
{
    /* Use ExynosCameraSensorS5K2P7SQBase Constructor */
};

ExynosCameraSensorS5K2T7SX::ExynosCameraSensorS5K2T7SX(int sensorId) : ExynosCameraSensor2T7SXBase(sensorId)
{
    /* Use ExynosCameraSensorS5KRPBBase Constructor */
};

ExynosCameraSensor6B2::ExynosCameraSensor6B2(int sensorId) : ExynosCameraSensor6B2Base(sensorId)
{
    /* Use ExynosCameraSensorS5KRPBBase Constructor */
};

}; /* namespace android */
