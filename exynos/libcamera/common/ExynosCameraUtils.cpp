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

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraUtils"
#include <cutils/log.h>

#include "ExynosCameraUtils.h"

#define ADD_BAYER_BY_NEON

namespace android {

status_t getCropRectAlign(
        int  src_w,  int   src_h,
        int  dst_w,  int   dst_h,
        int *crop_x, int *crop_y,
        int *crop_w, int *crop_h,
        int align_w, int align_h,
        int zoom, float zoomRatio)
{
    *crop_w = src_w;
    *crop_h = src_h;

    if (src_w == 0 || src_h == 0 || dst_w == 0 || dst_h == 0) {
        ALOGE("ERR(%s):width or height values is 0, src(%dx%d), dst(%dx%d)",
                 __func__, src_w, src_h, dst_w, dst_h);
        return BAD_VALUE;
    }

    /* Calculation aspect ratio */
    if (   src_w != dst_w
        || src_h != dst_h) {
        float src_ratio = 1.0f;
        float dst_ratio = 1.0f;

        /* ex : 1024 / 768 */
        src_ratio = ROUND_OFF_HALF(((float)src_w / (float)src_h), 2);

        /* ex : 352  / 288 */
        dst_ratio = ROUND_OFF_HALF(((float)dst_w / (float)dst_h), 2);

        if (dst_ratio <= src_ratio) {
            /* shrink w */
            *crop_w = src_h * ((float)dst_w / (float)dst_h);
            *crop_h = src_h;
        } else {
            /* shrink h */
            *crop_w = src_w;
            *crop_h = src_w / ((float)dst_w / (float)dst_h);
        }
    }

    /* Calculation zoom */
    if (zoom != 0) {
#if defined(SCALER_MAX_SCALE_UP_RATIO)
        /*
         * After dividing float & casting int,
         * zoomed size can be smaller too much.
         * so, when zoom until max, ceil up about floating point.
         */
        if (((int)((float)*crop_w / zoomRatio)) * SCALER_MAX_SCALE_UP_RATIO < *crop_w ||
            ((int)((float)*crop_h / zoomRatio)) * SCALER_MAX_SCALE_UP_RATIO < *crop_h) {
            *crop_w = (int)ceil(((float)*crop_w / zoomRatio));
            *crop_h = (int)ceil(((float)*crop_h / zoomRatio));
        } else
#endif
        {
            *crop_w = (int)((float)*crop_w / zoomRatio);
            *crop_h = (int)((float)*crop_h / zoomRatio);
        }
    }

    if (dst_w == dst_h) {
        int align_value = 0;
        align_value = (align_w < align_h)? align_h : align_w;

        *crop_w = ALIGN_UP(*crop_w, align_value);
        *crop_h = ALIGN_UP(*crop_h, align_value);

        if (*crop_w > src_w) {
            *crop_w = ALIGN_DOWN(src_w, align_value);
            *crop_h = *crop_w;
        } else if (*crop_h > src_h) {
            *crop_h = ALIGN_DOWN(src_h, align_value);
            *crop_w = *crop_h;
        }
    } else {
        *crop_w = ALIGN_UP(*crop_w, align_w);
        *crop_h = ALIGN_UP(*crop_h, align_h);

        if (*crop_w > src_w)
            *crop_w = ALIGN_DOWN(src_w, align_w);
        if (*crop_h > src_h)
            *crop_h = ALIGN_DOWN(src_h, align_h);
    }

    *crop_x = ALIGN_DOWN(((src_w - *crop_w) >> 1), 2);
    *crop_y = ALIGN_DOWN(((src_h - *crop_h) >> 1), 2);

    if (*crop_x < 0 || *crop_y < 0) {
        ALOGE("ERR(%s):crop size too big (%d, %d, %d, %d)",
                 __func__, *crop_x, *crop_y, *crop_w, *crop_h);
        return BAD_VALUE;
    }

    return NO_ERROR;
}

uint32_t bracketsStr2Ints(
        char *str,
        uint32_t num,
        ExynosRect2 *rect2s,
        int *weights,
        int mode)
{
    char *curStr = str;
    char buf[128];
    char *bracketsOpen;
    char *bracketsClose;

    int tempArray[5] = {0, };
    uint32_t validFocusedAreas = 0;
    bool isValid;
    bool nullArea = false;
    isValid = true;

    for (uint32_t i = 0; i < num + 1; i++) {
        if (curStr == NULL) {
            if (i != num) {
                nullArea = false;
            }
            break;
        }

        bracketsOpen = strchr(curStr, '(');
        if (bracketsOpen == NULL) {
            if (i != num) {
                nullArea = false;
            }
            break;
        }

        bracketsClose = strchr(bracketsOpen, ')');
        if (bracketsClose == NULL) {
            ALOGE("ERR(%s):subBracketsStr2Ints(%s) fail", __func__, buf);
            if (i != num) {
                nullArea = false;
            }
            break;
        } else if (i == num) {
            ALOGE("ERR(%s):Invalid Value(%d/%d), original data(%s)", __FUNCTION__, i, num, curStr);
            return 0;
        }

        strncpy(buf, bracketsOpen, bracketsClose - bracketsOpen + 1);
        buf[bracketsClose - bracketsOpen + 1] = 0;

        if (subBracketsStr2Ints(5, buf, tempArray) == false) {
            nullArea = false;
            break;
        }

        rect2s[i].x1 = tempArray[0];
        rect2s[i].y1 = tempArray[1];
        rect2s[i].x2 = tempArray[2];
        rect2s[i].y2 = tempArray[3];
        weights[i] = tempArray[4];

        if (mode) {
            isValid = true;

            for (int j = 0; j < 4; j++) {
                if (tempArray[j] < -1000 || tempArray[j] > 1000)
                    isValid = false;
            }

            if (tempArray[4] < 0 || tempArray[4] > 1000)
                isValid = false;

            if (!rect2s[i].x1 && !rect2s[i].y1 && !rect2s[i].x2 && !rect2s[i].y2 && !weights[i])
                nullArea = true;
            else if (weights[i] == 0)
                isValid = false;
            else if (!(tempArray[0] == 0 && tempArray[2] == 0) && tempArray[0] >= tempArray[2])
                isValid = false;
            else if (!(tempArray[1] == 0 && tempArray[3] == 0) && tempArray[1] >= tempArray[3])
                isValid = false;
            else if (!(tempArray[0] == 0 && tempArray[2] == 0) && (tempArray[1] == 0 && tempArray[3] == 0))
                isValid = false;
            else if ((tempArray[0] == 0 && tempArray[2] == 0) && !(tempArray[1] == 0 && tempArray[3] == 0))
                isValid = false;

            if (isValid) {
                validFocusedAreas++;
            } else {
                ALOGE("ERR(%s):CurStr(%s), buf(%s)", __FUNCTION__, curStr, buf);
                ALOGE("ERR(%s):Invalid Value(%d,%d,%d,%d,%d)",
                    __FUNCTION__, tempArray[0], tempArray[1], tempArray[2], tempArray[3], tempArray[4]);

                return 0;
            }
        } else {
            if (rect2s[i].x1 || rect2s[i].y1 || rect2s[i].x2 || rect2s[i].y2 || weights[i])
                validFocusedAreas++;
        }

        curStr = bracketsClose;
    }
    if (nullArea && mode)
        validFocusedAreas = num;

    if (validFocusedAreas == 0)
        validFocusedAreas = 1;

    ALOGD("(%s):(%d,%d,%d,%d,%d) - validFocusedAreas(%d)",
        __FUNCTION__, tempArray[0], tempArray[1], tempArray[2], tempArray[3], tempArray[4], validFocusedAreas);

    return validFocusedAreas;
}

bool subBracketsStr2Ints(int num, char *str, int *arr)
{
    if (str == NULL || arr == NULL) {
        ALOGE("ERR(%s):str or arr is NULL", __func__);
        return false;
    }

    /* ex : (-10,-10,0,0,300) */
    char buf[128];
    char *bracketsOpen;
    char *bracketsClose;
    char *tok;
    char *savePtr;

    bracketsOpen = strchr(str, '(');
    if (bracketsOpen == NULL) {
        ALOGE("ERR(%s):no '('", __func__);
        return false;
    }

    bracketsClose = strchr(bracketsOpen, ')');
    if (bracketsClose == NULL) {
        ALOGE("ERR(%s):no ')'", __func__);
        return false;
    }

    strncpy(buf, bracketsOpen + 1, bracketsClose - bracketsOpen + 1);
    buf[bracketsClose - bracketsOpen + 1] = 0;

    tok = strtok_r(buf, ",", &savePtr);
    if (tok == NULL) {
        ALOGE("ERR(%s):strtok_r(%s) fail", __func__, buf);
        return false;
    }

    arr[0] = atoi(tok);

    for (int i = 1; i < num; i++) {
        tok = strtok_r(NULL, ",", &savePtr);
        if (tok == NULL) {
            if (i < num - 1) {
                ALOGE("ERR(%s):strtok_r() (index : %d, num : %d) fail", __func__, i, num);
                return false;
            }
            break;
        }

        arr[i] = atoi(tok);
    }

    return true;
}

void convertingRectToRect2(ExynosRect *rect, ExynosRect2 *rect2)
{
    rect2->x1 = rect->x;
    rect2->y1 = rect->y;
    rect2->x2 = rect->x + rect->w;
    rect2->y2 = rect->y + rect->h;
}

void convertingRect2ToRect(ExynosRect2 *rect2, ExynosRect *rect)
{
    rect->x = rect2->x1;
    rect->y = rect2->y1;
    rect->w = rect2->x2 - rect2->x1;
    rect->h = rect2->y2 - rect2->y1;
}

bool isRectNull(ExynosRect *rect)
{
    if (   rect->x == 0
        && rect->y == 0
        && rect-> w == 0
        && rect->h == 0
        && rect->fullW == 0
        && rect->fullH == 0
        && rect->colorFormat == 0)
        return true;

    return false;
}

bool isRectNull(ExynosRect2 *rect2)
{
    if (   rect2->x1 == 0
        && rect2->y1 == 0
        && rect2->x2 == 0
        && rect2->y2 == 0)
        return true;

    return false;
}

bool isRectEqual(ExynosRect *rect1, ExynosRect *rect2)
{
    if (   rect1->x == rect2->x
        && rect1->y == rect2->y
        && rect1->w == rect2->w
        && rect1->h == rect2->h
        && rect1->fullW == rect2->fullW
        && rect1->fullH == rect2->fullH
        && rect1->colorFormat == rect2->colorFormat)
        return true;

    return false;
}

bool isRectEqual(ExynosRect2 *rect1, ExynosRect2 *rect2)
{
    if (   rect1->x1 == rect2->x1
        && rect1->y1 == rect2->y1
        && rect1->x2 == rect2->x2
        && rect1->y2 == rect2->y2)
        return true;

    return false;
}

ExynosRect2 convertingActualArea2HWArea(ExynosRect2 *srcRect, const ExynosRect *regionRect)
{
    int x = regionRect->x;
    int y = regionRect->y;
    int w = regionRect->w;
    int h = regionRect->h;

    ExynosRect2 newRect2;

    newRect2.x1 = srcRect->x1 - x;
    newRect2.y1 = srcRect->y1 - y;
    newRect2.x2 = srcRect->x2 - x;
    newRect2.y2 = srcRect->y2 - y;

    if (newRect2.x1 < 0)
        newRect2.x1 = 0;
    else if (w <= newRect2.x1)
        newRect2.x1 = w - 1;

    if (newRect2.y1 < 0)
        newRect2.y1 = 0;
    else if (h <= newRect2.y1)
        newRect2.y1 = h - 1;

    if (newRect2.x2 < 0)
        newRect2.x2 = 0;
    else if (w <= newRect2.x2)
        newRect2.x2 = w - 1;

    if (newRect2.y2 < 0)
        newRect2.y2 = 0;
    else if (h <= newRect2.y2)
        newRect2.y2 = h - 1;

    if (newRect2.x2 < newRect2.x1)
        newRect2.x2 = newRect2.x1;

    if (newRect2.y2 < newRect2.y1)
        newRect2.y2 = newRect2.y1;

    ALOGV("INFO(%s[%d]): src(%d %d %d %d) region(%d %d %d %d) newRect(%d %d %d %d)", __FUNCTION__, __LINE__,
        srcRect->x1, srcRect->y1, srcRect->x2, srcRect->y2,
        regionRect->x , regionRect->y, regionRect->w, regionRect->h,
        newRect2.x1 , newRect2.y1, newRect2.x2, newRect2.y2);

    return newRect2;
}

ExynosRect2 convertingAndroidArea2HWArea(ExynosRect2 *srcRect, const ExynosRect *regionRect)
{
    int x = regionRect->x;
    int y = regionRect->y;
    int w = regionRect->w;
    int h = regionRect->h;

    ExynosRect2 newRect2;

    newRect2.x1 = (srcRect->x1 + 1000) * w / 2000;
    newRect2.y1 = (srcRect->y1 + 1000) * h / 2000;
    newRect2.x2 = (srcRect->x2 + 1000) * w / 2000;
    newRect2.y2 = (srcRect->y2 + 1000) * h / 2000;

    if (newRect2.x1 < 0)
        newRect2.x1 = 0;
    else if (w <= newRect2.x1)
        newRect2.x1 = w - 1;

    if (newRect2.y1 < 0)
        newRect2.y1 = 0;
    else if (h <= newRect2.y1)
        newRect2.y1 = h - 1;

    if (newRect2.x2 < 0)
        newRect2.x2 = 0;
    else if (w <= newRect2.x2)
        newRect2.x2 = w - 1;

    if (newRect2.y2 < 0)
        newRect2.y2 = 0;
    else if (h <= newRect2.y2)
        newRect2.y2 = h - 1;

    if (newRect2.x2 < newRect2.x1)
        newRect2.x2 = newRect2.x1;

    if (newRect2.y2 < newRect2.y1)
        newRect2.y2 = newRect2.y1;

    ALOGV("INFO(%s[%d]): src(%d %d %d %d) region(%d %d %d %d) newRect(%d %d %d %d)", __FUNCTION__, __LINE__,
        srcRect->x1, srcRect->y1, srcRect->x2, srcRect->y2,
        regionRect->x , regionRect->y, regionRect->w, regionRect->h,
        newRect2.x1 , newRect2.y1, newRect2.x2, newRect2.y2);

    return newRect2;
}

ExynosRect2 convertingAndroidArea2HWAreaBcropOut(ExynosRect2 *srcRect, const ExynosRect *regionRect)
{
    /* do nothing, same as noraml converting */
    return convertingAndroidArea2HWArea(srcRect, regionRect);
}

ExynosRect2 convertingAndroidArea2HWAreaBcropIn(ExynosRect2 *srcRect, const ExynosRect *regionRect)
{
    ExynosRect2 newRect2;

    int x = regionRect->x;
    int y = regionRect->y;
    int w = regionRect->w;
    int h = regionRect->h;

    newRect2 = convertingAndroidArea2HWArea(srcRect, regionRect);

    /* add x, y size */
    newRect2.x1 += x;
    newRect2.y1 += y;
    newRect2.x2 += x;
    newRect2.y2 += y;

    ALOGI("INFO(%s[%d]): src(%d %d %d %d) region(%d %d %d %d) newRect(%d %d %d %d)", __FUNCTION__, __LINE__,
        srcRect->x1, srcRect->y1, srcRect->x2, srcRect->y2,
        regionRect->x , regionRect->y, regionRect->w, regionRect->h,
        newRect2.x1 , newRect2.y1, newRect2.x2, newRect2.y2);

    return newRect2;
}

status_t getResolutionList(String8 &string8Buf, struct ExynosSensorInfoBase *sensorInfo,
                            int *w, int *h, int mode, int camid)
{
    bool found = false;
    bool flagFirst = true;
    char strBuf[32];
    int sizeOfResSize = 0;
    int cropX = 0, cropY = 0, cropW = 0, cropH = 0;
    int max_w = 0, max_h = 0;

    /* this is up to /packages/apps/Camera/res/values/arrays.xml */
    int (*RESOLUTION_LIST)[SIZE_OF_RESOLUTION] = {NULL,};

    switch (mode) {
    case MODE_PREVIEW:
        if (camid == CAMERA_ID_BACK) {
            RESOLUTION_LIST = sensorInfo->rearPreviewList;
            sizeOfResSize = sensorInfo->rearPreviewListMax;
        } else {
            RESOLUTION_LIST = sensorInfo->frontPreviewList;
            sizeOfResSize = sensorInfo->frontPreviewListMax;
        }
        break;
    case MODE_PICTURE:
        if (camid == CAMERA_ID_BACK) {
            RESOLUTION_LIST = sensorInfo->rearPictureList;
            sizeOfResSize = sensorInfo->rearPictureListMax;
        } else {
            RESOLUTION_LIST = sensorInfo->frontPictureList;
            sizeOfResSize = sensorInfo->frontPictureListMax;
        }
        break;
    case MODE_VIDEO:
        if (camid == CAMERA_ID_BACK) {
            RESOLUTION_LIST = sensorInfo->rearVideoList;
            sizeOfResSize = sensorInfo->rearVideoListMax;
        } else {
            RESOLUTION_LIST = sensorInfo->frontVideoList;
            sizeOfResSize = sensorInfo->frontVideoListMax;
        }
        break;
    case MODE_THUMBNAIL:
        RESOLUTION_LIST = sensorInfo->thumbnailList;
        sizeOfResSize = sensorInfo->thumbnailListMax;
        break;
    default:
        ALOGE("ERR(%s):invalid mode(%d)", __func__, mode);
        return BAD_VALUE;
    }

    for (int i = 0; i < sizeOfResSize; i++) {
        if (   RESOLUTION_LIST[i][0] <= *w
            && RESOLUTION_LIST[i][1] <= *h) {
            if (flagFirst == true) {
                snprintf(strBuf, sizeof(strBuf), "%dx%d", RESOLUTION_LIST[i][0], RESOLUTION_LIST[i][1]);
                string8Buf.append(strBuf);
                max_w = RESOLUTION_LIST[i][0];
                max_h = RESOLUTION_LIST[i][1];

                flagFirst = false;
            } else {
                if ((mode == MODE_PICTURE || mode == MODE_THUMBNAIL) &&
                    ((max_w) / 16 > RESOLUTION_LIST[i][0] ||
                     (max_h / 16) > RESOLUTION_LIST[i][1]))
                    continue;

                snprintf(strBuf, sizeof(strBuf), ",%dx%d", RESOLUTION_LIST[i][0], RESOLUTION_LIST[i][1]);
                string8Buf.append(strBuf);
            }

            found = true;
        }
    }

    if (found == false) {
        ALOGE("ERR(%s):cannot find resolutions", __func__);
    } else {
        *w = max_w;
        *h = max_h;
    }

    return NO_ERROR;
}

void setZoomRatioList(int *list, int len, float maxZoomRatio)
{
    float zoom_ratio_delta = pow(maxZoomRatio, 1.0f / len);

    for (int i = 0; i <= len; i++) {
        list[i] = (int)(pow(zoom_ratio_delta, i) * 1000);
        ALOGV("INFO(%s):list[%d]:(%d), (%f)", __func__, i, list[i], (float)((float)list[i] / 1000));
    }
}

status_t getZoomRatioList(String8 & string8Buf, int maxZoom, int maxZoomRatio, int *list)
{
    bool flagFirst = true;
    char strBuf[32];

    int cur = 0;
    int step = maxZoom - 1;

    setZoomRatioList(list, maxZoom - 1, (float)(maxZoomRatio / 1000));

    for (int i = 0; i < step; i++) {
        cur = (int)(list[i] / 10);
        snprintf(strBuf, sizeof(strBuf), "%d", cur);
        string8Buf.append(strBuf);
        string8Buf.append(",");
    }

    snprintf(strBuf, sizeof(strBuf), "%d", (maxZoomRatio / 10));
    string8Buf.append(strBuf);

    /* ex : "100,130,160,190,220,250,280,310,340,360,400" */

    return NO_ERROR;
}

status_t getSupportedFpsList(String8 & string8Buf, int min, int max, int camid, struct ExynosSensorInfoBase *sensorInfo)
{
    bool found = false;
    bool flagFirst = true;
    char strBuf[32];
    int numOfList = 0;
    int (*sizeList)[2];

    if (camid == CAMERA_ID_BACK) {
        sizeList = sensorInfo->rearFPSList;
        for (int i = 0; i < sensorInfo->rearFPSListMax; i++) {
            if (min <= sizeList[i][0] &&
                sizeList[i][1] <= max) {
                if (flagFirst == true) {
                    flagFirst = false;
                    snprintf(strBuf, sizeof(strBuf), "(%d,%d)", sizeList[i][0], sizeList[i][1]);
                } else {
                    snprintf(strBuf, sizeof(strBuf), ",(%d,%d)", sizeList[i][0], sizeList[i][1]);
                }
                string8Buf.append(strBuf);

                found = true;
            }
        }
    } else {
        sizeList = sensorInfo->frontFPSList;
        for (int i = 0; i < sensorInfo->frontFPSListMax; i++) {
            if (min <= sizeList[i][0] &&
                sizeList[i][1] <= max) {
                if (flagFirst == true) {
                    flagFirst = false;
                    snprintf(strBuf, sizeof(strBuf), "(%d,%d)", sizeList[i][0], sizeList[i][1]);
                } else {
                    snprintf(strBuf, sizeof(strBuf), ",(%d,%d)", sizeList[i][0], sizeList[i][1]);
                }
                string8Buf.append(strBuf);

                found = true;
            }
        }
    }

    if (found == false)
        ALOGE("ERR(%s):cannot find fps list", __func__);

    return NO_ERROR;
}


int32_t getMetaDmRequestFrameCount(struct camera2_shot_ext *shot_ext)
{
    if (shot_ext == NULL) {
        ALOGE("ERR(%s[%d]): buffer is NULL", __FUNCTION__, __LINE__);
        return -1;
    }
    return shot_ext->shot.dm.request.frameCount;
}

int32_t getMetaDmRequestFrameCount(struct camera2_dm *dm)
{
    if (dm == NULL) {
        ALOGE("ERR(%s[%d]): buffer is NULL", __FUNCTION__, __LINE__);
        return -1;
    }
    return dm->request.frameCount;
}

void setMetaCtlAeTargetFpsRange(struct camera2_shot_ext *shot_ext, uint32_t min, uint32_t max)
{
    ALOGI("INFO(%s):aeTargetFpsRange(min=%d, min=%d)", __FUNCTION__, min, max);
    shot_ext->shot.ctl.aa.aeTargetFpsRange[0] = min;
    shot_ext->shot.ctl.aa.aeTargetFpsRange[1] = max;
}

void getMetaCtlAeTargetFpsRange(struct camera2_shot_ext *shot_ext, uint32_t *min, uint32_t *max)
{
    *min = shot_ext->shot.ctl.aa.aeTargetFpsRange[0];
    *max = shot_ext->shot.ctl.aa.aeTargetFpsRange[1];
}

void setMetaCtlSensorFrameDuration(struct camera2_shot_ext *shot_ext, uint64_t duration)
{
    shot_ext->shot.ctl.sensor.frameDuration = duration;
}

void getMetaCtlSensorFrameDuration(struct camera2_shot_ext *shot_ext, uint64_t *duration)
{
    *duration = shot_ext->shot.ctl.sensor.frameDuration;
}

void setMetaCtlAeMode(struct camera2_shot_ext *shot_ext, enum aa_aemode aeMode)
{
    if (shot_ext->shot.ctl.sensor.exposureTime == 0)
        shot_ext->shot.ctl.aa.aeMode = aeMode;
}

void getMetaCtlAeMode(struct camera2_shot_ext *shot_ext, enum aa_aemode *aeMode)
{
    *aeMode = shot_ext->shot.ctl.aa.aeMode;
}
#if defined(USE_HAL3_2_METADATA_INTERFACE)
void setMetaCtlAeLock(struct camera2_shot_ext *shot_ext, bool lock)
{
    if (lock == true)
        shot_ext->shot.ctl.aa.aeLock = AA_AE_LOCK_ON;
    else
        shot_ext->shot.ctl.aa.aeLock = AA_AE_LOCK_OFF;
}

void getMetaCtlAeLock(struct camera2_shot_ext *shot_ext, bool *lock)
{
    if (shot_ext->shot.ctl.aa.aeLock == AA_AE_LOCK_OFF)
        *lock = false;
    else
        *lock = true;
}
#endif

#ifdef USE_SUBDIVIDED_EV
void setMetaCtlExposureCompensationStep(struct camera2_shot_ext *shot_ext, float expCompensationStep)
{
    shot_ext->shot.ctl.aa.vendor_aeExpCompensationStep = expCompensationStep;
}
#endif

void setMetaCtlExposureCompensation(struct camera2_shot_ext *shot_ext, int32_t expCompensation)
{
    shot_ext->shot.ctl.aa.aeExpCompensation = expCompensation;
}

void getMetaCtlExposureCompensation(struct camera2_shot_ext *shot_ext, int32_t *expCompensation)
{
    *expCompensation = shot_ext->shot.ctl.aa.aeExpCompensation;
}

void setMetaCtlExposureTime(struct camera2_shot_ext *shot_ext, uint64_t exposureTime)
{
    if (exposureTime != 0) {
        shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_OFF;
        setMetaCtlAeRegion(shot_ext, 0, 0, 0, 0, 0);
    }
    shot_ext->shot.ctl.sensor.exposureTime = exposureTime;
}

void getMetaCtlExposureTime(struct camera2_shot_ext *shot_ext, uint64_t *exposureTime)
{
    *exposureTime = shot_ext->shot.ctl.sensor.exposureTime;
}

#if defined(USE_HAL3_2_METADATA_INTERFACE)
void setMetaCtlCaptureExposureTime(struct camera2_shot_ext *shot_ext, uint32_t exposureTime)
{
    shot_ext->shot.ctl.aa.vendor_captureExposureTime = exposureTime;
}

void getMetaCtlCaptureExposureTime(struct camera2_shot_ext *shot_ext, uint32_t *exposureTime)
{
    *exposureTime = shot_ext->shot.ctl.aa.vendor_captureExposureTime;
}
#endif

void setMetaCtlWbLevel(struct camera2_shot_ext *shot_ext, int32_t wbLevel)
{
#if defined(USE_HAL3_2_METADATA_INTERFACE)
    shot_ext->shot.ctl.aa.vendor_awbValue = wbLevel;
#else
    shot_ext->shot.ctl.aa.awbValue = wbLevel;
#endif
}

void getMetaCtlWbLevel(struct camera2_shot_ext *shot_ext, int32_t *wbLevel)
{
#if defined(USE_HAL3_2_METADATA_INTERFACE)
    *wbLevel = shot_ext->shot.ctl.aa.vendor_awbValue;
#else
    *wbLevel = shot_ext->shot.ctl.aa.awbValue;
#endif
}

status_t setMetaCtlCropRegion(
        struct camera2_shot_ext *shot_ext,
        int x, int y, int w, int h)
{
    shot_ext->shot.ctl.scaler.cropRegion[0] = x;
    shot_ext->shot.ctl.scaler.cropRegion[1] = y;
    shot_ext->shot.ctl.scaler.cropRegion[2] = w;
    shot_ext->shot.ctl.scaler.cropRegion[3] = h;

    return NO_ERROR;
}
status_t setMetaCtlCropRegion(
        struct camera2_shot_ext *shot_ext,
        int zoom,
        int srcW, int srcH,
        int dstW, int dstH, float zoomRatio)
{
    int newX = 0;
    int newY = 0;
    int newW = 0;
    int newH = 0;

    if (getCropRectAlign(srcW,  srcH,
                         dstW,  dstH,
                         &newX, &newY,
                         &newW, &newH,
                         CAMERA_MAGIC_ALIGN, 2,
                         zoom, zoomRatio) != NO_ERROR) {
        ALOGE("ERR(%s):getCropRectAlign(%d, %d, %d, %d) fail",
            __func__, srcW,  srcH, dstW,  dstH);
        return BAD_VALUE;
    }

    newX = ALIGN(newX, 2);
    newY = ALIGN(newY, 2);

    ALOGV("DEBUG(%s):size(%d, %d, %d, %d), level(%d)",
        __FUNCTION__, newX, newY, newW, newH, zoom);

    shot_ext->shot.ctl.scaler.cropRegion[0] = newX;
    shot_ext->shot.ctl.scaler.cropRegion[1] = newY;
    shot_ext->shot.ctl.scaler.cropRegion[2] = newW;
    shot_ext->shot.ctl.scaler.cropRegion[3] = newH;

    return NO_ERROR;
}

void getMetaCtlCropRegion(
        struct camera2_shot_ext *shot_ext,
        int *x, int *y,
        int *w, int *h)
{
    *x = shot_ext->shot.ctl.scaler.cropRegion[0];
    *y = shot_ext->shot.ctl.scaler.cropRegion[1];
    *w = shot_ext->shot.ctl.scaler.cropRegion[2];
    *h = shot_ext->shot.ctl.scaler.cropRegion[3];
}

void setMetaCtlAeRegion(
        struct camera2_shot_ext *shot_ext,
        int x, int y,
        int w, int h,
        int weight)
{
    shot_ext->shot.ctl.aa.aeRegions[0] = x;
    shot_ext->shot.ctl.aa.aeRegions[1] = y;
    shot_ext->shot.ctl.aa.aeRegions[2] = w;
    shot_ext->shot.ctl.aa.aeRegions[3] = h;
    shot_ext->shot.ctl.aa.aeRegions[4] = weight;
}

void getMetaCtlAeRegion(
        struct camera2_shot_ext *shot_ext,
        int *x, int *y,
        int *w, int *h,
        int *weight)
{
    *x = shot_ext->shot.ctl.aa.aeRegions[0];
    *y = shot_ext->shot.ctl.aa.aeRegions[1];
    *w = shot_ext->shot.ctl.aa.aeRegions[2];
    *h = shot_ext->shot.ctl.aa.aeRegions[3];
    *weight = shot_ext->shot.ctl.aa.aeRegions[4];
}


void setMetaCtlAntibandingMode(struct camera2_shot_ext *shot_ext, enum aa_ae_antibanding_mode antibandingMode)
{
    shot_ext->shot.ctl.aa.aeAntibandingMode = antibandingMode;
}

void getMetaCtlAntibandingMode(struct camera2_shot_ext *shot_ext, enum aa_ae_antibanding_mode *antibandingMode)
{
    *antibandingMode = shot_ext->shot.ctl.aa.aeAntibandingMode;
}

void setMetaCtlSceneMode(struct camera2_shot_ext *shot_ext, enum aa_mode mode, enum aa_scene_mode sceneMode)
{
#ifdef USE_NEW_NOISE_REDUCTION_ALGORITHM
    enum processing_mode default_edge_mode = PROCESSING_MODE_FAST;
    enum processing_mode default_noise_mode = PROCESSING_MODE_FAST;
    int default_edge_strength = 5;
    int default_noise_strength = 5;
#else
    enum processing_mode default_edge_mode = PROCESSING_MODE_OFF;
    enum processing_mode default_noise_mode = PROCESSING_MODE_OFF;
    int default_edge_strength = 0;
    int default_noise_strength = 0;
#endif

    shot_ext->shot.ctl.aa.mode = mode;
    shot_ext->shot.ctl.aa.sceneMode = sceneMode;

    switch (sceneMode) {
    case AA_SCENE_MODE_FACE_PRIORITY:
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF &&
            shot_ext->shot.ctl.sensor.exposureTime == 0)
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.aa.sceneMode = AA_SCENE_MODE_DISABLED;
        if(shot_ext->shot.ctl.aa.vendor_isoMode != AA_ISOMODE_MANUAL) {
            shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
            shot_ext->shot.ctl.aa.vendor_isoValue = 0;
            shot_ext->shot.ctl.sensor.sensitivity = 0;
        }
#else
        shot_ext->shot.ctl.aa.sceneMode = AA_SCENE_MODE_UNSUPPORTED;
        if(shot_ext->shot.ctl.aa.isoMode != AA_ISOMODE_MANUAL) {
            shot_ext->shot.ctl.aa.isoMode = AA_ISOMODE_AUTO;
            shot_ext->shot.ctl.aa.isoValue = 0;
        }
#endif
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
#else
        shot_ext->shot.ctl.color.saturation = 3; /* "3" is default. */
#endif
        break;
    case AA_SCENE_MODE_ACTION:
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF)
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoValue = 0;
        shot_ext->shot.ctl.sensor.sensitivity = 0;
#else
        shot_ext->shot.ctl.aa.isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.isoValue = 0;
#endif
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
#else
        shot_ext->shot.ctl.color.saturation = 3; /* "3" is default. */
#endif
        break;
    case AA_SCENE_MODE_PORTRAIT:
    case AA_SCENE_MODE_LANDSCAPE:
        /* set default setting */
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF)
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoValue = 0;
        shot_ext->shot.ctl.sensor.sensitivity = 0;
#else
        shot_ext->shot.ctl.aa.isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.isoValue = 0;
#endif
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
#else
        shot_ext->shot.ctl.color.saturation = 3; /* "3" is default. */
#endif
        break;
    case AA_SCENE_MODE_NIGHT:
        /* AE_LOCK is prohibited */
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF ||
#if defined(USE_HAL3_2_METADATA_INTERFACE)
            shot_ext->shot.ctl.aa.aeLock == AA_AE_LOCK_ON
#else
            shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_LOCKED
#endif
        ) {
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;
        }

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoValue = 0;
        shot_ext->shot.ctl.sensor.sensitivity = 0;
#else
        shot_ext->shot.ctl.aa.isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.isoValue = 0;
#endif
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
#else
        shot_ext->shot.ctl.color.saturation = 3; /* "3" is default. */
#endif
        break;
    case AA_SCENE_MODE_NIGHT_PORTRAIT:
    case AA_SCENE_MODE_THEATRE:
    case AA_SCENE_MODE_BEACH:
    case AA_SCENE_MODE_SNOW:
        /* set default setting */
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF)
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoValue = 0;
        shot_ext->shot.ctl.sensor.sensitivity = 0;
#else
        shot_ext->shot.ctl.aa.isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.isoValue = 0;
#endif
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
#else
        shot_ext->shot.ctl.color.saturation = 3; /* "3" is default. */
#endif
        break;
    case AA_SCENE_MODE_SUNSET:
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF)
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_DAYLIGHT;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoValue = 0;
        shot_ext->shot.ctl.sensor.sensitivity = 0;
#else
        shot_ext->shot.ctl.aa.isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.isoValue = 0;
#endif
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
#else
        shot_ext->shot.ctl.color.saturation = 3; /* "3" is default. */
#endif
        break;
    case AA_SCENE_MODE_STEADYPHOTO:
    case AA_SCENE_MODE_FIREWORKS:
    case AA_SCENE_MODE_SPORTS:
        /* set default setting */
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF)
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoValue = 0;
        shot_ext->shot.ctl.sensor.sensitivity = 0;
#else
        shot_ext->shot.ctl.aa.isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.isoValue = 0;
#endif
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
#else
        shot_ext->shot.ctl.color.saturation = 3; /* "3" is default. */
#endif
        break;
    case AA_SCENE_MODE_PARTY:
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF)
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_MANUAL;
        shot_ext->shot.ctl.aa.vendor_isoValue = 200;
        shot_ext->shot.ctl.sensor.sensitivity = 200;
#else
        shot_ext->shot.ctl.aa.isoMode = AA_ISOMODE_MANUAL;
        shot_ext->shot.ctl.aa.isoValue = 200;
#endif
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.color.vendor_saturation = 4; /* "4" is default + 1. */
#else
        shot_ext->shot.ctl.color.saturation = 4; /* "4" is default + 1. */
#endif
        break;
    case AA_SCENE_MODE_CANDLELIGHT:
        /* set default setting */
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF)
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoValue = 0;
        shot_ext->shot.ctl.sensor.sensitivity = 0;
#else
        shot_ext->shot.ctl.aa.isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.isoValue = 0;
#endif
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
#else
        shot_ext->shot.ctl.color.saturation = 3; /* "3" is default. */
#endif
        break;
#ifdef SAMSUNG_FOOD_MODE
    case AA_SCENE_MODE_FOOD:
        shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_MATRIX;

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoValue = 0;
        shot_ext->shot.ctl.sensor.sensitivity = 0;
#else
        shot_ext->shot.ctl.aa.isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.isoValue = 0;
#endif
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
#else
        shot_ext->shot.ctl.color.saturation = 3; /* "3" is default. */
#endif
        break;
#endif
    case AA_SCENE_MODE_AQUA:
        /* set default setting */
        if (shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_OFF)
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;

        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_isoValue = 0;
        shot_ext->shot.ctl.sensor.sensitivity = 0;
#else
        shot_ext->shot.ctl.aa.isoMode = AA_ISOMODE_AUTO;
        shot_ext->shot.ctl.aa.isoValue = 0;
#endif
        shot_ext->shot.ctl.noise.mode = default_noise_mode;
        shot_ext->shot.ctl.noise.strength = default_noise_strength;
        shot_ext->shot.ctl.edge.mode = default_edge_mode;
        shot_ext->shot.ctl.edge.strength = default_edge_strength;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
        shot_ext->shot.ctl.color.vendor_saturation = 3; /* "3" is default. */
#else
        shot_ext->shot.ctl.color.saturation = 3; /* "3" is default. */
#endif
        break;
    default:
        break;
    }
}

void getMetaCtlSceneMode(struct camera2_shot_ext *shot_ext, enum aa_mode *mode, enum aa_scene_mode *sceneMode)
{
    *mode = shot_ext->shot.ctl.aa.mode;
    *sceneMode = shot_ext->shot.ctl.aa.sceneMode;
}

void setMetaCtlAwbMode(struct camera2_shot_ext *shot_ext, enum aa_awbmode awbMode)
{
    shot_ext->shot.ctl.aa.awbMode = awbMode;
}

void getMetaCtlAwbMode(struct camera2_shot_ext *shot_ext, enum aa_awbmode *awbMode)
{
    *awbMode = shot_ext->shot.ctl.aa.awbMode;
}
#if defined(USE_HAL3_2_METADATA_INTERFACE)
void setMetaCtlAwbLock(struct camera2_shot_ext *shot_ext, bool lock)
{
    if (lock == true)
        shot_ext->shot.ctl.aa.awbLock = AA_AWB_LOCK_ON;
    else
        shot_ext->shot.ctl.aa.awbLock = AA_AWB_LOCK_OFF;
}

void getMetaCtlAwbLock(struct camera2_shot_ext *shot_ext, bool *lock)
{
    if (shot_ext->shot.ctl.aa.awbLock == AA_AWB_LOCK_OFF)
        *lock = false;
    else
        *lock = true;
}

void setMetaVtMode(struct camera2_shot_ext *shot_ext, enum camera_vt_mode mode)
{
    shot_ext->shot.uctl.vtMode = mode;
}

void setMetaVideoMode(struct camera2_shot_ext *shot_ext, enum aa_videomode mode)
{
    shot_ext->shot.ctl.aa.vendor_videoMode = mode;
}
#endif

void setMetaCtlAfRegion(struct camera2_shot_ext *shot_ext,
                        int x, int y, int w, int h, int weight)
{
    shot_ext->shot.ctl.aa.afRegions[0] = x;
    shot_ext->shot.ctl.aa.afRegions[1] = y;
    shot_ext->shot.ctl.aa.afRegions[2] = w;
    shot_ext->shot.ctl.aa.afRegions[3] = h;
    shot_ext->shot.ctl.aa.afRegions[4] = weight;
}

void getMetaCtlAfRegion(struct camera2_shot_ext *shot_ext,
                        int *x, int *y, int *w, int *h, int *weight)
{
    *x = shot_ext->shot.ctl.aa.afRegions[0];
    *y = shot_ext->shot.ctl.aa.afRegions[1];
    *w = shot_ext->shot.ctl.aa.afRegions[2];
    *h = shot_ext->shot.ctl.aa.afRegions[3];
    *weight = shot_ext->shot.ctl.aa.afRegions[4];
}

void setMetaCtlColorCorrectionMode(struct camera2_shot_ext *shot_ext, enum colorcorrection_mode mode)
{
    shot_ext->shot.ctl.color.mode = mode;
}

void getMetaCtlColorCorrectionMode(struct camera2_shot_ext *shot_ext, enum colorcorrection_mode *mode)
{
    *mode = shot_ext->shot.ctl.color.mode;
}
#if defined(USE_HAL3_2_METADATA_INTERFACE)
void setMetaCtlAaEffect(struct camera2_shot_ext *shot_ext, aa_effect_mode_t mode)
{
    shot_ext->shot.ctl.aa.effectMode = mode;
}

void getMetaCtlAaEffect(struct camera2_shot_ext *shot_ext, aa_effect_mode_t *mode)
{
    *mode = shot_ext->shot.ctl.aa.effectMode;
}
#endif
void setMetaCtlBrightness(struct camera2_shot_ext *shot_ext, int32_t brightness)
{
#if defined(USE_HAL3_2_METADATA_INTERFACE)
    shot_ext->shot.ctl.color.vendor_brightness = brightness;
#else
    shot_ext->shot.ctl.color.brightness = brightness;
#endif
}

void getMetaCtlBrightness(struct camera2_shot_ext *shot_ext, int32_t *brightness)
{
#if defined(USE_HAL3_2_METADATA_INTERFACE)
    *brightness = shot_ext->shot.ctl.color.vendor_brightness;
#else
    *brightness = shot_ext->shot.ctl.color.brightness;
#endif
}

void setMetaCtlSaturation(struct camera2_shot_ext *shot_ext, int32_t saturation)
{
#if defined(USE_HAL3_2_METADATA_INTERFACE)
    shot_ext->shot.ctl.color.vendor_saturation = saturation;
#else
    shot_ext->shot.ctl.color.saturation = saturation;
#endif
}

void getMetaCtlSaturation(struct camera2_shot_ext *shot_ext, int32_t *saturation)
{
#if defined(USE_HAL3_2_METADATA_INTERFACE)
    *saturation = shot_ext->shot.ctl.color.vendor_saturation;
#else
    *saturation = shot_ext->shot.ctl.color.saturation;
#endif
}

void setMetaCtlHue(struct camera2_shot_ext *shot_ext, int32_t hue)
{
#if defined(USE_HAL3_2_METADATA_INTERFACE)
    shot_ext->shot.ctl.color.vendor_hue = hue;
#else
    shot_ext->shot.ctl.color.hue = hue;
#endif
}

void getMetaCtlHue(struct camera2_shot_ext *shot_ext, int32_t *hue)
{
#if defined(USE_HAL3_2_METADATA_INTERFACE)
    *hue = shot_ext->shot.ctl.color.vendor_hue;
#else
    *hue = shot_ext->shot.ctl.color.hue;
#endif
}

void setMetaCtlContrast(struct camera2_shot_ext *shot_ext, uint32_t contrast)
{
#if defined(USE_HAL3_2_METADATA_INTERFACE)
    shot_ext->shot.ctl.color.vendor_contrast = contrast;
#else
    shot_ext->shot.ctl.color.contrast = contrast;
#endif
}

void getMetaCtlContrast(struct camera2_shot_ext *shot_ext, uint32_t *contrast)
{
#if defined(USE_HAL3_2_METADATA_INTERFACE)
    *contrast = shot_ext->shot.ctl.color.vendor_contrast;
#else
    *contrast = shot_ext->shot.ctl.color.contrast;
#endif
}

void setMetaCtlSharpness(struct camera2_shot_ext *shot_ext, enum processing_mode edge_mode, int32_t edge_sharpness,
                         enum processing_mode noise_mode, int32_t noise_sharpness)
{
    shot_ext->shot.ctl.edge.mode = edge_mode;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
    shot_ext->shot.ctl.edge.strength = (uint8_t) edge_sharpness;
#else
    shot_ext->shot.ctl.edge.strength = edge_sharpness;
#endif
    shot_ext->shot.ctl.noise.mode = noise_mode;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
    shot_ext->shot.ctl.noise.strength = (uint8_t) noise_sharpness;
#else
    shot_ext->shot.ctl.noise.strength = noise_sharpness;
#endif
}

void getMetaCtlSharpness(struct camera2_shot_ext *shot_ext, enum processing_mode *edge_mode, int32_t *edge_sharpness,
                         enum processing_mode *noise_mode, int32_t *noise_sharpness)
{
    *edge_mode = shot_ext->shot.ctl.edge.mode;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
    *edge_sharpness = (int32_t) shot_ext->shot.ctl.edge.strength;
#else
    *edge_sharpness = shot_ext->shot.ctl.edge.strength;
#endif
    *noise_mode = shot_ext->shot.ctl.noise.mode;
#if defined(USE_HAL3_2_METADATA_INTERFACE)
    *noise_sharpness = (int32_t) shot_ext->shot.ctl.noise.strength;
#else
    *noise_sharpness = shot_ext->shot.ctl.noise.strength;
#endif
}

#ifdef SAMSUNG_COMPANION
void setMetaCtlRTDrc(struct camera2_shot_ext *shot_ext, enum companion_drc_mode mode)
{
    shot_ext->shot.uctl.companionUd.drc_mode = mode;
}

void getMetaCtlRTDrc(struct camera2_shot_ext *shot_ext, enum companion_drc_mode *mode)
{
    *mode = shot_ext->shot.uctl.companionUd.drc_mode;
}

void setMetaCtlPaf(struct camera2_shot_ext *shot_ext, enum companion_paf_mode mode)
{
    shot_ext->shot.uctl.companionUd.paf_mode = mode;
}

void getMetaCtlPaf(struct camera2_shot_ext *shot_ext, enum companion_paf_mode *mode)
{
    *mode = shot_ext->shot.uctl.companionUd.paf_mode;
}

void setMetaCtlRTHdr(struct camera2_shot_ext *shot_ext, enum companion_wdr_mode mode)
{
    shot_ext->shot.uctl.companionUd.wdr_mode = mode;
}

void getMetaCtlRTHdr(struct camera2_shot_ext *shot_ext, enum companion_wdr_mode *mode)
{
    *mode = shot_ext->shot.uctl.companionUd.wdr_mode;
}
#endif

#ifdef SAMSUNG_OIS
void setMetaCtlOIS(struct camera2_shot_ext *shot_ext, enum optical_stabilization_mode mode)
{
    shot_ext->shot.ctl.lens.opticalStabilizationMode = mode;
}

void getMetaCtlOIS(struct camera2_shot_ext *shot_ext, enum optical_stabilization_mode *mode)
{
    *mode = shot_ext->shot.ctl.lens.opticalStabilizationMode;
}
#endif

#ifdef SAMSUNG_MANUAL_FOCUS
void setMetaCtlFocusDistance(struct camera2_shot_ext *shot_ext, float distance)
{
    shot_ext->shot.ctl.lens.focusDistance = distance;
}

void getMetaCtlFocusDistance(struct camera2_shot_ext *shot_ext, float *distance)
{
    *distance = shot_ext->shot.ctl.lens.focusDistance;
}
#endif

void setMetaCtlIso(struct camera2_shot_ext *shot_ext, enum aa_isomode mode, uint32_t iso)
{
#if defined(USE_HAL3_2_METADATA_INTERFACE)
    shot_ext->shot.ctl.aa.vendor_isoMode = mode;
    shot_ext->shot.ctl.aa.vendor_isoValue = iso;
    shot_ext->shot.ctl.sensor.sensitivity = iso;
#else
    shot_ext->shot.ctl.aa.isoMode = mode;
    shot_ext->shot.ctl.aa.isoValue = iso;
#endif
}

void getMetaCtlIso(struct camera2_shot_ext *shot_ext, enum aa_isomode *mode, uint32_t *iso)
{
#if defined(USE_HAL3_2_METADATA_INTERFACE)
    *mode = shot_ext->shot.ctl.aa.vendor_isoMode;
    *iso = shot_ext->shot.ctl.aa.vendor_isoValue;
#else
    *mode = shot_ext->shot.ctl.aa.isoMode;
    *iso = shot_ext->shot.ctl.aa.isoValue;
#endif
}

void setMetaCtlFdMode(struct camera2_shot_ext *shot_ext, enum facedetect_mode mode)
{
    shot_ext->shot.ctl.stats.faceDetectMode = mode;
}

void getStreamFrameValid(struct camera2_stream *shot_stream, uint32_t *fvalid)
{
    *fvalid = shot_stream->fvalid;
}

void getStreamFrameCount(struct camera2_stream *shot_stream, uint32_t *fcount)
{
    *fcount = shot_stream->fcount;
}

status_t setMetaDmSensorTimeStamp(struct camera2_shot_ext *shot_ext, uint64_t timeStamp)
{
    status_t status = NO_ERROR;
    if (shot_ext == NULL) {
        ALOGE("ERR(%s[%d]):buffer is NULL", __FUNCTION__, __LINE__);
        status = INVALID_OPERATION;
        return status;
    }

    shot_ext->shot.dm.sensor.timeStamp = timeStamp;
    return status;
}

nsecs_t getMetaDmSensorTimeStamp(struct camera2_shot_ext *shot_ext)
{
    if (shot_ext == NULL) {
        ALOGE("ERR(%s[%d]):buffer is NULL", __FUNCTION__, __LINE__);
        return 0;
    }
    return shot_ext->shot.dm.sensor.timeStamp;
}

#ifdef SAMSUNG_TIMESTAMP_BOOT
nsecs_t getMetaUdmSensorTimeStampBoot(struct camera2_shot_ext *shot_ext)
{
    if (shot_ext == NULL) {
        ALOGE("ERR(%s[%d]):buffer is NULL", __FUNCTION__, __LINE__);
        return 0;
    }
    return shot_ext->shot.udm.sensor.timeStampBoot;
}
#endif

void setMetaNodeLeaderRequest(struct camera2_shot_ext* shot_ext, int value)
{
    shot_ext->node_group.leader.request = value;

    ALOGV("INFO(%s[%d]):(%d)", __FUNCTION__, __LINE__,
        shot_ext->node_group.leader.request);
}

void setMetaNodeLeaderVideoID(struct camera2_shot_ext* shot_ext, int value)
{
    shot_ext->node_group.leader.vid = value;

    ALOGV("INFO(%s[%d]):(%d)", __FUNCTION__, __LINE__,
    shot_ext->node_group.leader.vid);
}

void setMetaNodeLeaderInputSize(struct camera2_shot_ext* shot_ext, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    shot_ext->node_group.leader.input.cropRegion[0] = x;
    shot_ext->node_group.leader.input.cropRegion[1] = y;
    shot_ext->node_group.leader.input.cropRegion[2] = w;
    shot_ext->node_group.leader.input.cropRegion[3] = h;

    ALOGV("INFO(%s[%d]):(%d, %d, %d, %d)", __FUNCTION__, __LINE__,
        shot_ext->node_group.leader.input.cropRegion[0],
        shot_ext->node_group.leader.input.cropRegion[1],
        shot_ext->node_group.leader.input.cropRegion[2],
        shot_ext->node_group.leader.input.cropRegion[3]);
}

void setMetaNodeLeaderOutputSize(struct camera2_shot_ext * shot_ext, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    shot_ext->node_group.leader.output.cropRegion[0] = x;
    shot_ext->node_group.leader.output.cropRegion[1] = y;
    shot_ext->node_group.leader.output.cropRegion[2] = w;
    shot_ext->node_group.leader.output.cropRegion[3] = h;

    ALOGV("INFO(%s[%d]):(%d, %d, %d, %d)", __FUNCTION__, __LINE__,
        shot_ext->node_group.leader.output.cropRegion[0],
        shot_ext->node_group.leader.output.cropRegion[1],
        shot_ext->node_group.leader.output.cropRegion[2],
        shot_ext->node_group.leader.output.cropRegion[3]);
}

void setMetaNodeCaptureRequest(struct camera2_shot_ext* shot_ext, int index, int value)
{
    shot_ext->node_group.capture[index].request = value;

    ALOGV("INFO(%s[%d]):(%d)(%d)", __FUNCTION__, __LINE__,
        index,
        shot_ext->node_group.capture[index].request);
}

void setMetaNodeCaptureVideoID(struct camera2_shot_ext* shot_ext, int index, int value)
{
    shot_ext->node_group.capture[index].vid = value;

    ALOGV("INFO(%s[%d]):(%d)(%d)", __FUNCTION__, __LINE__,
        index,
        shot_ext->node_group.capture[index].vid);
}

void setMetaNodeCaptureInputSize(struct camera2_shot_ext* shot_ext, int index, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    shot_ext->node_group.capture[index].input.cropRegion[0] = x;
    shot_ext->node_group.capture[index].input.cropRegion[1] = y;
    shot_ext->node_group.capture[index].input.cropRegion[2] = w;
    shot_ext->node_group.capture[index].input.cropRegion[3] = h;

    ALOGV("INFO(%s[%d]):(%d)(%d, %d, %d, %d)", __FUNCTION__, __LINE__,
        index,
        shot_ext->node_group.capture[index].input.cropRegion[0],
        shot_ext->node_group.capture[index].input.cropRegion[1],
        shot_ext->node_group.capture[index].input.cropRegion[2],
        shot_ext->node_group.capture[index].input.cropRegion[3]);
}

void setMetaNodeCaptureOutputSize(struct camera2_shot_ext * shot_ext, int index, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    shot_ext->node_group.capture[index].output.cropRegion[0] = x;
    shot_ext->node_group.capture[index].output.cropRegion[1] = y;
    shot_ext->node_group.capture[index].output.cropRegion[2] = w;
    shot_ext->node_group.capture[index].output.cropRegion[3] = h;

    ALOGV("INFO(%s[%d]):(%d)(%d, %d, %d, %d)", __FUNCTION__, __LINE__,
        index,
        shot_ext->node_group.capture[index].output.cropRegion[0],
        shot_ext->node_group.capture[index].output.cropRegion[1],
        shot_ext->node_group.capture[index].output.cropRegion[2],
        shot_ext->node_group.capture[index].output.cropRegion[3]);
}

void setMetaBypassDrc(struct camera2_shot_ext *shot_ext, int value)
{
    shot_ext->drc_bypass = value;
}

void setMetaBypassDis(struct camera2_shot_ext *shot_ext, int value)
{
    shot_ext->dis_bypass = value;
}

void setMetaBypassDnr(struct camera2_shot_ext *shot_ext, int value)
{
    shot_ext->dnr_bypass = value;
}

void setMetaBypassFd(struct camera2_shot_ext *shot_ext, int value)
{
    shot_ext->fd_bypass = value;
}

void setMetaSetfile(struct camera2_shot_ext *shot_ext, int value)
{
    shot_ext->setfile = value;
}

#ifdef SAMSUNG_DOF
void setMetaCtlLensPos(struct camera2_shot_ext *shot_ext, int value)
{
    shot_ext->shot.uctl.lensUd.pos = value;
    shot_ext->shot.uctl.lensUd.posSize = 10;
    shot_ext->shot.uctl.lensUd.direction = 0;
    shot_ext->shot.uctl.lensUd.slewRate = 0;

    ALOGD("[DOF][%s][%d] lens pos : %d", __func__, __LINE__, value);
}
#endif

#ifdef SAMSUNG_HRM
void setMetaCtlHRM(struct camera2_shot_ext *shot_ext, int ir_data, int status)
{
    shot_ext->shot.uctl.aaUd.hrmInfo.ir_data = ir_data;
    shot_ext->shot.uctl.aaUd.hrmInfo.status = status;
}
#endif
#ifdef SAMSUNG_LIGHT_IR
void setMetaCtlLight_IR(struct camera2_shot_ext *shot_ext, SensorListenerEvent_t data)
{
    shot_ext->shot.uctl.aaUd.illuminationInfo.visible_cdata = data.light_ir.light_white;
    shot_ext->shot.uctl.aaUd.illuminationInfo.visible_rdata = data.light_ir.light_red;
    shot_ext->shot.uctl.aaUd.illuminationInfo.visible_gdata = data.light_ir.light_green;
    shot_ext->shot.uctl.aaUd.illuminationInfo.visible_bdata = data.light_ir.light_blue;
    shot_ext->shot.uctl.aaUd.illuminationInfo.visible_gain = data.light_ir.ir_again;
    shot_ext->shot.uctl.aaUd.illuminationInfo.visible_exptime = data.light_ir.ir_atime;
    shot_ext->shot.uctl.aaUd.illuminationInfo.ir_north = data.light_ir.ir_data;
    shot_ext->shot.uctl.aaUd.illuminationInfo.ir_south = data.light_ir.ir_data;
    shot_ext->shot.uctl.aaUd.illuminationInfo.ir_east = data.light_ir.ir_data;
    shot_ext->shot.uctl.aaUd.illuminationInfo.ir_west = data.light_ir.ir_data;
    shot_ext->shot.uctl.aaUd.illuminationInfo.ir_gain = data.light_ir.ir_again;
    shot_ext->shot.uctl.aaUd.illuminationInfo.ir_exptime = data.light_ir.ir_atime;
}
#endif
#ifdef SAMSUNG_GYRO
void setMetaCtlGyro(struct camera2_shot_ext *shot_ext, SensorListenerEvent_t data)
{
    shot_ext->shot.uctl.aaUd.gyroInfo.x = data.gyro.x;
    shot_ext->shot.uctl.aaUd.gyroInfo.y = data.gyro.y;
    shot_ext->shot.uctl.aaUd.gyroInfo.z = data.gyro.z;
}
#endif

#ifdef USE_FW_ZOOMRATIO
void setMetaCtlZoom(struct camera2_shot_ext *shot_ext, float data)
{
    shot_ext->shot.uctl.zoomRatio = data;
}
#endif

#ifdef SAMSUNG_OIS_VDIS
void setMetaCtlOISCoef(struct camera2_shot_ext *shot_ext, uint32_t data)
{
    shot_ext->shot.uctl.lensUd.oisCoefVal = data;
}
#endif

int mergeSetfileYuvRange(int setfile, int yuvRange)
{
    int ret = setfile;

    ret &= (0x0000ffff);
    ret |= (yuvRange << 16);

    return ret;
}

int getPlaneSizeFlite(int width, int height)
{
    int PlaneSize;
    int Alligned_Width;
    int Bytes;

    Alligned_Width = (width + 9) / 10 * 10;
    Bytes = Alligned_Width * 8 / 5 ;

    PlaneSize = Bytes * height;

    return PlaneSize;
}

bool dumpToFile(char *filename, char *srcBuf, unsigned int size)
{
    FILE *yuvFd = NULL;
    char *buffer = NULL;

    yuvFd = fopen(filename, "w+");

    if (yuvFd == NULL) {
        ALOGE("ERR(%s):open(%s) fail",
            __func__, filename);
        return false;
    }

    buffer = (char *)malloc(size);

    if (buffer == NULL) {
        ALOGE("ERR(%s):malloc file", __func__);
        fclose(yuvFd);
        return false;
    }

    memcpy(buffer, srcBuf, size);

    fflush(stdout);

    fwrite(buffer, 1, size, yuvFd);

    fflush(yuvFd);

    if (yuvFd)
        fclose(yuvFd);
    if (buffer)
        free(buffer);

    ALOGD("DEBUG(%s):filedump(%s, size(%d) is successed!!",
            __func__, filename, size);

    return true;
}

status_t getYuvPlaneSize(int format, unsigned int *size,
                      unsigned int width, unsigned int height)
{
    unsigned int frame_ratio = 1;
    unsigned int frame_size = width * height;
    unsigned int src_bpp = 0;
    unsigned int src_planes = 0;

    if (getYuvFormatInfo(format, &src_bpp, &src_planes) < 0){
        ALOGE("ERR(%s[%d]): invalid format(%x)", __FUNCTION__, __LINE__, format);
        return BAD_VALUE;
    }

    src_planes = (src_planes == 0) ? 1 : src_planes;
    frame_ratio = 8 * (src_planes -1) / (src_bpp - 8);

    switch (src_planes) {
    case 1:
        switch (format) {
        case V4L2_PIX_FMT_BGR32:
        case V4L2_PIX_FMT_RGB32:
            size[0] = frame_size << 2;
            break;
        case V4L2_PIX_FMT_RGB565X:
        case V4L2_PIX_FMT_NV16:
        case V4L2_PIX_FMT_NV61:
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_UYVY:
        case V4L2_PIX_FMT_VYUY:
        case V4L2_PIX_FMT_YVYU:
            size[0] = frame_size << 1;
            break;
        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV21:
        case V4L2_PIX_FMT_NV21M:
            size[0] = (frame_size * 3) >> 1;
            break;
        case V4L2_PIX_FMT_YVU420:
            size[0] = frame_size + (ALIGN((width >> 1), 16) * ((height >> 1) * 2));
            break;
        default:
            ALOGE("%s::invalid color type", __func__);
            return false;
            break;
        }
        size[1] = 0;
        size[2] = 0;
        break;
    case 2:
        size[0] = frame_size;
        size[1] = frame_size / frame_ratio;
        size[2] = 0;
        break;
    case 3:
        size[0] = frame_size;
        size[1] = frame_size / frame_ratio;
        size[2] = frame_size / frame_ratio;
        break;
    default:
        ALOGE("%s::invalid color foarmt", __func__);
        return false;
        break;
    }

    return NO_ERROR;
}

status_t getYuvFormatInfo(unsigned int v4l2_pixel_format,
                          unsigned int *bpp, unsigned int *planes)
{
    switch (v4l2_pixel_format) {
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV21:
        *bpp    = 12;
        *planes = 1;
        break;
    case V4L2_PIX_FMT_NV12M:
    case V4L2_PIX_FMT_NV21M:
    case V4L2_PIX_FMT_NV12MT:
    case V4L2_PIX_FMT_NV12MT_16X16:
        *bpp    = 12;
        *planes = 2;
        break;
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YVU420:
        *bpp    = 12;
        *planes = 1;
        break;
    case V4L2_PIX_FMT_YUV420M:
    case V4L2_PIX_FMT_YVU420M:
        *bpp    = 12;
        *planes = 3;
        break;
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_VYUY:
        *bpp    = 16;
        *planes = 1;
        break;
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_NV61:
        *bpp    = 16;
        *planes = 2;
        break;
    case V4L2_PIX_FMT_YUV422P:
        *bpp    = 16;
        *planes = 3;
        break;
    default:
        return BAD_VALUE;
        break;
    }

    return NO_ERROR;
}

int getYuvPlaneCount(unsigned int v4l2_pixel_format)
{
    int ret = 0;
    unsigned int bpp = 0;
    unsigned int planeCnt = 0;

    ret = getYuvFormatInfo(v4l2_pixel_format, &bpp, &planeCnt);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]): BAD_VALUE", __FUNCTION__, __LINE__);
        return -1;
    }

    return planeCnt;
}

int displayExynosBuffer( ExynosCameraBuffer *buffer) {
        ALOGD("-----------------------------------------------");
        ALOGD(" buffer.index = %d ", buffer->index);
        ALOGD(" buffer.planeCount = %d ", buffer->planeCount);
        for(int i = 0 ; i < buffer->planeCount ; i++ ) {
            ALOGD(" buffer.fd[%d] = %d ", i, buffer->fd[i]);
            ALOGD(" buffer.size[%d] = %d ", i, buffer->size[i]);
            ALOGD(" buffer.addr[%d] = %p ", i, buffer->addr[i]);
        }
        ALOGD("-----------------------------------------------");
        return 0;
}

#ifdef SENSOR_NAME_GET_FROM_FILE
int getSensorIdFromFile(int camId)
{
    FILE *fp = NULL;
    int numread = -1;
    char sensor_name[50];
    int sensorName = -1;
    bool ret = true;

    if (camId == CAMERA_ID_BACK) {
        fp = fopen(SENSOR_NAME_PATH_BACK, "r");
        if (fp == NULL) {
            ALOGE("ERR(%s[%d]):failed to open sysfs entry", __FUNCTION__, __LINE__);
            goto err;
        }
    } else {
        fp = fopen(SENSOR_NAME_PATH_FRONT, "r");
        if (fp == NULL) {
            ALOGE("ERR(%s[%d]):failed to open sysfs entry", __FUNCTION__, __LINE__);
            goto err;
        }
    }

    if (fgets(sensor_name, sizeof(sensor_name), fp) == NULL) {
        ALOGE("ERR(%s[%d]):failed to read sysfs entry", __FUNCTION__, __LINE__);
	    goto err;
    }

    numread = strlen(sensor_name);
    ALOGD("DEBUG(%s[%d]):Sensor name is %s(%d)", __FUNCTION__, __LINE__, sensor_name, numread);

    /* TODO: strncmp for check sensor name, str is vendor specific sensor name
     * ex)
     *    if (strncmp((const char*)sensor_name, "str", numread - 1) == 0) {
     *        sensorName = SENSOR_NAME_IMX135;
     *    }
     */
    sensorName = atoi(sensor_name);

err:
    if (fp != NULL)
        fclose(fp);

    return sensorName;
}
#endif

#ifdef SENSOR_FW_GET_FROM_FILE
const char *getSensorFWFromFile(struct ExynosSensorInfoBase *info, int camId)
{
    FILE *fp = NULL;
    int numread = -1;

    if (camId == CAMERA_ID_BACK) {
        fp = fopen(SENSOR_FW_PATH_BACK, "r");
        if (fp == NULL) {
            ALOGE("ERR(%s[%d]):failed to open sysfs entry", __FUNCTION__, __LINE__);
            goto err;
        }
    } else {
        fp = fopen(SENSOR_FW_PATH_FRONT, "r");
        if (fp == NULL) {
            ALOGE("ERR(%s[%d]):failed to open sysfs entry", __FUNCTION__, __LINE__);
            goto err;
        }
    }
    if (fgets(info->sensor_fw, sizeof(info->sensor_fw), fp) == NULL) {
        ALOGE("ERR(%s[%d]):failed to read sysfs entry", __FUNCTION__, __LINE__);
	    goto err;
    }

    numread = strlen(info->sensor_fw);
    ALOGD("DEBUG(%s[%d]):Sensor fw is %s(%d)", __FUNCTION__, __LINE__, info->sensor_fw, numread);

err:
    if (fp != NULL)
        fclose(fp);

    return (const char *)info->sensor_fw;
}
#endif

#ifdef SAMSUNG_OIS
char *getOisEXIFFromFile(struct ExynosSensorInfoBase *info, int mode)
{
    FILE *fp = NULL;
    char ois_mode[5] = {0, };
    char ois_data[OIS_EXIF_SIZE] = {0, };

    fp = fopen(OIS_EXIF_PATH_BACK, "r");
    if (fp == NULL) {
        ALOGE("ERR(%s[%d]):failed to open sysfs entry", __FUNCTION__, __LINE__);
        goto err;
    }

    /* ois tag */
    memset(info->ois_exif_info.ois_exif, 0, OIS_EXIF_SIZE);
    memcpy(info->ois_exif_info.ois_exif, OIS_EXIF_TAG, sizeof(OIS_EXIF_TAG));

    if (fgets(ois_data, sizeof(ois_data), fp) == NULL) {
        ALOGE("ERR(%s[%d]):failed to read sysfs entry", __FUNCTION__, __LINE__);
	    goto err;
    }

    /* ois data */
    sprintf(ois_mode, "%d\n", mode);
    strncat(ois_data, " ", 1);
    strncat(ois_data, ois_mode, sizeof(ois_mode));
    strncat(info->ois_exif_info.ois_exif, ois_data, OIS_EXIF_SIZE - (sizeof(OIS_EXIF_TAG) + 1));

    ALOGD("DEBUG(%s[%d]):ois exif data : %s", __FUNCTION__, __LINE__, info->ois_exif_info.ois_exif);

err:
    if (fp != NULL)
        fclose(fp);

    return info->ois_exif_info.ois_exif;
}
#endif

int checkBit(unsigned int *target, int index)
{
    int ret = 0;
    if (*target & (1 << index)) {
        ret = 1;
    } else {
        ret = 0;
    }
    return ret;
}

void clearBit(unsigned int *target, int index, bool isStatePrint)
{
    *target = *target &~ (1 << index);

    if (isStatePrint)
        ALOGD("INFO(%s[%d]):(0x%x)", __FUNCTION__, __LINE__, *target);
}

void setBit(unsigned int *target, int index, bool isStatePrint)
{
    *target = *target | (1 << index);

    if (isStatePrint)
        ALOGD("INFO(%s[%d]):(0x%x)", __FUNCTION__, __LINE__, *target);
}

void resetBit(unsigned int *target, int value, bool isStatePrint)
{
    *target = value;

    if (isStatePrint)
        ALOGD("INFO(%s[%d]):(0x%x)", __FUNCTION__, __LINE__, *target);
}

void checkAndroidVersion(void) {
    char value[PROPERTY_VALUE_MAX] = {0};
    char targetAndroidVersion[PROPERTY_VALUE_MAX] = {0};

    snprintf(targetAndroidVersion, PROPERTY_VALUE_MAX, "%d.%d", TARGET_ANDROID_VER_MAJ, TARGET_ANDROID_VER_MIN);

    property_get("ro.build.version.release", value, "0");

    if (strncmp(targetAndroidVersion, value, PROPERTY_VALUE_MAX))
        ALOGD("DEBUG(%s[%d]): Tartget Android version (%s), build version (%s)",
                __FUNCTION__, __LINE__, targetAndroidVersion, value);
    else
        ALOGI("Andorid build version release %s", value);
}

bool isOwnScc(int cameraId)
{
    bool ret = false;

    if (cameraId == CAMERA_ID_BACK) {
#ifdef MAIN_CAMERA_HAS_OWN_SCC
        ret = MAIN_CAMERA_HAS_OWN_SCC;
#else
        ALOGW("WRN(%s[%d]): MAIN_CAMERA_HAS_OWN_SCC is not defined", __FUNCTION__, __LINE__);
#endif
    } else {
#ifdef FRONT_CAMERA_HAS_OWN_SCC
        ret = FRONT_CAMERA_HAS_OWN_SCC;
#else
        ALOGW("WRN(%s[%d]): FRONT_CAMERA_HAS_OWN_SCC is not defined", __FUNCTION__, __LINE__);
#endif
    }

    return ret;
}

int checkPropertyForShotMode(int newShotMode, __unused int vtMode)
{
    int shotMode = newShotMode;
    char propertyValue[PROPERTY_VALUE_MAX];

#ifdef USE_LIMITATION_FOR_THIRD_PARTY
    property_get("sys.cameramode.blackbox", propertyValue, "0");
    if (strcmp(propertyValue, "1") == 0) {
        shotMode = THIRD_PARTY_BLACKBOX_MODE;
    } else {
        property_get("sys.hangouts.fps", propertyValue, "0");
        int newHangOutFPS = atoi(propertyValue);
        if (newHangOutFPS > 0) {
            shotMode = THIRD_PARTY_HANGOUT_MODE;
        } else {
            if ((vtMode <= 0) || (vtMode > 2)) {
                property_get("sys.cameramode.vtcall", propertyValue, "0");
                if (strcmp(propertyValue, "1") == 0) {
                    shotMode = THIRD_PARTY_VTCALL_MODE;
                }
            }
        }
    }
#endif
    return shotMode;
}

bool checkProperty(bool check_cc)
{
    bool ret = false;
    char propertyValue[PROPERTY_VALUE_MAX];

    if (check_cc) {
        property_get("system.camera.CC.disable", propertyValue, "0");
        if (strcmp(propertyValue, "1") == 0) {
            return true;
        } else {
            property_get("system.camera.CC.fac.disable", propertyValue, "0");
            if (strcmp(propertyValue, "1") == 0) {
                property_set("system.camera.CC.fac.disable", "0");
                return true;
            }
        }
    }

    property_get("sys.cameramode.blackbox", propertyValue, "0");
    if (strcmp(propertyValue, "1") == 0) {
        ret = true;
    } else {
        int newHangOutFPS = 0;
        property_get("sys.hangouts.fps", propertyValue, "0");
        newHangOutFPS = atoi(propertyValue);
        if (newHangOutFPS > 0) {
            ret = true;
        } else {
            property_get("sys.cameramode.vtcall", propertyValue, "0");
            if (strcmp(propertyValue, "1") == 0) {
                ret = true;
            }
        }
    }
    return ret;
}

bool isCompanion(int cameraId)
{
    bool ret = false;

    if (cameraId == CAMERA_ID_BACK) {
#ifdef MAIN_CAMERA_USE_SAMSUNG_COMPANION
        ret = MAIN_CAMERA_USE_SAMSUNG_COMPANION;
#else
        ALOGI("INFO(%s[%d]): MAIN_CAMERA_USE_SAMSUNG_COMPANION is not defined", __FUNCTION__, __LINE__);
#endif
    } else {
#ifdef FRONT_CAMERA_USE_SAMSUNG_COMPANION
        if (FRONT_CAMERA_USE_SAMSUNG_COMPANION) {
            if (checkProperty(true) == true) {
                ret = false; /* not use */
            } else {
                ret = true; /* use */
            }
        }
#else
        ALOGI("INFO(%s[%d]): FRONT_CAMERA_USE_SAMSUNG_COMPANION is not defined", __FUNCTION__, __LINE__);
#endif
    }

    return ret;
}

bool isEEprom(int cameraId)
{
    bool ret = false;

    if (cameraId == CAMERA_ID_BACK) {
#ifdef SAMSUNG_EEPROM_REAR
        ret = SAMSUNG_EEPROM_REAR;
#else
        ALOGI("INFO(%s[%d]): SAMSUNG_EEPROM_REAR is not defined", __FUNCTION__, __LINE__);
#endif
    } else {
#ifdef SAMSUNG_EEPROM_FRONT
        ret = SAMSUNG_EEPROM_FRONT;
#else
        ALOGI("INFO(%s[%d]): SAMSUNG_EEPROM_FRONT is not defined", __FUNCTION__, __LINE__);
#endif
    }

    return ret;
}

bool isFastenAeStable(int cameraId, bool useCompanion)
{
    bool ret = false;

    if (cameraId == CAMERA_ID_BACK) {
#ifdef USE_FASTEN_AE_STABLE
        ret = USE_FASTEN_AE_STABLE;
#else
        ALOGI("INFO(%s[%d]): USE_FASTEN_AE_STABLE is not defined", __FUNCTION__, __LINE__);
#endif
    } else {
#ifdef USE_FASTEN_AE_STABLE_FRONT
        if(useCompanion) {
            ret = USE_FASTEN_AE_STABLE_FRONT;
        } else {
            ret = false;
        }
#else
        ALOGI("INFO(%s[%d]): USE_FASTEN_AE_STABLE_FRONT is not defined", __FUNCTION__, __LINE__);
#endif
    }

    return ret;
}

/* LLS_Deblur Capture */
bool isLDCapture(int cameraId)
{
    bool ret = false;

    if (cameraId == CAMERA_ID_BACK) {
#ifdef SAMSUNG_LDCAPTURE_REAR
        ret = SAMSUNG_LDCAPTURE_REAR;
#else
        ALOGI("INFO(%s[%d]): SAMSUNG_LDCAPTURE_REAR is not defined", __FUNCTION__, __LINE__);
#endif
    } else {
#ifdef SAMSUNG_LDCAPTURE_FRONT
        ret = SAMSUNG_LDCAPTURE_FRONT;
#else
        ALOGI("INFO(%s[%d]): SAMSUNG_LDCAPTURE_FRONT is not defined", __FUNCTION__, __LINE__);
#endif
    }

    return ret;
}

#define SATURATING_ADD(a, b)  ((((a<<4) > (0xFFFF - (b<<4))) ? 0xFFFF : ((a<<4) + (b<<4)))>>4)
status_t m_addBayerBufferByNeon(struct ExynosCameraBuffer *srcBuf,
                                struct ExynosCameraBuffer *dstBuf,
                                unsigned int copySize)
{
    if (srcBuf->addr[0] == NULL) {
        ALOGE("ERR(%s[%d]):srcBuf->addr[0] == NULL",  __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (dstBuf->addr[0] == NULL) {
        ALOGE("ERR(%s[%d]):dstBuf->addr[0] == NULL",  __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    /* bayer is max 16bit, so add by short */
    unsigned short*firstSrcAddr = (unsigned short *)srcBuf->addr[0];
    unsigned short*firstDstAddr = (unsigned short *)dstBuf->addr[0];
    unsigned short*srcAddr = firstSrcAddr;
    unsigned short*dstAddr = firstDstAddr;
    /*
     * loop as copySize / 32 byte
     * 32 byte is perfect align size of cache.
     * 64 byte is not faster than 32byte.
     */
    unsigned int alignByte = 64;
    unsigned int alignShort = 32;
    unsigned int realCopySize = copySize / alignByte;
    unsigned int remainedCopySize = copySize % alignByte;

    ALOGD("DEBUG(%s[%d]):srcAddr(%p), dstAddr(%p), copySize(%d), sizeof(short)(%d),\
        alignByte(%d), realCopySize(%d), remainedCopySize(%d)",
        __FUNCTION__, __LINE__, srcAddr, dstAddr, copySize, (int)sizeof(short), alignByte,
        realCopySize, remainedCopySize);

    unsigned short* src0_ptr, *src1_ptr;
    uint16x8_t src0_u16x8_0;
    uint16x8_t src0_u16x8_1;
    uint16x8_t src0_u16x8_2;
    uint16x8_t src0_u16x8_3;

    src0_ptr = dstAddr;
    src1_ptr = srcAddr;

    for (unsigned int i = 0; i < realCopySize; i++) {
        src0_u16x8_0 = vqaddq_u16(vshlq_n_u16(vld1q_u16((uint16_t*)(src0_ptr)), 4),
                                  vshlq_n_u16(vld1q_u16((uint16_t*)(src1_ptr)), 4));
        src0_u16x8_1 = vqaddq_u16(vshlq_n_u16(vld1q_u16((uint16_t*)(src0_ptr + 8)), 4),
                                  vshlq_n_u16(vld1q_u16((uint16_t*)(src1_ptr + 8)), 4));
        src0_u16x8_2 = vqaddq_u16(vshlq_n_u16(vld1q_u16((uint16_t*)(src0_ptr + 16)), 4),
                                  vshlq_n_u16(vld1q_u16((uint16_t*)(src1_ptr + 16)), 4));
        src0_u16x8_3 = vqaddq_u16(vshlq_n_u16(vld1q_u16((uint16_t*)(src0_ptr + 24)), 4),
                                  vshlq_n_u16(vld1q_u16((uint16_t*)(src1_ptr + 24)), 4));

        vst1q_u16((src0_ptr), vshrq_n_u16(src0_u16x8_0, 4));
        vst1q_u16((src0_ptr + 8), vshrq_n_u16(src0_u16x8_1, 4));
        vst1q_u16((src0_ptr + 16),vshrq_n_u16(src0_u16x8_2, 4));
        vst1q_u16((src0_ptr + 24),vshrq_n_u16(src0_u16x8_3, 4));

        src0_ptr = firstDstAddr + (alignShort * (i + 1));
        src1_ptr = firstSrcAddr + (alignShort * (i + 1));
    }

    for (unsigned int i = 0; i < remainedCopySize; i++) {
        dstAddr[i] = SATURATING_ADD(dstAddr[i], srcAddr[i]);
    }

    return NO_ERROR;
}

status_t m_addBayerBufferByCpu(struct ExynosCameraBuffer *srcBuf,
                               struct ExynosCameraBuffer *dstBuf,
                               unsigned int copySize)
{
    if (srcBuf->addr[0] == NULL) {
        ALOGE("ERR(%s[%d]):srcBuf->addr[0] == NULL",  __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (dstBuf->addr[0] == NULL) {
        ALOGE("ERR(%s[%d]):dstBuf->addr[0] == NULL",  __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    /* bayer is max 16bit, so add by short */
    unsigned short *firstSrcAddr = (unsigned short *)srcBuf->addr[0];
    unsigned short *firstDstAddr = (unsigned short *)dstBuf->addr[0];
    unsigned short *srcAddr = firstSrcAddr;
    unsigned short *dstAddr = firstDstAddr;

    /*
     * loop as copySize / 32 byte
     * 32 byte is perfect align size of cache.
     * 64 byte is not faster than 32byte.
     */
    unsigned int alignByte = 32;
    unsigned int alignShort = 16;
    unsigned int realCopySize     = copySize / alignByte;
    unsigned int remainedCopySize = copySize % alignByte;

    ALOGD("DEBUG(%s[%d]):srcAddr(%p), dstAddr(%p), copySize(%d), sizeof(short)(%d),\
        alignByte(%d), realCopySize(%d), remainedCopySize(%d)",
        __FUNCTION__, __LINE__, srcAddr, dstAddr, copySize, (int)sizeof(short), alignByte,
        realCopySize, remainedCopySize);

    for (unsigned int i = 0; i < realCopySize; i++) {
        dstAddr[0] = SATURATING_ADD(dstAddr[0], srcAddr[0]);
        dstAddr[1] = SATURATING_ADD(dstAddr[1], srcAddr[1]);
        dstAddr[2] = SATURATING_ADD(dstAddr[2], srcAddr[2]);
        dstAddr[3] = SATURATING_ADD(dstAddr[3], srcAddr[3]);
        dstAddr[4] = SATURATING_ADD(dstAddr[4], srcAddr[4]);
        dstAddr[5] = SATURATING_ADD(dstAddr[5], srcAddr[5]);
        dstAddr[6] = SATURATING_ADD(dstAddr[6], srcAddr[6]);
        dstAddr[7] = SATURATING_ADD(dstAddr[7], srcAddr[7]);
        dstAddr[8] = SATURATING_ADD(dstAddr[8], srcAddr[8]);
        dstAddr[9] = SATURATING_ADD(dstAddr[9], srcAddr[9]);
        dstAddr[10] = SATURATING_ADD(dstAddr[10], srcAddr[10]);
        dstAddr[11] = SATURATING_ADD(dstAddr[11], srcAddr[11]);
        dstAddr[12] = SATURATING_ADD(dstAddr[12], srcAddr[12]);
        dstAddr[13] = SATURATING_ADD(dstAddr[13], srcAddr[13]);
        dstAddr[14] = SATURATING_ADD(dstAddr[14], srcAddr[14]);
        dstAddr[15] = SATURATING_ADD(dstAddr[15], srcAddr[15]);

        // jump next 32bytes.
        //srcAddr += alignShort;
        //dstAddr += alignShort;
        /* This is faster on compiler lever */
        srcAddr = firstSrcAddr + (alignShort * (i + 1));
        dstAddr = firstDstAddr + (alignShort * (i + 1));
    }

    for (unsigned int i = 0; i < remainedCopySize; i++) {
        dstAddr[i] = SATURATING_ADD(dstAddr[i], srcAddr[i]);
    }

    return NO_ERROR;
}

#define COMBINE(a, b) ((((a<<20)&0xFFF00000)|((b<<8)&0x000FFF00)))
#define COMBINE_P0(a, b) ((((a)&0x00FF)|((b<<8)&0x0F00)))
#define COMBINE_P1(a, b) ((((a>>4)&0x000F)|((b<<4)&0x0FF0)))
#define COMBINE_P3(a, b) ((((a>>8)&0x000F)|((b<<4)&0x00F0)))
#define COMBINE_P4(a, b) ((((a<<8)&0x0F00)|((b)&0x00FF)))
status_t m_addBayerBufferByNeonPacked(struct ExynosCameraBuffer *srcBuf,
                                      struct ExynosCameraBuffer *dstBuf,
                                      ExynosRect *dstRect,
                                      unsigned int copySize)
{
    if (srcBuf->addr[0] == NULL) {
        ALOGE("ERR(%s[%d]):srcBuf->addr[0] == NULL",  __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (dstBuf->addr[0] == NULL) {
        ALOGE("ERR(%s[%d]):dstBuf->addr[0] == NULL",  __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    /* bayer is max 16bit, so add by short */
    unsigned char *firstSrcAddr = (unsigned char *)srcBuf->addr[0];
    unsigned char *firstDstAddr = (unsigned char *)dstBuf->addr[0];
    unsigned char *srcAddr = firstSrcAddr;
    unsigned char *dstAddr = firstDstAddr;

    /*
     *      * loop as copySize / 32 byte
     *      * 32 byte is perfect align size of cache.
     *      * 64 byte is not faster than 32byte.
     *      */

    unsigned int alignByte = 12;
    unsigned int alignShort = 6;
    unsigned int realCopySize = copySize / alignByte;
    unsigned int remainedCopySize = copySize % alignByte;

    uint16x8_t src_u16x8_0;
    uint16x8_t dst_u16x8_0;

    unsigned int width_byte = dstRect->w * 12 / 8;
    width_byte = ALIGN_UP(width_byte, 16);

    ALOGD("DEBUG(%s[%d]):srcAddr(%p), dstAddr(%p), copySize(%d), sizeof(short)(%d),\
        alignByte(%d), realCopySize(%d), remainedCopySize(%d), pixel width(%d), pixel height(%d),\
        16 aligned byte width(%d)",
        __FUNCTION__, __LINE__, srcAddr, dstAddr, copySize, (int)sizeof(short),
        alignByte, realCopySize, remainedCopySize, dstRect->w, dstRect->h, width_byte);


    unsigned char dst_temp[16];
    unsigned short dstPix_0, dstPix_1, dstPix_2, dstPix_3, dstPix_4, dstPix_5, dstPix_6, dstPix_7;
    unsigned short srcPix_0, srcPix_1, srcPix_2, srcPix_3, srcPix_4, srcPix_5, srcPix_6, srcPix_7;
    unsigned int col;

    for (unsigned int row = 0; row < (unsigned int)dstRect->h; row++) {
        for (col = 0; col + alignByte <= width_byte; col += alignByte) {
            dstAddr = firstDstAddr + width_byte * row + col;
            srcAddr = firstSrcAddr + width_byte * row + col;

            unsigned short temp_0 = dstAddr[0];
            unsigned short temp_1 = dstAddr[1];
            unsigned short temp_cmbd = COMBINE_P0(temp_0, temp_1);
            unsigned short temp_2 = dstAddr[2];
            unsigned short temp_cmbd2 = COMBINE_P1(temp_1, temp_2);

            dstPix_0 = temp_cmbd;
            dstPix_1 = temp_cmbd2;

            temp_0 = dstAddr[3];
            temp_1 = dstAddr[4];
            temp_cmbd = COMBINE_P0(temp_0, temp_1);
            temp_2 = dstAddr[5];
            temp_cmbd2 = COMBINE_P1(temp_1, temp_2);

            dstPix_2 = temp_cmbd;
            dstPix_3 = temp_cmbd2;

            temp_0 = dstAddr[6];
            temp_1 = dstAddr[7];
            temp_cmbd = COMBINE_P0(temp_0, temp_1);
            temp_2 = dstAddr[8];
            temp_cmbd2 = COMBINE_P1(temp_1, temp_2);

            dstPix_4 = temp_cmbd;
            dstPix_5 = temp_cmbd2;

            temp_0 = dstAddr[9];
            temp_1 = dstAddr[10];
            temp_cmbd = COMBINE_P0(temp_0, temp_1);
            temp_2 = dstAddr[11];
            temp_cmbd2 = COMBINE_P1(temp_1, temp_2);

            dstPix_6 = temp_cmbd;
            dstPix_7 = temp_cmbd2;

            temp_0 = srcAddr[0];
            temp_1 = srcAddr[1];
            temp_cmbd = COMBINE_P0(temp_0, temp_1);
            temp_2 = srcAddr[2];
            temp_cmbd2 = COMBINE_P1(temp_1, temp_2);

            srcPix_0 = temp_cmbd;
            srcPix_1 = temp_cmbd2;

            temp_0 = srcAddr[3];
            temp_1 = srcAddr[4];
            temp_cmbd = COMBINE_P0(temp_0, temp_1);
            temp_2 = srcAddr[5];
            temp_cmbd2 = COMBINE_P1(temp_1, temp_2);

            srcPix_2 = temp_cmbd;
            srcPix_3 = temp_cmbd2;

            temp_0 = srcAddr[6];
            temp_1 = srcAddr[7];
            temp_cmbd = COMBINE_P0(temp_0, temp_1);
            temp_2 = srcAddr[8];
            temp_cmbd2 = COMBINE_P1(temp_1, temp_2);
            srcPix_4 = temp_cmbd;
            srcPix_5 = temp_cmbd2;

            temp_0 = srcAddr[9];
            temp_1 = srcAddr[10];
            temp_cmbd = COMBINE_P0(temp_0, temp_1);
            temp_2 = srcAddr[11];
            temp_cmbd2 = COMBINE_P1(temp_1, temp_2);
            srcPix_6 = temp_cmbd;
            srcPix_7 = temp_cmbd2;

            src_u16x8_0 =  vsetq_lane_u16(srcPix_0, src_u16x8_0, 0);
            src_u16x8_0 =  vsetq_lane_u16(srcPix_1, src_u16x8_0, 1);
            src_u16x8_0 =  vsetq_lane_u16(srcPix_2, src_u16x8_0, 2);
            src_u16x8_0 =  vsetq_lane_u16(srcPix_3, src_u16x8_0, 3);
            src_u16x8_0 =  vsetq_lane_u16(srcPix_4, src_u16x8_0, 4);
            src_u16x8_0 =  vsetq_lane_u16(srcPix_5, src_u16x8_0, 5);
            src_u16x8_0 =  vsetq_lane_u16(srcPix_6, src_u16x8_0, 6);
            src_u16x8_0 =  vsetq_lane_u16(srcPix_7, src_u16x8_0, 7);

            dst_u16x8_0 =  vsetq_lane_u16(dstPix_0, dst_u16x8_0, 0);
            dst_u16x8_0 =  vsetq_lane_u16(dstPix_1, dst_u16x8_0, 1);
            dst_u16x8_0 =  vsetq_lane_u16(dstPix_2, dst_u16x8_0, 2);
            dst_u16x8_0 =  vsetq_lane_u16(dstPix_3, dst_u16x8_0, 3);
            dst_u16x8_0 =  vsetq_lane_u16(dstPix_4, dst_u16x8_0, 4);
            dst_u16x8_0 =  vsetq_lane_u16(dstPix_5, dst_u16x8_0, 5);
            dst_u16x8_0 =  vsetq_lane_u16(dstPix_6, dst_u16x8_0, 6);
            dst_u16x8_0 =  vsetq_lane_u16(dstPix_7, dst_u16x8_0, 7);

            dst_u16x8_0 = vqaddq_u16(vshlq_n_u16(dst_u16x8_0, 4), vshlq_n_u16(src_u16x8_0, 4));
            dst_u16x8_0 = vshlq_n_u16(vshrq_n_u16(dst_u16x8_0, 6), 2);

            dstPix_0 = vgetq_lane_u16(dst_u16x8_0, 0);
            dstPix_1 = vgetq_lane_u16(dst_u16x8_0, 1);
            dstPix_2 = vgetq_lane_u16(dst_u16x8_0, 2);
            dstPix_3 = vgetq_lane_u16(dst_u16x8_0, 3);
            dstPix_4 = vgetq_lane_u16(dst_u16x8_0, 4);
            dstPix_5 = vgetq_lane_u16(dst_u16x8_0, 5);
            dstPix_6 = vgetq_lane_u16(dst_u16x8_0, 6);
            dstPix_7 = vgetq_lane_u16(dst_u16x8_0, 7);

            dstAddr[0] = (unsigned char)(dstPix_0);
            dstAddr[1] = (unsigned char)((COMBINE_P3(dstPix_0, dstPix_1)));
            dstAddr[2] = (unsigned char)(dstPix_1 >> 4);
            dstAddr[3] = (unsigned char)(dstPix_2);
            dstAddr[4] = (unsigned char)((COMBINE_P3(dstPix_2, dstPix_3)));
            dstAddr[5] = (unsigned char)(dstPix_3 >> 4);
            dstAddr[6] = (unsigned char)(dstPix_4);
            dstAddr[7] = (unsigned char)((COMBINE_P3(dstPix_4, dstPix_5)));
            dstAddr[8] = (unsigned char)(dstPix_5 >> 4);
            dstAddr[9] = (unsigned char)(dstPix_6);
            dstAddr[10] = (unsigned char)((COMBINE_P3(dstPix_6, dstPix_7)));
            dstAddr[11] = (unsigned char)(dstPix_7 >> 4);
        }
    }
#if 0
    /* for the case of pixel width which is not a multiple of 8. The section of codes need to be verified */
    for (unsigned int i = 0; i < remainedCopySize; i += 3) {
        unsigned char temp_0 = dstAddr[0];
        unsigned char temp_1 = dstAddr[1];
        unsigned char temp_cmbd = COMBINE_P0(temp_0, temp_1);
        unsigned char temp_2 = dstAddr[2];
        unsigned char temp_cmbd2 = COMBINE_P1(temp_1, temp_2);
        unsigned char dstPix_0 = temp_cmbd;
        unsigned char dstPix_1 = temp_cmbd2;

        temp_0 = srcAddr[0];
        temp_1 = srcAddr[1];
        temp_cmbd = COMBINE_P0(temp_0, temp_1);
        temp_2 = srcAddr[2];
        temp_cmbd2 = COMBINE_P1(temp_1, temp_2);
        srcPix_0 = temp_cmbd;
        srcPix_1 = temp_cmbd2;

        dstPix_0 = SATURATING_ADD(dstPix_0, srcPix_0);
        dstPix_1 = SATURATING_ADD(dstPix_1, srcPix_1);

        dstAddr[0] = (unsigned char)(dstPix_0);
        dstAddr[1] = (unsigned char)((COMBINE_P3(dstPix_0, dstPix_1)));
        dstAddr[2] = (unsigned char)(dstPix_1 >> 4);

        dstAddr += 3;
        srcAddr += 3;
    }
#endif
    return NO_ERROR;
}

status_t addBayerBuffer(struct ExynosCameraBuffer *srcBuf,
                        struct ExynosCameraBuffer *dstBuf,
                        ExynosRect *dstRect,
                        bool isPacked)
{
    status_t ret = NO_ERROR;

    if (srcBuf == NULL) {
        ALOGE("ERR(%s[%d]):srcBuf == NULL",  __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (srcBuf == NULL) {
        ALOGE("ERR(%s[%d]):srcBuf == NULL",  __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    /* assume bayer buffer is 0 */
    if (srcBuf->size[0] != dstBuf->size[0]) {
        ALOGW("WARN(%s[%d]):srcBuf->size[0] (%d)!= dstBuf->size[0](%d). weird",
            __FUNCTION__, __LINE__, srcBuf->size[0], dstBuf->size[0]);
    }

    unsigned int copySize = dstBuf->size[0];

    if (srcBuf->size[0] < copySize)
        copySize = srcBuf->size[0];

#ifdef ADD_BAYER_BY_NEON
    if (isPacked) {
        ret = m_addBayerBufferByNeonPacked(srcBuf, dstBuf, dstRect, copySize);
    } else {
        ret = m_addBayerBufferByNeon(srcBuf, dstBuf, copySize);
    }

    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):m_addBayerBufferByNeon() fail", __FUNCTION__, __LINE__);
        return ret;
    }
#else
    ret = m_addBayerBufferByCpu(srcBuf, dstBuf, copySize);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):m_addBayerBufferByCpu() fail", __FUNCTION__, __LINE__);
        return ret;
    }
#endif

    return NO_ERROR;
}

}; /* namespace android */
