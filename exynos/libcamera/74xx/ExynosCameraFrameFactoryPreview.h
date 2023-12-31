/*
**
** Copyright 2013, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_FRAME_FACTORY_PREVIEW_H
#define EXYNOS_CAMERA_FRAME_FACTORY_PREVIEW_H

#include "ExynosCameraFrameFactory.h"

#include "ExynosCameraFrame.h"

namespace android {

class ExynosCameraFrameFactoryPreview : public ExynosCameraFrameFactory {
public:
    ExynosCameraFrameFactoryPreview()
    {
        m_init();
    }

    ExynosCameraFrameFactoryPreview(int cameraId, ExynosCameraParameters *param)
    {
        m_init();

        m_cameraId = cameraId;
        m_parameters = param;
        m_activityControl = m_parameters->getActivityControl();

        const char *myName = (m_cameraId == CAMERA_ID_BACK) ? "FrameFactoryBackPreview" : "FrameFactoryFrontPreview";
        strncpy(m_name, myName,  EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    }

public:
    virtual ~ExynosCameraFrameFactoryPreview();

    virtual status_t        create(bool active = true);
#ifdef SAMSUNG_COMPANION
    virtual status_t        precreate(void);
    virtual status_t        postcreate(void);
#endif

    virtual status_t        fastenAeStable(int32_t numFrames, ExynosCameraBuffer *buffers);

    virtual ExynosCameraFrame *createNewFrame(void);

    virtual status_t        initPipes(void);
    virtual status_t        preparePipes(void);

    virtual status_t        startPipes(void);
    virtual status_t        startInitialThreads(void);
    virtual status_t        setStopFlag(void);
    virtual status_t        stopPipes(void);

protected:
    virtual status_t        m_fillNodeGroupInfo(ExynosCameraFrame *frame);
    virtual status_t        m_setupConfig(void);

    /* setting node number on every pipe */
    virtual status_t        m_setDeviceInfo(void) = 0;

    /* pipe setting */
    virtual status_t        m_initPipes(void) = 0;

    /* pipe setting for fastAE */
    virtual status_t        m_initPipesFastenAeStable(int32_t numFrames,
                                                      int hwSensorW, int hwSensorH,
                                                      int hwPreviewW, int hwPreviewH) = 0;

private:
    void                    m_init(void);

};

}; /* namespace android */

#endif
