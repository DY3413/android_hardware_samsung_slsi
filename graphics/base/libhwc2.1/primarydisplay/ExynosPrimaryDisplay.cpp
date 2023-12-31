/*
 * Copyright (C) 2012 The Android Open Source Project
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
//#define LOG_NDEBUG 0

#define ATRACE_TAG (ATRACE_TAG_GRAPHICS | ATRACE_TAG_HAL)
#include "ExynosHWCDebug.h"
#include "ExynosPrimaryDisplay.h"
#include "ExynosHWCHelper.h"
#include "ExynosDisplayFbInterfaceModule.h"
#include "ExynosPrimaryDisplayFbInterfaceModule.h"
#include "ExynosDisplayDrmInterface.h"
#include "ExynosLayer.h"

#include <linux/fb.h>

extern struct exynos_hwc_control exynosHWCControl;

ExynosPrimaryDisplay::ExynosPrimaryDisplay(DisplayIdentifier node)
    : ExynosDisplay(node) {
    // There is no restriction in main display
    mNumMaxPriorityAllowed = MAX_DECON_WIN;

    mResolutionInfo.clear();

#ifdef HIBER_EXIT_NODE_NAME
    mHiberState.hiberExitFd = fopen(HIBER_EXIT_NODE_NAME, "r");
    if (mHiberState.hiberExitFd != NULL) {
        ALOGI("%s is opened", HIBER_EXIT_NODE_NAME);
    } else {
        ALOGI("Hibernation exit node is not opened");
    }
#endif

#if defined(MAX_BRIGHTNESS_NODE_BASE) && defined(BRIGHTNESS_NODE_BASE)
    FILE *maxBrightnessFd = fopen(MAX_BRIGHTNESS_NODE_BASE, "r");
    ALOGI("Trying %s open for get max brightness", MAX_BRIGHTNESS_NODE_BASE);

    if (maxBrightnessFd != NULL) {
        char val[MAX_BRIGHTNESS_LEN] = {0};
        size_t size = fread(val, 1, MAX_BRIGHTNESS_LEN, maxBrightnessFd);
        if (size) {
            mMaxBrightness = atoi(val);
            ALOGI("Max brightness : %d", mMaxBrightness);

            mBrightnessFd = fopen(BRIGHTNESS_NODE_BASE, "w+");
            ALOGI("Trying %s open for brightness control", BRIGHTNESS_NODE_BASE);

            if (mBrightnessFd == NULL)
                ALOGE("%s open failed! %s", BRIGHTNESS_NODE_BASE, strerror(errno));
        } else {
            ALOGE("Max brightness read failed (size: %zu)", size);
            if (ferror(maxBrightnessFd)) {
                ALOGE("An error occurred");
                clearerr(maxBrightnessFd);
            }
        }
        fclose(maxBrightnessFd);
    } else {
        ALOGE("Brightness node is not opened");
    }
#endif
}

ExynosPrimaryDisplay::~ExynosPrimaryDisplay() {
    if (mHiberState.hiberExitFd != NULL) {
        fclose(mHiberState.hiberExitFd);
        mHiberState.hiberExitFd = NULL;
    }

#ifdef USE_DISPLAY_COLOR_INTERFACE
    if (mDisplayColorCoefAddr)
        munmap(mDisplayColorCoefAddr, mDisplayColorCoefSize);
    if (mDisplayColorFd >= 0)
        close(mDisplayColorFd);
#endif

    if (mBrightnessFd != NULL) {
        fclose(mBrightnessFd);
        mBrightnessFd = NULL;
    }
}

int32_t ExynosPrimaryDisplay::setPowerMode(int32_t mode,
                                           uint64_t &geometryFlag) {
    if (mode == static_cast<int32_t>(mPowerModeState)) {
        ALOGI("Skip power mode transition due to the same power state.");
        return HWC2_ERROR_NONE;
    }

    int fb_blank = (mode != HWC2_POWER_MODE_OFF) ? FB_BLANK_UNBLANK : FB_BLANK_POWERDOWN;
    ALOGD("%s:: FBIOBLANK mode(%d), blank(%d), mPowerModeState(%d)", __func__, mode, fb_blank, mPowerModeState);
    String8 pendConfigDump;
    mPendConfigInfo.dump(pendConfigDump);
    ALOGD("\tactiveConfig:: %d, %s", mActiveConfig, pendConfigDump.string());

    switch (mode) {
    case HWC2_POWER_MODE_DOZE_SUSPEND:
        return setPowerDoze(true);
    case HWC2_POWER_MODE_DOZE:
        return setPowerDoze(false);
    case HWC2_POWER_MODE_OFF:
        if (mUseDynamicRecomp && mDynamicRecompTimer)
            mDynamicRecompTimer->stop();
        setPowerOff();
        setGeometryChanged(GEOMETRY_DISPLAY_POWER_OFF, geometryFlag);
        break;
    case HWC2_POWER_MODE_ON:
        if (mUseDynamicRecomp && mDynamicRecompTimer)
            mDynamicRecompTimer->start();
        setPowerOn();
        setGeometryChanged(GEOMETRY_DISPLAY_POWER_ON, geometryFlag);
        break;
    default:
        return HWC2_ERROR_BAD_PARAMETER;
    }

    return HWC2_ERROR_NONE;
}

int32_t ExynosPrimaryDisplay::setPowerOn() {
    ATRACE_CALL();

    if ((mDisplayInterface->mType != INTERFACE_TYPE_DRM) ||
        (mPowerModeState == HWC2_POWER_MODE_OFF) ||
        (mPowerModeState == HWC2_POWER_MODE_DOZE_SUSPEND)) {
        mDisplayInterface->setPowerMode(HWC2_POWER_MODE_ON);
    }

    hwc2_power_mode_t prevModeState = mPowerModeState;
    /*
     * Power mode should be set before calling
     * setActiveConfig, setActiveConfigWithConstraints
     */
    mPowerModeState = HWC2_POWER_MODE_ON;

    if (mPendConfigInfo.isPending && (mActiveConfig != mPendConfigInfo.config)) {
        if (mPendConfigInfo.configType == PendingConfigInfo::NO_CONSTRAINTS)
            ExynosDisplay::setActiveConfig(mPendConfigInfo.config);
        else
            ExynosDisplay::setActiveConfigWithConstraints(mPendConfigInfo.config,
                                                          &mPendConfigInfo.constraints,
                                                          &mPendConfigInfo.vsyncAppliedTimeLine,
                                                          false); /* No update timeline */
    }

    if ((prevModeState != HWC2_POWER_MODE_OFF) && /* It was low power mode */
        ((mPendConfigInfo.isPending == false) ||  /* There is no pending config */
         /* There was pending config but it was same with active config */
         (mPendConfigInfo.isPending && (mActiveConfig == mPendConfigInfo.config)) ||
         /* There was pending config but it will be applied later,
            active config should be applied until pending config is applied */
         (mConfigRequestState == hwc_request_state_t::SET_CONFIG_STATE_PENDING))) {
        /*
         * Save mConfigRequestState.
         * It can be reset by setActiveConfigInternal
         */
        hwc_request_state_t configRequestState = mConfigRequestState;

        /*
         * Restore active config
         * If the previous power mode was doze
         * actual display config is different with mActiveConfig.
         * Need to restore mActiveConfig.
         * setActiveConfig() can skip setting if requested config is same with
         * mActiveConfig.
         * In order not to skip config setting, call setActiveConfigInternal().
         */
        ExynosDisplay::setActiveConfigInternal(mActiveConfig);

        ALOGD("\t%s:configRequestState(%d)", __func__, configRequestState);
        /* Refresh in order to apply pending config */
        if (configRequestState == hwc_request_state_t::SET_CONFIG_STATE_PENDING) {
            /* Request one more time because status is reset by setActiveConfigInternal */
            ExynosDisplay::setActiveConfigWithConstraints(mPendConfigInfo.config,
                                                          &mPendConfigInfo.constraints,
                                                          &mPendConfigInfo.vsyncAppliedTimeLine,
                                                          false); /* No update timeline */
            invalidate();
        }
    }

    mPendConfigInfo.isPending = false;

    ALOGD("%s, Power on request done", __func__);

    return HWC2_ERROR_NONE;
}

int32_t ExynosPrimaryDisplay::setPowerOff() {
    ATRACE_CALL();

    clearDisplay();

    /*
     * Restore active config
     * It is same with power on case
     */
    if (mPowerModeState != HWC2_POWER_MODE_ON)
        ExynosDisplay::setActiveConfigInternal(mActiveConfig);

    mPowerModeState = HWC2_POWER_MODE_OFF;

    mDisplayInterface->setPowerMode(HWC2_POWER_MODE_OFF);

    if ((mRenderingState >= RENDERING_STATE_VALIDATED) &&
        (mRenderingState < RENDERING_STATE_PRESENTED))
        closeFencesForSkipFrame(RENDERING_STATE_VALIDATED);
    mRenderingState = RENDERING_STATE_NONE;

    ALOGD("%s, Power off request done", __func__);

    return HWC2_ERROR_NONE;
}

int32_t ExynosPrimaryDisplay::setPowerDoze(bool suspend) {
    ATRACE_CALL();

    if (!mDisplayInterface->isDozeModeAvailable()) {
        return HWC2_ERROR_UNSUPPORTED;
    }

    if (mDisplayInterface->setLowPowerMode(suspend)) {
        ALOGI("Not support LP mode.");
        return HWC2_ERROR_UNSUPPORTED;
    }

    ALOGD("%s, Doze request done", __func__);
    if (suspend)
        mPowerModeState = HWC2_POWER_MODE_DOZE_SUSPEND;
    else
        mPowerModeState = HWC2_POWER_MODE_DOZE;

    return HWC2_ERROR_NONE;
}

bool ExynosPrimaryDisplay::getHDRException(ExynosLayer *__unused layer,
                                           DevicePresentInfo &__unused deviceInfo) {
    return false;
}

void ExynosPrimaryDisplay::initDisplayInterface(uint32_t interfaceType,
                                                void *deviceData, size_t &deviceDataSize) {
    if (interfaceType == INTERFACE_TYPE_DRM)
        mDisplayInterface = std::make_unique<ExynosDisplayDrmInterface>();
    else
        mDisplayInterface = std::make_unique<ExynosPrimaryDisplayFbInterfaceModule>();
    mDisplayInterface->init(mDisplayInfo.displayIdentifier,
                            deviceData, deviceDataSize);
}

void ExynosPrimaryDisplay::init(uint32_t maxWindowNum, ExynosMPP *blendingMPP) {
    // Get display configs
    uint32_t outNumConfigs;
    getDisplayConfigs(&outNumConfigs, NULL);
    if (outNumConfigs >= 1) {
        std::vector<hwc2_config_t> outConfigs(outNumConfigs);
        if (getDisplayConfigs(&outNumConfigs, outConfigs.data()) != HWC2_ERROR_NONE)
            ALOGI("%s:: Getting display configs is failed", __func__);
    }
    hwc2_config_t config;
    mDisplayInterface->getDPUConfig(&config);
    updateInternalDisplayConfigVariables(config, true);
    mDisplayInterface->getDisplayHWInfo(mXres, mYres, mPsrMode, mResolutionInfo);

    uint32_t outNumTypes;
    float outMaxLuminance = 0.0f, outMaxAverageLuminance = 0.0f, outMinLuminance = 0.0f;

    getHdrCapabilities(&outNumTypes, NULL, &outMaxLuminance, &outMaxAverageLuminance, &outMinLuminance);

    std::vector<int32_t> outTypes(outNumTypes);
    getHdrCapabilities(&outNumTypes, outTypes.data(), &outMaxLuminance, &outMaxAverageLuminance, &outMinLuminance);

    mHdrCoefInterface = createHdrInterfaceInstance();
    mHdr10PMetaInterface = createHdr10PMetaInterfaceInstance();
    if (mHdrCoefInterface) {
        mHdrCoefInterface->sethdr10pMetaInterface(mHdr10PMetaInterface);
        mHdrTargetInfo.bpc = HDR_BPC_8;
        mHdrTargetInfo.hdr_capa = HDR_CAPA_INNER;
    }

#ifdef USE_DISPLAY_COLOR_INTERFACE
    mDisplayColorInterface = IDisplayColor::createInstance(mType);
    if (mDisplayColorInterface) {
        mDisplayColorCoefSize = mDisplayColorInterface->getDqeLutSize();
        if ((mDisplayColorCoefSize > 0) &&
            (allocParcelData(&mDisplayColorFd, mDisplayColorCoefSize) == NO_ERROR)) {
            mDisplayColorCoefAddr = (void *)mmap(0, mDisplayColorCoefSize,
                                                 PROT_READ | PROT_WRITE, MAP_SHARED, mDisplayColorFd, 0);
            if (mDisplayColorCoefAddr == MAP_FAILED) {
                ALOGE("%s::Fail to map DisplayColor Parcel data", __func__);
                mDisplayColorCoefAddr = NULL;
            }
        } else {
            ALOGE("%s:: Fail to alloc Parcel Data : %d", __func__, mDisplayColorCoefSize);
        }
    } else {
        ALOGE("%s:: Fail to create IDisplayColor instance", __func__);
    }
#endif

#ifdef ENABLE_DYNAMIC_RECOMP
    mUseDynamicRecomp = (mPsrMode == PSR_NONE);
#endif

    ExynosDisplay::init(maxWindowNum, blendingMPP);
}

void ExynosPrimaryDisplay::checkLayersForSettingDR() {
    Mutex::Autolock lock(mDRMutex);
    /* According to the following conditions, HWC will skip the mode switch
     * 1. There is YUV layer
     * 2. There is scaling layer
     * 3. Layer has plane alpha. (not 1.0)
     * 4. Layer needs color gamut change
     */
    for (size_t i = 0; i < mLayers.size(); i++) {
        if (mLayers[i]->mLayerBuffer) {
            exynos_image src, dst;
            mLayers[i]->setSrcExynosImage(&src);
            mLayers[i]->setDstExynosImage(&dst);

            if (src.exynosFormat.isYUV() ||
                (src.w != dst.w) || (src.h != dst.h) ||
                (src.planeAlpha != 1.0f) ||
                ((mColorMode != HAL_COLOR_MODE_NATIVE) &&
                 (src.dataSpace != dst.dataSpace))) {
                mDynamicRecompMode = CLIENT_TO_DEVICE;
                DISPLAY_LOGD(eDebugDynamicRecomp, "[DYNAMIC_RECOMP] CLIENT_TO_DEVICE by specific condition layer");
                return;
            }
        }
    }
    DISPLAY_LOGD(eDebugDynamicRecomp, "[DYNAMIC_RECOMP] DEVICE_TO_CLIENT is set");
    mDynamicRecompMode = DEVICE_TO_CLIENT;
    invalidate();
}

int32_t ExynosPrimaryDisplay::getClientTargetSupport(
    uint32_t width, uint32_t height,
    int32_t /*android_pixel_format_t*/ format,
    int32_t /*android_dataspace_t*/ dataspace) {
    if (mPendConfigInfo.isPending) {
        int pendingWidth, pendingHeight;
        getDisplayAttribute(mPendConfigInfo.config, HWC2_ATTRIBUTE_WIDTH, (int32_t *)&pendingWidth);
        getDisplayAttribute(mPendConfigInfo.config, HWC2_ATTRIBUTE_HEIGHT, (int32_t *)&pendingHeight);
        if (pendingWidth != width)
            return HWC2_ERROR_UNSUPPORTED;
        if (pendingHeight != height)
            return HWC2_ERROR_UNSUPPORTED;
    }
    return ExynosDisplay::getClientTargetSupport(mXres, mYres, format, dataspace);
}
