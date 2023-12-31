/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef HUB_CONNECTION_H_

#define HUB_CONNECTION_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

#include <android/hardware/sensors/1.0/types.h>
#include <android/hardware/sensors/2.0/types.h>
#include <utils/Errors.h>
#include <utils/Mutex.h>
#include <utils/Thread.h>

#include <list>

#include "directchannel.h"
#include "../../firmware/os/inc/eventnums.h"
#include "../../firmware/os/inc/halIntf.h"
#include "hubdefs.h"
#include "sensorlist.h"
#include "../../../../../device/google/contexthub/util/common/ring.h"
#include "V2_0/SubHal.h"
#include "V2_1/SubHal.h"

#include <unordered_map>

#define WAKELOCK_NAME "sensorHal"

#define ACCEL_BIAS_TAG     "accel"
#define ACCEL_SW_BIAS_TAG  "accel_sw"
#define GYRO_BIAS_TAG      "gyro"
#define GYRO_OTC_DATA_TAG  "gyro_otc"
#define GYRO_SW_BIAS_TAG   "gyro_sw"
#define MAG_BIAS_TAG       "mag"

#define MAX_ALTERNATES     2
#define DFS_TABLE_PATH   "/mnt/vendor/efs/chub/dfs"

namespace android {

using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V2_1::Event;
using ::android::hardware::sensors::V2_1::implementation::IHalProxyCallback;
using ::android::hardware::Return;

struct HubConnection : public Thread {
    static HubConnection *getInstance();

    status_t initCheck() const;

    enum ProximitySensorType {
        PROXIMITY_UNKNOWN,
        PROXIMITY_ROHM,
        PROXIMITY_AMS,
    };

    virtual bool threadLoop();

    void queueActivate(int handle, bool enable);
    void queueBatch(int handle, nsecs_t sampling_period_ns,
            nsecs_t max_report_latency_ns);
    Return<Result> queueFlush(int handle);

    void postEvents(const std::vector<Event>& events, bool wakeup);

    ssize_t write(const sensors_event_t *ev, size_t n);

    void saveSensorSettings() const;

    void setcallback(const sp<IHalProxyCallback>& callback) {
        mCallback = callback;
    }

    void dfs_save(void);
    void dfs_send_data(void *data, size_t length);
    void dfs_load(void);
    void processDfsRequest(uint8_t *buf, ssize_t len);

protected:
    HubConnection();
    virtual ~HubConnection();

    virtual void onFirstRef();

private:
    typedef uint32_t rate_q10_t;  // q10 means lower 10 bits are for fractions

    sp<IHalProxyCallback> mCallback;

    static inline uint64_t period_ns_to_frequency_q10(nsecs_t period_ns) {
        return 1024000000000ULL / period_ns;
    }

    static inline nsecs_t frequency_q10_to_period_ns(uint64_t frequency_q10) {
        if (frequency_q10)
            return 1024000000000LL / frequency_q10;
        else
            return (nsecs_t)0;
    }

    static inline uint64_t frequency_to_frequency_q10(float frequency) {
        return period_ns_to_frequency_q10(static_cast<nsecs_t>(1e9f/frequency));
    }

    enum
    {
        CONFIG_CMD_DISABLE      = 0,
        CONFIG_CMD_ENABLE       = 1,
        CONFIG_CMD_FLUSH        = 2,
        CONFIG_CMD_CFG_DATA     = 3,
        CONFIG_CMD_CALIBRATE    = 4,
    };

    struct ConfigCmd
    {
        uint32_t evtType;
        uint64_t latency;
        rate_q10_t rate;
        uint8_t sensorType;
        uint8_t cmd;
        uint16_t flags;
        uint8_t data[];
    } __attribute__((packed));

    struct MsgCmd
    {
        uint32_t evtType;
        struct HostHubRawPacket msg;
    } __attribute__((packed));

    struct Flush
    {
        int handle;
        uint8_t count;

        // Used to synchronize the transition in and out of
        // lefty mode between nanohub and the AP.
        bool internal;
    };

    struct SensorState {
        uint64_t latency;
        uint64_t lastTimestamp;
        uint64_t desiredTSample;
        rate_q10_t rate;
        uint8_t sensorType;
        uint8_t primary;
        uint8_t alt[MAX_ALTERNATES];
        bool enable;
    };

    struct FirstSample
    {
        uint8_t numSamples;
        uint8_t numFlushes;
        uint8_t highAccuracy : 1;
        uint8_t biasPresent : 1;
        uint8_t biasSample : 6;
        uint8_t pad;
    };

    struct RawThreeAxisSample
    {
        uint32_t deltaTime;
        int16_t ix, iy, iz;
    } __attribute__((packed));

    struct ThreeAxisSample
    {
        uint32_t deltaTime;
        float x, y, z;
    } __attribute__((packed));

    struct OneAxisSample
    {
        uint32_t deltaTime;
        union
        {
            float fdata;
            uint32_t idata;
        };
    } __attribute__((packed));

    // The following structure should match struct HostIntfDataBuffer found in
    // firmware/inc/hostIntf.h
    struct nAxisEvent
    {
        uint32_t evtType;
        union
        {
            struct
            {
                uint64_t referenceTime;
                union
                {
                    struct FirstSample firstSample;
                    struct OneAxisSample oneSamples[];
                    struct RawThreeAxisSample rawThreeSamples[];
                    struct ThreeAxisSample threeSamples[];
                };
            };
            uint8_t buffer[];
        };
    } __attribute__((packed));

    static Mutex sInstanceLock;
    static HubConnection *sInstance;

    // This lock is used for synchronization between the write thread (from
    // sensorservice) and the read thread polling from the nanohub driver.
    Mutex mLock;

    int32_t mWriteFailures;

    float mMagBias[3];
    uint8_t mMagAccuracy;
    uint8_t mMagAccuracyRestore;

    float mGyroBias[3], mAccelBias[3], mAccelEnabledBias[3];
    bool mAccelEnabledBiasStored;
    GyroOtcData mGyroOtcData;

    float mScaleAccel, mScaleMag;

    SensorState mSensorState[NUM_COMMS_SENSORS_PLUS_1];
    std::list<struct Flush> mFlushesPending[NUM_COMMS_SENSORS_PLUS_1];

    uint64_t mStepCounterOffset;
    uint64_t mLastStepCount;

    uint64_t mPedometerOffset;
    uint64_t mLastPedometer;

    uint64_t mCurrent = 0;

    int mFd;
    int mInotifyPollIndex;
    struct pollfd mPollFds[4];
    int mNumPollFds;

    sensors_event_t *initEv(sensors_event_t *ev, uint64_t timestamp, uint32_t type, uint32_t sensor);
    uint8_t magAccuracyUpdate(sensors_vec_t *sv);
    void processSample(uint64_t timestamp, uint32_t type, uint32_t sensor, struct OneAxisSample *sample, bool highAccuracy);
    void processSample(uint64_t timestamp, uint32_t type, uint32_t sensor, struct RawThreeAxisSample *sample, bool highAccuracy);
    void processSample(uint64_t timestamp, uint32_t type, uint32_t sensor, struct ThreeAxisSample *sample, bool highAccuracy);
    ssize_t processBuf(uint8_t *buf, size_t len);

    inline bool isValidHandle(int handle) {
        return handle >= 0
            && handle < NUM_COMMS_SENSORS_PLUS_1
            && mSensorState[handle].sensorType;
    }

    ssize_t sendCmd(const void *buf, size_t count);
    void initConfigCmd(struct ConfigCmd *cmd, int handle);

    void queueFlushInternal(int handle, bool internal);

    void queueDataInternal(int handle, void *data, size_t length);

    void discardInotifyEvent();
    void waitOnNanohubLock();

    void initNanohubLock();

    void restoreSensorState();
    void sendCalibrationOffsets();

    // Enable SCHED_FIFO priority for main thread
    void enableSchedFifoMode();

    // Direct report functions
public:
    int addDirectChannel(const struct sensors_direct_mem_t *mem);
    int removeDirectChannel(int channel_handle);
    int configDirectReport(int sensor_handle, int channel_handle, int rate_level);
    bool isDirectReportSupported() const;
private:
    void sendDirectReportEvent(const sensors_event_t *nev, size_t n);
    void mergeDirectReportRequest(struct ConfigCmd *cmd, int handle);
    bool isSampleIntervalSatisfied(int handle, uint64_t timestamp);
    void updateSampleRate(int handle, int reason);
    bool isTimestampSatisfied(uint32_t sensor, uint64_t timestamp);
#ifdef DIRECT_REPORT_ENABLED
    int stopAllDirectReportOnChannel(
            int channel_handle, std::vector<int32_t> *unstoppedSensors);
    uint64_t rateLevelToDeviceSamplingPeriodNs(int handle, int rateLevel) const;
    inline static bool intervalLargeEnough(uint64_t actual, uint64_t desired) {
        return (actual + (actual >> 4)) >= desired; // >= 94.11% of desired
    }

    struct DirectChannelTimingInfo{
        uint64_t lastTimestamp;
        int rateLevel;
    };
    Mutex mDirectChannelLock;
    //sensor_handle=>(channel_handle => DirectChannelTimingInfo)
    std::unordered_map<int32_t,
            std::unordered_map<int32_t, DirectChannelTimingInfo> > mSensorToChannel;
    //channel_handle=>ptr of Channel obj
    std::unordered_map<int32_t, std::unique_ptr<DirectChannelBase>> mDirectChannel;
    int32_t mDirectChannelHandle;
#endif

    DISALLOW_EVIL_CONSTRUCTORS(HubConnection);
};

}  // namespace android

#endif  // HUB_CONNECTION_H_
