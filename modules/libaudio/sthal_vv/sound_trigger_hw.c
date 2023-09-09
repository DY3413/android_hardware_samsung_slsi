/*
 * Copyright (C) 2017 The Android Open Source Project
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

/*
 * @file            : sound_trigger_hw.c
 * @brief           : Sound Trigger primary HAL implmentation
 * @author          : Palli Satish Kumar Reddy (palli.satish@samsung.com)
 * @version         : 1.0
 * @history
 *   2017.02.20     : Create
 */

#define LOG_TAG "soundtrigger_hw_primary"
#define LOG_NDEBUG 0

//#define VERY_VERBOSE_LOGGING
#ifdef VERY_VERBOSE_LOGGING
#define ALOGVV ALOGV
#else
#define ALOGVV(a...) do { } while(0)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/prctl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <log/log.h>
#include <cutils/uevent.h>
#include <dlfcn.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>

#include "sound_trigger_hw.h"

#ifdef MMAP_INTERFACE_ENABLED
#include"vts.h"
#endif
static void get_vts_keyword_length(struct sound_trigger_device *stdev);
#ifdef SUPPORT_BARGEIN_MODE
static void set_bargein_keyword_type(struct sound_trigger_device *stdev);
static void get_bargein_keyword_length(struct sound_trigger_device *stdev);
static void enable_bargein(struct sound_trigger_device *stdev, int enabled_algorithms);
static void disable_bargein(struct sound_trigger_device *stdev);
#endif

/* Note: odmword_uuid should be updated */
static sound_trigger_uuid_t odmword_uuid = { 0x1817de20, 0xfa3b, 0x11e5, 0xbef2, { 0x00, 0x03, 0xa6, 0xd6, 0xc6, 0x1c } };
static sound_trigger_uuid_t hotword_uuid = { 0x7038ddc8, 0x30f2, 0x11e6, 0xb0ac, { 0x40, 0xa8, 0xf0, 0x3d, 0x3f, 0x15 } };


// A few function declarations to have some sort of logical ordering of code below.
static void *callback_thread_loop(void *context);
static void stdev_close_callback_thread_sockets(struct sound_trigger_device *stdev);
static void stdev_join_callback_thread(struct sound_trigger_device *stdev,
                                       bool keep_vts_powered);
static int stdev_stop_recognition(const struct sound_trigger_hw_device *dev,
                                  sound_model_handle_t handle);
static int stdev_stop_recognition_l(struct sound_trigger_device *stdev,
                                    sound_model_handle_t handle);
static void handle_stop_recognition_l(struct sound_trigger_device *stdev);


// Since there's only ever one sound_trigger_device, keep it as a global so that other people can
// dlopen this lib to get at the streaming audio.
static struct sound_trigger_device g_stdev = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .trigger_lock = PTHREAD_MUTEX_INITIALIZER,
    .recording_lock = PTHREAD_MUTEX_INITIALIZER
};

// Utility function for configuration MIC mixer controls
int set_mixer_ctrls(
        struct sound_trigger_device *stdev,
        char *path_name[],
        int *path_ctlvalue,
        int ctrl_count,
        bool reverse)
{
    int i = (reverse ? (ctrl_count - 1): 0);
    int ret = 0;
    struct mixer_ctl *mixerctl = NULL;

    ALOGV("%s, path: %s", __func__, path_name[0]);

    if (stdev->mixer) {
        //for (i=0; i < ctrl_count; i++) {
        while(ctrl_count) {
            ALOGVV("%s, ctrl_count: %d Loop index: %d", __func__, ctrl_count, i);
            /* Get required control from mixer */
            mixerctl = mixer_get_ctl_by_name(stdev->mixer, path_name[i]);
            if (mixerctl) {
                /* Enable the control */
                if (path_ctlvalue)
                    ret = mixer_ctl_set_value(mixerctl, 0, path_ctlvalue[i]);
                else
                    ret = mixer_ctl_set_value(mixerctl, 0, 0);

                if (ret) {
                    ALOGE("%s: %s Failed to configure\n", __func__, path_name[i]);
                    ret = -EINVAL;
                    break;
                } else {
                    ALOGV("%s: %s configured value: %d\n", __func__, path_name[i],
                        (path_ctlvalue ? path_ctlvalue[i] : 0));
                }
             } else {
                ALOGE("%s: %s control doesn't exist\n", __func__, path_name[i]);
                ret = -EINVAL;
                break;
            }
            ctrl_count--;
            if (reverse)
                i--;
            else
                i++;
        }
    } else{
        ALOGE("%s: Failed to open mixer\n", __func__);
        return -EINVAL;
    }

    return ret;
}

#ifdef MMAP_INTERFACE_ENABLED
// Utility function for loading model binary to kernel through mmap interface
static int load_modelbinary(
    struct sound_trigger_device *stdev,
    int model_index,
    char *data,
    int len)
{
    unsigned long ioctl_cmd = 0;
    int ret = 0;

    ALOGV("%s: Actual size %d\n", __func__, len);

    /* Copy model binary to VTS mapped address */
    memcpy(stdev->mapped_addr, data, len);

    ALOGV("%s: %s Model loaded size[%d]", __func__,
                (model_index == HOTWORD_INDEX ? "Google" : "ODMVoice"), len);

    if (len > VTSDRV_MISC_MODEL_BIN_MAXSZ) {
        ALOGW("%s: buffer overflow Model size[%d] > Mapped bufsize[%d]",
                __func__, len, VTSDRV_MISC_MODEL_BIN_MAXSZ);
        len = VTSDRV_MISC_MODEL_BIN_MAXSZ;
    }

    if (model_index == HOTWORD_INDEX)
        ioctl_cmd = VTSDRV_MISC_IOCTL_WRITE_GOOGLE;
    else
        ioctl_cmd = VTSDRV_MISC_IOCTL_WRITE_ODMVOICE;

    /* Update model binary inforation to VTS misc driver */
    if (ioctl(stdev->vtsdev_fd, ioctl_cmd, &len) < 0) {
        ALOGE("%s: VTS device IOCTL failed", __func__);
        return -EINVAL;
    }

    return ret;
}
#else
// Utility function for loading model binary to kernel through sysfs interface
static int sysfs_write(
    const char *path,
    char *data,
    int len)
{
    char buf[80];
    int fd = open(path, O_WRONLY);
    int tmp = 0, written = 0;
    int ret = 0;

    ALOGV("%s: Actual size %d\n", __func__, len);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return fd;
    }

    while (len) {
        tmp = write(fd, data+written, len);
        if (tmp < 0) {
            strerror_r(errno, buf, sizeof(buf));
            ALOGE("Error writing to %s: %s\n", path, buf);
            ret = tmp;
            break;
        }
        len -= tmp;
        written += tmp;
        //ALOGV("%s: current written %d Actual %d Total written %d\n", __func__, tmp, len, written);
    }

    ALOGV("%s: Total written %d\n", __func__, written);

    close(fd);
    return ret;
}
#endif

// Utility function for allocating a hotword recognition event. Caller receives ownership of
// allocated struct.
static struct sound_trigger_recognition_event *sound_trigger_hotword_event_alloc(
        struct sound_trigger_device *stdev)
{
    struct sound_trigger_phrase_recognition_event *event =
            (struct sound_trigger_phrase_recognition_event *)calloc(
                    1, sizeof(struct sound_trigger_phrase_recognition_event));
    if (!event) {
        return NULL;
    }

    event->common.status = RECOGNITION_STATUS_SUCCESS;
    event->common.type = SOUND_MODEL_TYPE_KEYPHRASE;
    event->common.model = stdev->model_handles[HOTWORD_INDEX];

    if (stdev->configs[HOTWORD_INDEX]) {
        unsigned int i;

        event->num_phrases = stdev->configs[HOTWORD_INDEX]->num_phrases;
        if (event->num_phrases > SOUND_TRIGGER_MAX_PHRASES)
            event->num_phrases = SOUND_TRIGGER_MAX_PHRASES;
        for (i=0; i < event->num_phrases; i++)
            memcpy(&event->phrase_extras[i], &stdev->configs[HOTWORD_INDEX]->phrases[i],
                   sizeof(struct sound_trigger_phrase_recognition_extra));
    }

    event->num_phrases = 1;
    event->phrase_extras[0].confidence_level = 100;
    event->phrase_extras[0].num_levels = 1;
    event->phrase_extras[0].levels[0].level = 100;
    event->phrase_extras[0].levels[0].user_id = 0;
    // Signify that all the data is coming through streaming, not through the
    // buffer.
    event->common.capture_available = true;
    event->common.trigger_in_data = false;

    event->common.audio_config = AUDIO_CONFIG_INITIALIZER;
    event->common.audio_config.sample_rate = 16000;
    event->common.audio_config.channel_mask = AUDIO_CHANNEL_IN_MONO;
    event->common.audio_config.format = AUDIO_FORMAT_PCM_16_BIT;

    return &event->common;
}

// Utility function for allocating a hotsound recognition event. Caller receives ownership of
// allocated struct.
static struct sound_trigger_recognition_event *sound_trigger_odmvoice_event_alloc(
        struct sound_trigger_device* stdev)
{
#if 0 //Generic recognition event
    struct sound_trigger_generic_recognition_event *event =
            (struct sound_trigger_generic_recognition_event *)calloc(
                    1, sizeof(struct sound_trigger_generic_recognition_event));
    if (!event) {
        return NULL;
    }

    event->common.status = RECOGNITION_STATUS_SUCCESS;
    event->common.type = SOUND_MODEL_TYPE_GENERIC;
    event->common.model = stdev->model_handles[ODMVOICE_INDEX];
    // Signify that all the data is coming through streaming, not through the
    // buffer.
    event->common.capture_available = true;

    event->common.audio_config = AUDIO_CONFIG_INITIALIZER;
    event->common.audio_config.sample_rate = 16000;
    event->common.audio_config.channel_mask = AUDIO_CHANNEL_IN_MONO;
    event->common.audio_config.format = AUDIO_FORMAT_PCM_16_BIT;
#else //as ODMVoice model
    struct sound_trigger_phrase_recognition_event *event =
            (struct sound_trigger_phrase_recognition_event *)calloc(
                    1, sizeof(struct sound_trigger_phrase_recognition_event));
    if (!event) {
        return NULL;
    }

    event->common.status = RECOGNITION_STATUS_SUCCESS;
    event->common.type = SOUND_MODEL_TYPE_KEYPHRASE;
    event->common.model = stdev->model_handles[ODMVOICE_INDEX];

    if (stdev->configs[ODMVOICE_INDEX]) {
        unsigned int i;

        event->num_phrases = stdev->configs[ODMVOICE_INDEX]->num_phrases;
        if (event->num_phrases > SOUND_TRIGGER_MAX_PHRASES)
            event->num_phrases = SOUND_TRIGGER_MAX_PHRASES;
        for (i=0; i < event->num_phrases; i++)
            memcpy(&event->phrase_extras[i], &stdev->configs[ODMVOICE_INDEX]->phrases[i],
                   sizeof(struct sound_trigger_phrase_recognition_extra));
    }

    event->num_phrases = 1;
    event->phrase_extras[0].confidence_level = 100;
    event->phrase_extras[0].num_levels = 1;
    event->phrase_extras[0].levels[0].level = 100;
    event->phrase_extras[0].levels[0].user_id = 0;
    // Signify that all the data is coming through streaming, not through the
    // buffer.
    event->common.capture_available = true;
    event->common.trigger_in_data = false;

    event->common.audio_config = AUDIO_CONFIG_INITIALIZER;
    event->common.audio_config.sample_rate = 16000;
    event->common.audio_config.channel_mask = AUDIO_CHANNEL_IN_MONO;
    event->common.audio_config.format = AUDIO_FORMAT_PCM_16_BIT;
#endif
    return &event->common;
}
#ifdef STHAL_2_3
const struct sound_trigger_properties_header* stdev_get_properties_extended(
        const struct sound_trigger_hw_device *dev) {
    struct sound_trigger_device *stdev = (struct sound_trigger_device *)dev;

    ALOGI("%s: enter", __func__);

    if (stdev == NULL)
        return NULL;

    return (struct sound_trigger_properties_header *)&hw_properties_1_3;
}
#endif
static int stdev_get_properties(
        const struct sound_trigger_hw_device *dev,
        struct sound_trigger_properties *properties)
{
    struct sound_trigger_device *stdev = (struct sound_trigger_device *)dev;
    int google_version = 0;

    ALOGI("%s", __func__);
    if (stdev == NULL || properties == NULL) {
        return -EINVAL;
    }
#ifdef MMAP_INTERFACE_ENABLED
    pthread_mutex_lock(&stdev->lock);
    if (ioctl(stdev->vtsdev_fd, VTSDRV_MISC_IOCTL_READ_GOOGLE_VERSION, &google_version) < 0) {
        ALOGE("%s: VTSDRV_MISC_IOCTL_READ_GOOGLE_VERSION failed", __func__);
        //return -EINVAL;
    }
    pthread_mutex_unlock(&stdev->lock);
    ALOGI("%s Google Version : %d", __func__, google_version);
#endif
    memcpy(properties, &hw_properties, sizeof(struct sound_trigger_properties));
    properties->version = google_version;
    return 0;
}

//Parses Extra config structure data, for ODMVoice specification information
static void ParseExtraConfigData(
        struct sound_trigger_device *stdev,
        const char *keyValuePairs)
{
    ALOGV("%s : %s", __func__, keyValuePairs);
    struct str_parms *parms = str_parms_create_str(keyValuePairs);
    int value;
    int ret;

    if (!parms) {
        ALOGE("%s: str_params NULL", __func__);
        return;
    }

	/* Note: if any Extra config data is defined by ODM,
	 * parsing strings should updated as ODM specific requirements
	 * Information that can be set in extra data,
	 * 1. Backlog_size: How many ms of previous data to be captured from the trigger word
	 * 2. Voice trigger mode: ODM specific, other default trigger mode will be used
	*/

    // get backlog_size
    ret = str_parms_get_int(parms, "backlog_size", &value);
    if (ret >= 0) {
        ALOGV("backlog_size = (%d)", value);
        stdev->backlog_size = value;
        str_parms_del(parms, "backlog_size");
    }

    /* ODM specific voice trigger mode if any
     * currently 3 modes as reserved1, reserved2 & default one ODM Voice trigger mode
    */
    ret = str_parms_get_int(parms, "voice_trigger_mode", &value);
    if (ret >= 0) {
        ALOGV("voice_trigger_mode = (%d)", value);
        stdev->odmvoicemodel_mode = value;
        str_parms_del(parms, "voice_trigger_mode");
    }
    str_parms_destroy(parms);
}

// If enable_mic = 0, then the VTS MIC controls disabled.
// Must be called with the stdev->lock held.
static void stdev_vts_set_mic(
        struct sound_trigger_device *stdev,
        int enable_mic)
{
    ALOGV("%s, enable_mic:%d, stdev->is_mic_configured:%d",__func__, enable_mic, stdev->is_mic_configured);
    if (enable_mic != 0) {
        /* Check whether MIC controls are configured or not, if not configure first */
        /* FXIME: add condition to check whether MAIN or HEADSET MIC should be configured */
        if (!stdev->is_mic_configured) {

            if (stdev->notify_set_vts_route)
                stdev->notify_set_vts_route(true, stdev->active_mic);

            stdev->is_mic_configured = 1;
            ALOGD("%s: Enable MIC Controls ", __func__);
        }
    } else {
        /* Reset MIC controls for disabling VTS */
        if (stdev->is_mic_configured) {
            if (stdev->notify_set_vts_route)
                stdev->notify_set_vts_route(false, stdev->active_mic);

            stdev->is_mic_configured = 0;
            ALOGD("%s: Disable MIC Controls ", __func__);
        }
    }

    return;
}

static int stdev_set_parameter(const struct sound_trigger_hw_device *dev __unused,
                                   sound_model_handle_t handle __unused,
                                   sound_trigger_model_parameter_t param __unused,
                                   int32_t value __unused) {
    ALOGI("%s: enter", __func__);

    return 0;
}

static int stdev_get_parameter(const struct sound_trigger_hw_device *dev __unused,
        sound_model_handle_t handle __unused, sound_trigger_model_parameter_t param __unused,
        int32_t *value __unused) {
    ALOGI("%s: enter", __func__);

    return 0;
}

static int stdev_query_parameter(const struct sound_trigger_hw_device *dev __unused,
        sound_model_handle_t handle __unused, sound_trigger_model_parameter_t param __unused,
        sound_trigger_model_parameter_range_t *param_range __unused) {
    ALOGI("%s: enter", __func__);

    return 0;
}

// If enabled_algorithms = 0, then the VTS will be turned off. Otherwise, treated as a bit mask for
// which algorithms should be enabled on the VTS. Must be called with the stdev->lock held.
static void stdev_vts_set_power(
        struct sound_trigger_device *stdev,
        int enabled_algorithms)
{
    ALOGV("%s enabled: %d", __func__, enabled_algorithms);
    ALOGD("%s stdev->is_streaming %d", __func__, stdev->is_streaming);

    stdev->is_streaming = 0;

    if (stdev->streaming_pcm) {
        ALOGW("%s: Streaming PCM node is not closed", __func__);
        pcm_close(stdev->streaming_pcm);
        stdev->streaming_pcm = NULL;
    }
    stdev->is_seamless_recording = false;

    if (enabled_algorithms != 0) {
        /* Configure MIC controls first */
        stdev_vts_set_mic(stdev, true);

        /* Start recognition of bit masked algorithms */
        if (enabled_algorithms & (0x1 << HOTWORD_INDEX)) {
            ALOGV("%s: Google Model recognization start", __func__);
            if (set_mixer_ctrls(stdev, model_recognize_start_ctlname,
                hotword_recognize_start_ctlvalue, MODEL_START_CONTROL_COUNT, false)) {
                ALOGE("%s: Google Model recognization start Failed", __func__);
                goto exit;
            }
            stdev->recognize_started |= (0x1 << HOTWORD_INDEX);
            ALOGD("%s: Google Model recognization started & Notified to AudioHAL", __func__);
        }

        if (enabled_algorithms & (0x1 << ODMVOICE_INDEX)) {
            int *ctrl_values = NULL;

            if (stdev->odmvoicemodel_mode == ODMVOICE_RESERVED1_MODE)
                ctrl_values = odmvoice_reserved1recognize_start_ctlvalue;
            else if (stdev->odmvoicemodel_mode == ODMVOICE_RESERVED2_MODE)
                ctrl_values = odmvoice_reserved2recognize_start_ctlvalue;
            else if (stdev->odmvoicemodel_mode == ODMVOICE_TRIGGER_MODE)
                ctrl_values = odmvoice_triggerrecognize_start_ctlvalue;
            else {
                ALOGE("%s: Unknown ODMVoice recognition mode to start, set default ODMVoice Trigger mode", __func__);
                ctrl_values = odmvoice_triggerrecognize_start_ctlvalue;
            }

            ALOGV("%s: ODMVoice Model [%s] recognization start", __func__,
                ((stdev->odmvoicemodel_mode == ODMVOICE_RESERVED2_MODE) ?
                "ODM Reserved2 mode" : ((stdev->odmvoicemodel_mode == ODMVOICE_RESERVED1_MODE) ?
                "ODM Reserved1 mode" : "ODMVoice trigger mode")));
            if (set_mixer_ctrls(stdev, model_recognize_start_ctlname,
                        ctrl_values, MODEL_START_CONTROL_COUNT, false)) {
                ALOGE("%s: ODMVoice Model recognization start Failed", __func__);
                goto exit;
            }

            /* handle backlog control size */
            if ((stdev->odmvoicemodel_mode == ODMVOICE_RESERVED1_MODE ||
                stdev->odmvoicemodel_mode == ODMVOICE_TRIGGER_MODE) &&
                stdev->backlog_size) {
                if (set_mixer_ctrls(stdev, model_backlog_size_ctlname,
                            &stdev->backlog_size, MODEL_BACKLOG_CONTROL_COUNT, false)) {
                    ALOGE("%s: ODMVoice Model backlog size configuration Failed", __func__);
                    goto exit;
                }
                ALOGD("%s: ODMVoice Model Backlog size [%d] configured", __func__, stdev->backlog_size);
            }
            stdev->recognize_started |= (0x1 << ODMVOICE_INDEX);
            ALOGD("%s: ODMVoice Model [%s] recognization started", __func__,
                ((stdev->odmvoicemodel_mode == ODMVOICE_RESERVED2_MODE) ?
                "ODM Reserved2 mode" : ((stdev->odmvoicemodel_mode == ODMVOICE_RESERVED1_MODE) ?
                "ODM Reserved1 mode" : "ODMVoice trigger mode")));
        }
    } else {
        /* Stop recognition of previous started models */
        if (stdev->recognize_started & (0x1 << HOTWORD_INDEX)) {
            if (set_mixer_ctrls(stdev, model_recognize_stop_ctlname,
                hotword_recognize_stop_ctlvalue, MODEL_STOP_CONTROL_COUNT, false)) {
                ALOGE("%s: Google Model recognization stop Failed", __func__);
                goto exit;
            }
            stdev->recognize_started &= ~(0x1 << HOTWORD_INDEX);
            ALOGD("%s: Google Model recognization stopped & Notified to AudioHAL", __func__);
        }

        if (stdev->recognize_started & (0x1 << ODMVOICE_INDEX)) {
            int *ctrl_values = NULL;

            if (stdev->odmvoicemodel_mode == ODMVOICE_RESERVED1_MODE)
                ctrl_values = odmvoice_reserved1recognize_stop_ctlvalue;
            else if (stdev->odmvoicemodel_mode == ODMVOICE_RESERVED2_MODE)
                ctrl_values = odmvoice_reserved2recognize_stop_ctlvalue;
            else if (stdev->odmvoicemodel_mode == ODMVOICE_TRIGGER_MODE)
                ctrl_values = odmvoice_triggerrecognize_stop_ctlvalue;
            else {
                ALOGE("%s: Unknown ODMVoice recognition mode to stop, use default ODMVoice Trigger mode", __func__);
                ctrl_values = odmvoice_triggerrecognize_stop_ctlvalue;
            }

            if (set_mixer_ctrls(stdev, model_recognize_stop_ctlname,
                ctrl_values, MODEL_STOP_CONTROL_COUNT, false)) {
                ALOGE("%s: ODMVoice Model recognization stop Failed", __func__);
                stdev->recognize_started &= ~(0x1 << ODMVOICE_INDEX);
                goto exit;
            }
            stdev->recognize_started &= ~(0x1 << ODMVOICE_INDEX);
            ALOGD("%s: ODMVoice Model [%s] recognization stopped", __func__,
                ((stdev->odmvoicemodel_mode == ODMVOICE_RESERVED2_MODE) ?
                "ODM Reserved2 mode" : ((stdev->odmvoicemodel_mode == ODMVOICE_RESERVED1_MODE) ?
                "ODM Reserved1 mode" : "ODMVoice trigger mode")));
        }

        ALOGD("%s: stdev->recognize_started %d, stdev->is_recording %d", __func__,
            stdev->recognize_started, stdev->is_recording);

        if (!stdev->recognize_started && !stdev->is_recording) {
            /* Reset MIC controls for disabling VTS */
            stdev_vts_set_mic(stdev, false);
        }
    }

exit:
    if (!stdev->recognize_started && !stdev->is_recording) {
        /* Reset MIC controls for disabling VTS */
        stdev_vts_set_mic(stdev, false);
    }
    return;
}

static int stdev_init_mixer(struct sound_trigger_device *stdev)
{
    int ret = -1;
    int retry = 30;

    ALOGV("%s", __func__);

    do {
        stdev->mixer = mixer_open(VTS_MIXER_CARD);
        if (stdev->mixer) {
            ret = 0;
            goto end;
        } else {
            usleep(100000);
            retry--;
            ALOGW("%s: Mixer open failed retrying cnt: %d", __func__, (retry % 30));
        }
    } while (retry);

    if (stdev->mixer)
        mixer_close(stdev->mixer);

end:
    return ret;
}

static void stdev_close_mixer(struct sound_trigger_device *stdev)
{
    ALOGV("%s", __func__);
#ifdef SUPPORT_BARGEIN_MODE
    if (stdev->is_bargein_mode_configed)
        disable_bargein(stdev);
    else if (stdev->is_mic_configured)
#endif
        stdev_vts_set_power(stdev, 0);

    stdev_join_callback_thread(stdev, false);
    mixer_close(stdev->mixer);
}

// Starts the callback thread if not already running. Returns 0 on success, or a negative error code
// otherwise. Must be called with the stdev->lock held.
static int stdev_start_callback_thread(struct sound_trigger_device *stdev)
{
    ALOGV("%s", __func__);

    if (stdev->callback_thread_active) {
        ALOGV("%s callback thread is already running", __func__);
        return 0;
    }
    int ret = 0;

    // If we existing sockets to communicate with the thread, close them and make new ones.
    stdev_close_callback_thread_sockets(stdev);

    int thread_sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, thread_sockets) == -1) {
        ALOGE("%s: Failed to create socket pair", __func__);
        ret = errno;
        goto err;
    }
    stdev->send_socket = thread_sockets[0];
    stdev->term_socket = thread_sockets[1];

    stdev->uevent_socket = uevent_open_socket(64*1024, true);
    if (stdev->uevent_socket == -1) {
        ALOGE("%s: Failed to open uevent socket", __func__);
        ret = errno;
        goto err;
    }

    stdev->callback_thread_active = true;
    ret = pthread_create(&stdev->callback_thread, (const pthread_attr_t *) NULL,
                         callback_thread_loop, stdev);
    if (ret) {
        goto err;
    }
    return 0;

err:
    stdev->callback_thread_active = false;
    stdev_close_callback_thread_sockets(stdev);
    return -ret;
}

// Helper function to close (and mark as closed) all sockets used by the callback thread. Must be
// called with the stdev->lock held.
static void stdev_close_callback_thread_sockets(struct sound_trigger_device *stdev)
{
    ALOGV("%s", __func__);

    if (stdev->send_socket >=0) {
        close(stdev->send_socket);
        stdev->send_socket = -1;
    }
    if (stdev->term_socket >=0) {
        close(stdev->term_socket);
        stdev->term_socket = -1;
    }
    if (stdev->uevent_socket >= 0) {
        close(stdev->uevent_socket);
        stdev->uevent_socket = -1;
    }
}

// If the callback thread is active, stops it and joins the thread. Also closes all resources needed
// to talk to the thread. If keep_vts_powered is false, then the VTS will be shut down after the
// thread joins. Must be called with the stdev->lock held.
static void stdev_join_callback_thread(
        struct sound_trigger_device *stdev,
        bool keep_powered)
{
    ALOGV("%s", __func__);

    if (stdev->callback_thread_active) {
        // If the thread is active, send the termination signal and join up with it. Also, turn off
        // the VTS, since we're no longer listening for events. callback_thread_active will be set
        // to false when the thread joins.
        write(stdev->send_socket, "T", 1);
        pthread_mutex_unlock(&stdev->lock);
        ALOGD("%s: callback_thread join", __func__);
        pthread_join(stdev->callback_thread, (void **)NULL);
        ALOGD("%s: callback_thread join exit", __func__);
        pthread_mutex_lock(&stdev->lock);
    }
    if (!keep_powered) {
        ALOGE("%s:don't need to keep_powered", __func__);
#ifdef SUPPORT_BARGEIN_MODE
        if (stdev->is_bargein_mode_configed)
            disable_bargein(stdev);
        else if (stdev->is_mic_configured)
#endif
            stdev_vts_set_power(stdev, 0);
    }
    stdev_close_callback_thread_sockets(stdev);
}

// Loads a model file into the kernel. Must be called with the stdev->lock held.
static int vts_load_sound_model(
        struct sound_trigger_device *stdev __unused,
        char *buffer,
        size_t buffer_len,
        int model_index)
{
    int ret = 0;

    ALOGV("%s model_index: %d", __func__, model_index);

    if (model_index == HOTWORD_INDEX) {
        /* load Hotword model binary */
#ifdef MMAP_INTERFACE_ENABLED
        ret = load_modelbinary(stdev, model_index, buffer, buffer_len);
#else
        ret = sysfs_write(VTS_HOTWORD_MODEL, buffer, buffer_len);
#endif
    } else if (model_index == ODMVOICE_INDEX) {
        /* load ODMVoice model binary */
#ifdef MMAP_INTERFACE_ENABLED
        ret = load_modelbinary(stdev, model_index, buffer, buffer_len);
#else
        ret = sysfs_write(VTS_SVOICE_MODEL, buffer, buffer_len);
#endif
    } else {
        ALOGE("Can't determine model write ioctl %d", model_index);
        return -ENOSYS;
    }

    if (ret) {
        ALOGE("Error in VTS sysfs write : %d", ret);
    }

    return ret;
}

// Returns a bitmask where each bit is set if there is a recognition callback function set for that
// index. Must be called with the stdev->lock held.
static inline int stdev_active_callback_bitmask(struct sound_trigger_device* stdev)
{
    int bitmask = 0;
    int i;

    for (i = 0; i < MAX_SOUND_MODELS; ++i) {
        if (stdev->recognition_callbacks[i] != NULL) {
            bitmask |= (1 << i);
        }
    }

    ALOGV("%s, bitmask %d", __func__, bitmask);

    return bitmask;
}

static void error_recovery(struct sound_trigger_device* stdev) {
    int active_bitmask = stdev_active_callback_bitmask(stdev);
    ALOGI("%s hardfault issue happens", __func__);
    if (active_bitmask) {
        ALOGI("%s, recovery the VTS", __func__);
        stdev_vts_set_power(stdev, 0);
        stdev_vts_set_power(stdev, active_bitmask);
    }
}

static void *callback_thread_loop(void *context)
{
    char msg[UEVENT_MSG_LEN];
    struct sound_trigger_device *stdev =
               (struct sound_trigger_device *)context;
    struct sound_trigger_recognition_event *event = NULL;
    recognition_callback_t callbacks = NULL;
    sound_model_handle_t model = 0;
    void *cookies = NULL;
    int trigger_index = 0;
    int model_execstate = -1;
    struct pollfd fds[2];
    int kwDetected = 0;
    int err = 0;
    int i, n;

    ALOGI("%s", __func__);
    prctl(PR_SET_NAME, (unsigned long)"sound trigger callback", 0, 0, 0);

    pthread_mutex_lock(&stdev->lock);

    fds[0].events = POLLIN;
    fds[0].fd = stdev->uevent_socket;
    fds[1].events = POLLIN;
    fds[1].fd = stdev->term_socket;

    stdev->recog_cbstate = RECOG_CB_NONE;
    pthread_mutex_unlock(&stdev->lock);

    while (1) {
        err = poll(fds, 2, -1);
        pthread_mutex_lock(&stdev->lock);
        stdev->recog_cbstate = RECOG_CB_STARTED;
        if (err < 0) {
            ALOGE_IF(err < 0, "Error in poll: %d", err);
            break;
        }

        if (fds[0].revents & POLLIN) {
            n = uevent_kernel_multicast_recv(fds[0].fd, msg, UEVENT_MSG_LEN);
            if (n <= 0) {
                stdev->recog_cbstate = RECOG_CB_NONE;
                pthread_mutex_unlock(&stdev->lock);
                continue;
            }
            for (i = 0; i < n;) {
                if (!stdev->is_streaming &&
                        (strstr(msg + i, "VOICE_WAKEUP_WORD_ID=1") ||
                    strstr(msg + i, "VOICE_WAKEUP_WORD_ID=2") ||
                        strstr(msg + i, "VOICE_WAKEUP_WORD_ID=3"))) {

                    if(event != NULL)
                        free(event);

                    event = NULL;
                    trigger_index = 0;
                    kwDetected = 1;
                    err = 0;

                    ALOGD("%s onDetected!", __func__);

                    if (strstr(msg + i, "VOICE_WAKEUP_WORD_ID=1")||
                        strstr(msg + i, "VOICE_WAKEUP_WORD_ID=3")) {
                        int len = strlen("VOICE_WAKEUP_WORD_ID=1");
                        ALOGI("%s, len:%d, source:%s, uevent:%s", __func__, len, (msg + i+ len + 1), (msg + i));
#ifdef SUPPORT_BARGEIN_MODE
                        if (strstr(msg + i + len + 1, "SOURCE=BARGEIN") && stdev->is_bargein_mode_configed) {
                            ALOGI("%s, wakeup from bargein", __func__);
                            get_bargein_keyword_length(stdev);
                            i += strlen(msg + i) + 1; // two K-V in this case, skip one here
                        } else if (strstr(msg + i + len + 1, "SOURCE=VTS") && stdev->is_mic_configured) {
#else
                        if (strstr(msg + i + len + 1, "SOURCE=VTS") && stdev->is_mic_configured) {
#endif
                            ALOGI("%s, wakeup from VTS", __func__);
                            get_vts_keyword_length(stdev);
                            i += strlen(msg + i) + 1; // two K-V in this case, skip one here
                        } else {
                            int active_bitmask = stdev_active_callback_bitmask(stdev);
                            if (active_bitmask) {
 #ifdef SUPPORT_BARGEIN_MODE
                                if (stdev->requires_bargein_mode) {
                                    ALOGI("%s re-config bargein when sthal and driver not compatible", __func__);
                                    disable_bargein(stdev);
                                    enable_bargein(stdev,active_bitmask);
                                } else
#endif
                                {
                                    ALOGI("%s re-config vts when when sthal and driver not compatible", __func__);
                                    stdev_vts_set_power(stdev, 0);
                                    stdev_vts_set_power(stdev, active_bitmask);
                                }
                            }
                            i += strlen(msg + i) + 1;
                            continue;
                        }
                        stdev->is_streaming = (0x1 << ODMVOICE_INDEX);
                        //start thread to read keywords
                        ALOGD("%s stdev->is_streaming %d, err %d", __func__, stdev->is_streaming, err);

                        if (!err)
                            event = sound_trigger_odmvoice_event_alloc(stdev);

                            trigger_index = ODMVOICE_INDEX;
                    } else {
                            event = sound_trigger_hotword_event_alloc(stdev);
                            trigger_index = HOTWORD_INDEX;
                            ALOGI("%s HOTWORD Event Triggered --%d", __func__, trigger_index);
                    }

                    if (event != NULL) {
                        callbacks = stdev->recognition_callbacks[trigger_index];
                        cookies = stdev->recognition_cookies[trigger_index];
                        model_execstate = stdev->model_execstate[trigger_index];
                        model = stdev->model_handles[trigger_index];
                    }
                    stdev->recog_cbstate = RECOG_CB_CALLED;
                } else if (strstr(msg + i, "VTS_ERROR_RECOVERY")){
                    error_recovery(stdev);
                }
                i += strlen(msg + i) + 1;
            }
        } else if (fds[1].revents & POLLIN) {
            read(fds[1].fd, &n, sizeof(n)); /* clear the socket */
            stdev->recog_cbstate = RECOG_CB_NONE;
            ALOGI("%s: Termination message", __func__);
            break;
        } else {
            stdev->recog_cbstate = RECOG_CB_NONE;
            ALOGI("%s: Message to ignore", __func__);
        }

        pthread_mutex_unlock(&stdev->lock);

        if (kwDetected == 1) {
            kwDetected = 0;
                    if (event) {
                if (model_execstate == MODEL_STATE_RUNNING && callbacks != NULL) {
                            ALOGI("%s send callback model %d", __func__, trigger_index);
                    callbacks(event, cookies);
                        } else {
                            ALOGE("%s no callback for model %d", __func__, trigger_index);
                        }
                        free(event);
                event = NULL;

                pthread_mutex_lock(&stdev->lock);
                ALOGI("%s ODMVOICE Event Triggerred %d", __func__, trigger_index);
                stdev->recognition_callbacks[trigger_index] = NULL;

                        // Start reading data from the VTS while the upper levels do their thing.
                        if (stdev->model_execstate[trigger_index] == MODEL_STATE_RUNNING &&
                            stdev->configs[trigger_index] &&
                            stdev->configs[trigger_index]->capture_requested) {
                    ALOGI("%s Streaming Enabled for %s", __func__, (trigger_index ? "ODMVOICE" : "HOTWORD"));
                            stdev->is_streaming = (0x1 << trigger_index);
                    //goto exit;
                        } else {
                            /* Error handling if models stop-recognition failed */
                            if ((stdev->model_stopfailedhandles[HOTWORD_INDEX] != HANDLE_NONE) ||
                                (stdev->model_stopfailedhandles[ODMVOICE_INDEX] != HANDLE_NONE) ||
                                stdev->model_execstate[trigger_index] == MODEL_STATE_STOPABORT) {
                                handle_stop_recognition_l(stdev);
                                stdev->model_execstate[trigger_index] = MODEL_STATE_NONE;
                                ALOGI("%s stop-recognition error state handled", __func__);
                            }

                            // If we're not supposed to capture data, power cycle the VTS and start
                            // whatever algorithms are still active.
                            int active_bitmask = stdev_active_callback_bitmask(stdev);
                            if (active_bitmask) {
#ifdef SUPPORT_BARGEIN_MODE
                        if (stdev->is_bargein_mode_configed) {
                            ALOGI("%s re-config bargein mode", __func__);
                            disable_bargein(stdev);
                            enable_bargein(stdev,active_bitmask);
                        } else
#endif
                        {
                            ALOGI("%s re-config vts", __func__);
                                stdev_vts_set_power(stdev, 0);
                                stdev_vts_set_power(stdev, active_bitmask);
                            }
                        }
                    }
                stdev->recog_cbstate = RECOG_CB_NONE;
                pthread_mutex_unlock(&stdev->lock);
        } else {
                ALOGE("%s: invalid trigger or out of memory, failure_callback", __func__);

                pthread_mutex_lock(&stdev->lock);
                stdev->recognition_callbacks[trigger_index] = NULL;
        stdev->recog_cbstate = RECOG_CB_NONE;
        pthread_mutex_unlock(&stdev->lock);
    }
        }
    }

    if (!stdev->is_streaming) {
#ifdef SUPPORT_BARGEIN_MODE
        if (stdev->is_bargein_mode_configed) {
            ALOGI("%s disable bargein mode", __func__);
            disable_bargein(stdev);
        } else
#endif
        if (stdev->is_mic_configured)
        {
            stdev_vts_set_power(stdev, 0);
        }
    }

    stdev->callback_thread_active = false;
    stdev->recog_cbstate = RECOG_CB_NONE;
    pthread_mutex_unlock(&stdev->lock);
    ALOGI("%s: exit", __func__);

    return (void *)(long)err;
}
static void get_vts_keyword_length(struct sound_trigger_device *stdev)
{
    unsigned ret = 0;
    struct mixer_ctl *mixerctl = NULL;
    stdev->kw_length = KW_LENGTH;

    mixerctl = mixer_get_ctl_by_name(stdev->mixer, VTS_KWDLENGTH_CTL_NAME);
    if (mixerctl) {
        ALOGI("%s: get vts kwd length(%s)", __func__, VTS_KWDLENGTH_CTL_NAME);
        ret = mixer_ctl_get_value(mixerctl, 0);
        if (ret > 0 && ret <= KW_LENGTH) {
            stdev->kw_length = ret;
            ALOGI("%s: buffer size of the key word is %d", __func__, stdev->kw_length);
        } else {
            ALOGE("%s: failed to get %s , use default", __func__, VTS_KWDLENGTH_CTL_NAME);
        }
    } else {
        ALOGI("%s: (%s) not exist", __func__, VTS_KWDLENGTH_CTL_NAME);
    }
}
#ifdef SUPPORT_BARGEIN_MODE
static void set_bargein_keyword_type(struct sound_trigger_device *stdev)
{
    struct mixer_ctl *mixerctl = NULL;
    int keyword_type = 0;
    int ret = 0;

    if (stdev->sound_card_state == ABNORMAL_STATUS)
        return;

    mixerctl = mixer_get_ctl_by_name(stdev->mixer, BARGEIN_KWDTYPE_CTL_NAME);
    if (mixerctl) {
        if (stdev->kw_type == 1) // APP use 1 XVXV 2 HiJovi, vts use 2 XVXV 1 HiJovi
            keyword_type = 2;
        else if (stdev->kw_type == 2)
            keyword_type = 1;

        ALOGI("%s: configure bargein kwd type control(%s)", __func__, BARGEIN_KWDTYPE_CTL_NAME);
        ret = mixer_ctl_set_value(mixerctl, 0, keyword_type);
        if (ret != 0) {
            ALOGI("%s: failed to set the key word type for Barge-In", __func__);
        } else {
            ALOGE("%s: successfuly to set the key word type for Barge-In ", __func__);
        }
    } else {
        ALOGI("%s: (%s) not exist", __func__, BARGEIN_KWDTYPE_CTL_NAME);
    }
}

static void get_bargein_keyword_length(struct sound_trigger_device *stdev)
{
    unsigned ret = 0;
    struct mixer_ctl *mixerctl = NULL;
    stdev->kw_length = KW_LENGTH;

#ifdef SUPPORT_AUDIO_MONITOR
    if (stdev->sound_card_state == ABNORMAL_STATUS)
        return;
#endif

    mixerctl = mixer_get_ctl_by_name(stdev->mixer, BARGEIN_KWDLENGTH_CTL_NAME);
    if (mixerctl) {
        ALOGI("%s: get bargein kwd length(%s)", __func__, BARGEIN_KWDLENGTH_CTL_NAME);
        ret = mixer_ctl_get_value(mixerctl, 0);
        if (ret > 0 && ret <= KW_LENGTH) {
            stdev->kw_length = ret;
            ALOGI("%s: buffer size of the key word is %d", __func__, stdev->kw_length);
        } else {
            ALOGE("%s: failed to get %s, use default", __func__, BARGEIN_KWDLENGTH_CTL_NAME);
        }
    } else {
        ALOGI("%s: (%s) not exist", __func__, BARGEIN_KWDLENGTH_CTL_NAME);
    }
}
#endif

static int stdev_load_sound_model(
        const struct sound_trigger_hw_device *dev,
        struct sound_trigger_sound_model *sound_model,
        sound_model_callback_t callback,
        void *cookie,
        sound_model_handle_t *handle)
{
    struct sound_trigger_device *stdev = (struct sound_trigger_device *)dev;
    int ret = 0;
    int model_index = -1;

    ALOGI("%s", __func__);
    pthread_mutex_lock(&stdev->lock);
    if (handle == NULL || sound_model == NULL) {
        ALOGE("%s: handle or sound_model pointer NULL error", __func__);
        ret = -EINVAL;
        goto exit;
    }
    if (sound_model->data_size == 0 ||
            sound_model->data_offset < sizeof(struct sound_trigger_sound_model)) {
        ALOGE("%s: Model data size [%d] or data offset [%d] expected offset [%zu]  invalid",
                    __func__, sound_model->data_size, sound_model->data_offset,
                    sizeof(struct sound_trigger_sound_model));
        ret =  -EINVAL;
        goto exit;
    }

    /* TODO: Figure out what the model type is by looking at the UUID? */
    if (sound_model->type == SOUND_MODEL_TYPE_KEYPHRASE) {
        if (!memcmp(&sound_model->vendor_uuid, &odmword_uuid, sizeof(sound_trigger_uuid_t))) {
            model_index = ODMVOICE_INDEX;
            ALOGV("%s ODMVOICE_INDEX Sound Model", __func__);
        } else if (!memcmp(&sound_model->vendor_uuid, &hotword_uuid, sizeof(sound_trigger_uuid_t))) {
            model_index = HOTWORD_INDEX;
            ALOGV("%s HOTWORD_INDEX Sound Model", __func__);
        } else {
            ALOGE("%s Invalid UUID: {0x%x, 0x%x, 0x%x, 0x%x \n {0x%x 0x%x 0x%x 0x%x 0x%x 0x%x}}", __func__,
                sound_model->vendor_uuid.timeLow, sound_model->vendor_uuid.timeMid,
                sound_model->vendor_uuid.timeHiAndVersion, sound_model->vendor_uuid.clockSeq,
                sound_model->vendor_uuid.node[0], sound_model->vendor_uuid.node[1],
                sound_model->vendor_uuid.node[2], sound_model->vendor_uuid.node[3],
                sound_model->vendor_uuid.node[4], sound_model->vendor_uuid.node[5]);
            ret = -EINVAL;
            goto exit;
        }
    } else if (sound_model->type == SOUND_MODEL_TYPE_GENERIC) {
        if (!memcmp(&sound_model->vendor_uuid, &odmword_uuid, sizeof(sound_trigger_uuid_t))) {
            model_index = ODMVOICE_INDEX;
            ALOGV("%s ODMVOICE_INDEX Sound Model", __func__);
        } else {
            ALOGE("%s Generic Invalid UUID: {0x%x, 0x%x, 0x%x, 0x%x \n {0x%x 0x%x 0x%x 0x%x 0x%x 0x%x}}",
                __func__, sound_model->vendor_uuid.timeLow, sound_model->vendor_uuid.timeMid,
                sound_model->vendor_uuid.timeHiAndVersion, sound_model->vendor_uuid.clockSeq,
                sound_model->vendor_uuid.node[0], sound_model->vendor_uuid.node[1],
                sound_model->vendor_uuid.node[2], sound_model->vendor_uuid.node[3],
                sound_model->vendor_uuid.node[4], sound_model->vendor_uuid.node[5]);
            ret = -EINVAL;
            goto exit;
        }
    } else {
        ALOGE("%s: Could not determine model type", __func__);
        ret = -EINVAL;
        goto exit;
    }

    if (model_index < 0 || stdev->model_handles[model_index] != -1) {
        ALOGE("%s: unknown Model type or already running", __func__);
        ret = -ENOSYS;
        goto exit;
    }

    stdev->kw_type = 0;
#ifdef SUPPORT_BARGEIN_MODE
    set_bargein_keyword_type(stdev);
#endif
    ret = vts_load_sound_model(stdev, ((char *)sound_model) + sound_model->data_offset,
                               sound_model->data_size, model_index);
    if (ret) {
        goto exit;
    }

    stdev->model_handles[model_index] = model_index;
    stdev->sound_model_callbacks[model_index] = callback;
    stdev->sound_model_cookies[model_index] = cookie;
    *handle = stdev->model_handles[model_index];

exit:
    pthread_mutex_unlock(&stdev->lock);
    return ret;
}

static int stdev_unload_sound_model(
        const struct sound_trigger_hw_device *dev,
        sound_model_handle_t handle)
{
    struct sound_trigger_device *stdev = (struct sound_trigger_device *)dev;
    int ret = 0;

    ALOGI("%s handle: %d", __func__, handle);
    pthread_mutex_lock(&stdev->lock);
    if (handle < 0 || handle >= MAX_SOUND_MODELS) {
        ret = -EINVAL;
        goto exit;
    }
    if (stdev->model_handles[handle] != handle) {
        ret = -ENOSYS;
        goto exit;
    }

    // If we still have a recognition callback, that means we should cancel the
    // recognition first.
    if (stdev->recognition_callbacks[handle] != NULL ||
            (stdev->is_streaming & (0x1 << handle))) {
        ret = stdev_stop_recognition_l(stdev, handle);
    }
    stdev->model_handles[handle] = -1;

exit:
    pthread_mutex_unlock(&stdev->lock);
    ALOGI("%s exit", __func__);

    return ret;
}
#ifdef STHAL_2_3
static int stdev_start_recognition_extended(
        const struct sound_trigger_hw_device *dev,
        sound_model_handle_t handle,
        const struct sound_trigger_recognition_config_header *config,
        recognition_callback_t callback,
        void *cookie)
{
    struct sound_trigger_device *stdev = (struct sound_trigger_device *)dev;
    struct sound_trigger_recognition_config_extended_1_3 *configs = (struct sound_trigger_recognition_config_extended_1_3 *)config;
    int ret = 0;

    ALOGI("%s Handle %d", __func__, handle);

    pthread_mutex_lock(&stdev->lock);
    pthread_mutex_lock(&stdev->trigger_lock);

    if (handle < 0 || handle >= MAX_SOUND_MODELS || stdev->model_handles[handle] != handle) {
         ALOGE("%s: Handle doesn't match", __func__);
        ret = -ENOSYS;
        goto exit;
    }
    if (stdev->recognition_callbacks[handle] != NULL) {
         ALOGW("%s: model recognition is already started Checking for error state", __func__);
         /* Error handling if models stop-recognition failed */
         if ((stdev->model_stopfailedhandles[HOTWORD_INDEX] != HANDLE_NONE) ||
             (stdev->model_stopfailedhandles[ODMVOICE_INDEX] != HANDLE_NONE)) {
             handle_stop_recognition_l(stdev);
             ALOGI("%s stop-recognition error state handled", __func__);
         }
    }

    // Copy the config for this handle.
    if (config) {
        if (stdev->configs[handle]) {
            free(stdev->configs[handle]);
        }

        stdev->configs[handle] = malloc(sizeof(configs->base));
        if (!stdev->configs[handle]) {
            ret = -ENOMEM;
            goto exit;
        }

        memcpy(stdev->configs[handle], &configs->base, sizeof(configs->base));

        /* Check whether config has extra inforamtion */
        if (configs->base.data_size > 0) {
            char *params = (char*)configs + sizeof(*configs);
            // reset & update user data
            stdev->backlog_size = 0;
            stdev->odmvoicemodel_mode = ODMVOICE_UNKNOWN_MODE;
            if (params)
                ParseExtraConfigData(stdev, params);
        }
    }

    ret = stdev_start_callback_thread(stdev);
    if (ret) {
        goto exit;
    }

    stdev->recognition_callbacks[handle] = callback;
    stdev->recognition_cookies[handle] = cookie;

    ALOGI("%s, stdev->audio_input_state:%d, stdev->is_streaming:%d\n", __func__, stdev->audio_input_state, stdev->is_streaming);
    // Reconfigure to run any algorithm that have a callback.
    if (!stdev->is_streaming || (stdev->is_streaming & (0x1 << handle))) {
        int active_bitmask = stdev_active_callback_bitmask(stdev);
        ALOGI("Starting Voice Recognition,stdev->voicecall_state:%d, stdev->audio_input_state:%d\n", stdev->voicecall_state, stdev->audio_input_state);

        if (stdev->voicecall_state == VOICECALL_STARTED || stdev->audio_input_state) {
            if (active_bitmask & (0x1 << HOTWORD_INDEX)) {
                stdev->recognize_started |= (0x1 << HOTWORD_INDEX);
                ALOGI("%s: Google Model recognization started", __func__);
            }

            if (active_bitmask & (0x1 << ODMVOICE_INDEX)) {
                stdev->recognize_started |= (0x1 << ODMVOICE_INDEX);
                ALOGI("%s: ODMVoice Model recognition mode started", __func__);
            }
        } else {
#ifdef SUPPORT_BARGEIN_MODE
            ALOGI("%s: Starting Voice Recognition,requires_bargein_mode:%d\n",
                   __func__, stdev->requires_bargein_mode);

            if (stdev->requires_bargein_mode) {
                ALOGI("%s: re-config Bargein mode:%d\n", __func__, stdev->is_bargein_mode_configed);
                if (stdev->is_bargein_mode_configed)
                    disable_bargein(stdev);

                enable_bargein(stdev, stdev_active_callback_bitmask(stdev));
            } else
#endif
            {
                ALOGI("%s: re-config for VTS Recognition", __func__);
                stdev_vts_set_power(stdev, 0);
                stdev_vts_set_power(stdev, stdev_active_callback_bitmask(stdev));
            }
        }
    }

    stdev->model_stopfailedhandles[handle] = HANDLE_NONE;
    stdev->model_execstate[handle] = MODEL_STATE_RUNNING;
    ALOGI("%s Handle Exit %d", __func__, handle);

exit:
    pthread_mutex_unlock(&stdev->trigger_lock);
    pthread_mutex_unlock(&stdev->lock);
    return ret;
}
#endif
static int stdev_start_recognition(
        const struct sound_trigger_hw_device *dev,
        sound_model_handle_t handle,
        const struct sound_trigger_recognition_config *config,
        recognition_callback_t callback,
        void *cookie)
{
    struct sound_trigger_device *stdev = (struct sound_trigger_device *)dev;
    int ret = 0;

    ALOGI("%s Handle %d", __func__, handle);

    pthread_mutex_lock(&stdev->lock);
    pthread_mutex_lock(&stdev->trigger_lock);

    if (handle < 0 || handle >= MAX_SOUND_MODELS || stdev->model_handles[handle] != handle) {
         ALOGE("%s: Handle doesn't match", __func__);
        ret = -ENOSYS;
        goto exit;
    }
    if (stdev->recognition_callbacks[handle] != NULL) {
         ALOGW("%s:model recognition is already started Checking for error state", __func__);
         /* Error handling if models stop-recognition failed */
         if ((stdev->model_stopfailedhandles[HOTWORD_INDEX] != HANDLE_NONE) ||
             (stdev->model_stopfailedhandles[ODMVOICE_INDEX] != HANDLE_NONE)) {
             handle_stop_recognition_l(stdev);
             ALOGI("%s stop-recognition error state handled", __func__);
         }
    }

    // Copy the config for this handle.
    if (config) {
        if (stdev->configs[handle]) {
            free(stdev->configs[handle]);
        }
        stdev->configs[handle] = malloc(sizeof(*config));
        if (!stdev->configs[handle]) {
            ret = -ENOMEM;
            goto exit;
        }
        memcpy(stdev->configs[handle], config, sizeof(*config));

        /* Check whether config has extra inforamtion */
        if (config->data_size > 0) {
            char *params = (char*)config + sizeof(*config);
            // reset & update user data
            stdev->backlog_size = 0;
            stdev->odmvoicemodel_mode = ODMVOICE_UNKNOWN_MODE;
            if (params)
                ParseExtraConfigData(stdev, params);
        }
    }

    ret = stdev_start_callback_thread(stdev);
    if (ret) {
        goto exit;
    }

    stdev->recognition_callbacks[handle] = callback;
    stdev->recognition_cookies[handle] = cookie;

    ALOGI("%s, stdev->audio_input_state:%d, stdev->is_streaming:%d\n", __func__, stdev->audio_input_state, stdev->is_streaming);
    // Reconfigure to run any algorithm that have a callback.
    if (!stdev->is_streaming || (stdev->is_streaming & (0x1 << handle))) {
        int active_bitmask = stdev_active_callback_bitmask(stdev);
        ALOGI("Starting Voice Recognition,stdev->voicecall_state:%d, stdev->audio_input_state:%d\n", stdev->voicecall_state, stdev->audio_input_state);

        if (stdev->voicecall_state == VOICECALL_STARTED || stdev->audio_input_state) {
            if (active_bitmask & (0x1 << HOTWORD_INDEX)) {
                stdev->recognize_started |= (0x1 << HOTWORD_INDEX);
                ALOGI("%s: Google Model recognization started", __func__);
            }

            if (active_bitmask & (0x1 << ODMVOICE_INDEX)) {
                stdev->recognize_started |= (0x1 << ODMVOICE_INDEX);
                ALOGI("%s: ODMVoice Model recognition mode started", __func__);
            }
        } else {
#ifdef SUPPORT_BARGEIN_MODE
            ALOGI("%s: Starting Voice Recognition,requires_bargein_mode:%d\n",
                   __func__, stdev->requires_bargein_mode);

            if (stdev->requires_bargein_mode) {
                ALOGI("%s: re-config Bargein mode:%d\n", __func__, stdev->is_bargein_mode_configed);
                if (stdev->is_bargein_mode_configed)
                    disable_bargein(stdev);

                enable_bargein(stdev, stdev_active_callback_bitmask(stdev));
            } else
#endif
            {
                ALOGI("%s: re-config for VTS Recognition", __func__);
                stdev_vts_set_power(stdev, 0);
                stdev_vts_set_power(stdev, stdev_active_callback_bitmask(stdev));
            }
        }
    }

    stdev->model_stopfailedhandles[handle] = HANDLE_NONE;
    stdev->model_execstate[handle] = MODEL_STATE_RUNNING;
    ALOGI("%s Handle Exit %d", __func__, handle);
exit:
    pthread_mutex_unlock(&stdev->trigger_lock);
    pthread_mutex_unlock(&stdev->lock);
    return ret;
}

static int stdev_stop_recognition(
        const struct sound_trigger_hw_device *dev,
        sound_model_handle_t handle)
{
    struct sound_trigger_device *stdev = (struct sound_trigger_device *)dev;
    int ret = 0;

    ALOGI("%s Handle %d", __func__, handle);

    /* Error handling to avoid ST HWservice Framework deadlock situation */
    if (pthread_mutex_trylock(&stdev->lock)) {
        int retry_count = 0;
        do {
            retry_count++;
            if (stdev->recog_cbstate == RECOG_CB_CALLED) {
                if (handle < 0 || handle >= MAX_SOUND_MODELS || stdev->model_handles[handle] != handle) {
                    ALOGE("%s: Recognition Even CallBack function called - Handle doesn't match", __func__);
                    return -ENOSYS;
                }
                ALOGI("%s Handle %d Recognition Even CallBack function called",
                            __func__, handle);
                stdev->model_stopfailedhandles[handle] = handle;
                stdev->model_execstate[handle] = MODEL_STATE_STOPABORT;
                return -EBUSY;
            }

            if (!(retry_count % 25))
                ALOGI("%s Handle %d Trylock retry count %d!!", __func__, handle, retry_count);

            if (retry_count > 100) {
                ALOGE("%s Handle %d Unable to Acquire Lock", __func__, handle);
                stdev->model_stopfailedhandles[handle] = handle;
                stdev->model_execstate[handle] = MODEL_STATE_STOPABORT;
                return -ENOSYS;
            }
            usleep(1000);  // wait for 1msec before retrying
        } while (pthread_mutex_trylock(&stdev->lock));
    } else
        ALOGV("%s Handle %d Trylock acquired successfully", __func__, handle);

    pthread_mutex_lock(&stdev->trigger_lock);
    ALOGV("%s Handle %d Trigger-Trylock successfully", __func__, handle);

    ret = stdev_stop_recognition_l(stdev, handle);

    pthread_mutex_unlock(&stdev->trigger_lock);
    pthread_mutex_unlock(&stdev->lock);
    ALOGI("%s Handle Exit %d", __func__, handle);
    return ret;
}

static int stdev_stop_recognition_l(
        struct sound_trigger_device *stdev,
        sound_model_handle_t handle)
{
    int active_bitmask;
    ALOGV("%s", __func__);
    //stop to read data

    if (handle < 0 || handle >= MAX_SOUND_MODELS || stdev->model_handles[handle] != handle) {
        ALOGE("%s: Handle doesn't match", __func__);
        return -ENOSYS;
    }

    if (stdev->configs[handle] != NULL) {
    free(stdev->configs[handle]);
    stdev->configs[handle] = NULL;
    }
    stdev->recognition_callbacks[handle] = NULL;
    active_bitmask = stdev_active_callback_bitmask(stdev);
    ALOGD("%s active_bitmask %d", __func__, active_bitmask);

        if (active_bitmask == 0) {
            stdev_join_callback_thread(stdev, false);
        } else {
#ifdef SUPPORT_BARGEIN_MODE
        if (stdev->requires_bargein_mode) {
            ALOGD("%s, Re-Start Bargein mode",__func__);
            if (stdev->is_bargein_mode_configed)
                disable_bargein(stdev);

            enable_bargein(stdev, active_bitmask);
        } else
#endif
        {
            ALOGD("%s, reconfig VTS",__func__);
            stdev_vts_set_power(stdev, 0);
            stdev_vts_set_power(stdev, active_bitmask);
        }
    }

    stdev->model_stopfailedhandles[handle] = HANDLE_NONE;
    stdev->model_execstate[handle] = MODEL_STATE_NONE;
    return 0;
}

/* calling function should acquire stdev lock */
static void handle_stop_recognition_l(struct sound_trigger_device *stdev)
{
    int i;

    ALOGV("%s", __func__);

    for (i = 0; i < MAX_SOUND_MODELS; ++i) {
        if (stdev->model_execstate[i] == MODEL_STATE_STOPABORT &&
            stdev->model_stopfailedhandles[i] == i) {
            if (stdev->recognition_callbacks[i] == NULL &&
                    !(stdev->is_streaming & (0x1 << i))) {
                ALOGI("%s:model recognition is already stopped", __func__);
            } else {
                if (stdev->configs[i])
                    free(stdev->configs[i]);
                stdev->configs[i] = NULL;
                stdev->recognition_callbacks[i] = NULL;
                ALOGI("%s:model recognition callback released", __func__);
            }

            // If we're streaming, then we shouldn't touch the VTS's current state.
            if (!stdev->is_streaming || (stdev->is_streaming & (0x1 << i))) {
                // force stop recognition
#ifdef SUPPORT_BARGEIN_MODE
                if (stdev->is_bargein_mode_configed)
                    disable_bargein(stdev);
                else if (stdev->is_mic_configured)
#endif
                    stdev_vts_set_power(stdev, 0);
#ifdef SUPPORT_BARGEIN_MODE
                ALOGI("%s:model recognition force stopped,stdev->requires_bargein_mode:%d", __func__,stdev->requires_bargein_mode);
#endif
                // reconfigure if other model is running.
                int active_bitmask = stdev_active_callback_bitmask(stdev);
                if (active_bitmask) {
#ifdef SUPPORT_BARGEIN_MODE
                    if (stdev->requires_bargein_mode) {
                        enable_bargein(stdev, active_bitmask);
                    } else
#endif
                    {
                    stdev_vts_set_power(stdev, active_bitmask);
                }
            }
        }
        }

        stdev->model_execstate[i] = MODEL_STATE_NONE;
        stdev->model_stopfailedhandles[i] = HANDLE_NONE;
    }

    return;
}

__attribute__ ((visibility ("default")))
int sound_trigger_open_for_streaming()
{
    struct sound_trigger_device *stdev = &g_stdev;
    int ret = 0;
    char fn[256];

    ALOGV("%s", __func__);

    pthread_mutex_lock(&stdev->lock);
    pthread_mutex_lock(&stdev->trigger_lock);

    if (!stdev->sthal_opened) {
        ALOGE("%s: stdev has not been opened", __func__);
        ret = -EFAULT;
        goto exit;
    }

    if (stdev->voicecall_state == VOICECALL_STARTED) {
        ALOGI("%s VoiceCall in progress", __func__);
        ret = -EBUSY;
        goto exit;
    }

    if (!stdev->is_streaming) {
        ALOGE("%s: VTS is not streaming currently", __func__);
        ret = -EBUSY;
        goto exit;
    }

    if (stdev->is_seamless_recording) {
        ALOGE("%s: VTS is already seamless recording currently", __func__);
        ret = -EBUSY;
        goto exit;
    }

    snprintf(fn, sizeof(fn), "/dev/snd/pcmC%uD%u%c", VTS_SOUND_CARD, VTS_TRICAP_DEVICE_NODE, 'c');
    ALOGI("%s: Opening PCM Device %s", __func__, fn);

    /* open vts streaming PCM node */
    stdev->streaming_pcm = pcm_open(VTS_SOUND_CARD, VTS_TRICAP_DEVICE_NODE, PCM_IN, &pcm_config_vt_capture);
    if (stdev->streaming_pcm && !pcm_is_ready(stdev->streaming_pcm)) {
        ALOGE("%s: failed to open streaming PCM (%s)", __func__, pcm_get_error(stdev->streaming_pcm));
        ret = -EFAULT;
        goto exit;
    }

    stdev->is_seamless_recording = true;
    ret = 1;
exit:
    pthread_mutex_unlock(&stdev->trigger_lock);
    pthread_mutex_unlock(&stdev->lock);
    return ret;
}

__attribute__ ((visibility ("default")))
size_t sound_trigger_read_samples(
        int audio_handle,
        void *buffer,
        size_t  buffer_len)
{
    struct sound_trigger_device *stdev = &g_stdev;
//    int i;
    size_t ret = 0;

    //ALOGV("%s", __func__);

    if (audio_handle <= 0) {
        ALOGE("%s: invalid audio handle", __func__);
        return -EINVAL;
    }

    pthread_mutex_lock(&stdev->trigger_lock);
    if (!stdev->sthal_opened) {
        ALOGE("%s: stdev has not been opened", __func__);
        ret = -EFAULT;
        goto exit;
    }

    if (stdev->voicecall_state == VOICECALL_STARTED) {
        ALOGI("%s VoiceCall in progress", __func__);
        ret = -EBUSY;
        goto exit;
    }

    if (!stdev->is_streaming) {
        ALOGE("%s: VTS is not streaming currently", __func__);
        ret = -EINVAL;
        goto exit;
    }

    if(stdev->streaming_pcm)
        ret = pcm_read(stdev->streaming_pcm, buffer, buffer_len);
    if (ret == 0) {
        ALOGVV("%s: --Sent %zu bytes to buffer", __func__, buffer_len);
    } else {
        ret = 0;
        ALOGE("%s: Read Fail = %s", __func__, pcm_get_error(stdev->streaming_pcm));
    }

exit:
    pthread_mutex_unlock(&stdev->trigger_lock);
    return ret;
}

__attribute__ ((visibility ("default")))
int sound_trigger_close_for_streaming(int audio_handle __unused)
{
    struct sound_trigger_device *stdev = &g_stdev;
//    int i;
    size_t ret = 0;

    ALOGV("%s", __func__);

    if (audio_handle <= 0) {
        ALOGE("%s: invalid audio handle", __func__);
        return -EINVAL;
    }

    pthread_mutex_lock(&stdev->lock);
    pthread_mutex_lock(&stdev->trigger_lock);

    if (!stdev->sthal_opened) {
        ALOGE("%s: stdev has not been opened", __func__);
        ret = -EFAULT;
        goto exit;
    }

    if (stdev->voicecall_state == VOICECALL_STARTED) {
        ALOGI("%s VoiceCall in progress", __func__);
        ret = -EBUSY;
        goto exit;
    }

    if (!stdev->is_seamless_recording) {
        ALOGE("%s: VTS Seamless Recording PCM Node is not opened", __func__);
        ret = -EINVAL;
        goto exit;
    }

    if (!stdev->is_streaming) {
        ALOGE("%s: VTS is not currently streaming", __func__);
        ret = -EINVAL;
        goto exit;
    }

    /* close streaming pcm node */
    pcm_close(stdev->streaming_pcm);
    stdev->streaming_pcm = NULL;

    stdev->is_seamless_recording = false;
    // Power off the VTS, but then re-enable any algorithms that have callbacks.
    int active_bitmask = stdev_active_callback_bitmask(stdev);
#ifdef SUPPORT_BARGEIN_MODE
    if (stdev->is_bargein_mode_configed) {
        ALOGI("disable bargein mode, then re-enable algorithm for active callback,active_bitmask:%d",active_bitmask);
        disable_bargein(stdev);
        if (active_bitmask) {
            stdev_start_callback_thread(stdev);
            enable_bargein(stdev, active_bitmask);
        }
    } else
#endif
    {
        ALOGI("disable vts, then re-enable algorithm for active callback,active_bitmask:%d",active_bitmask);
    stdev_vts_set_power(stdev, 0);
    if (active_bitmask) {
        stdev_start_callback_thread(stdev);
        stdev_vts_set_power(stdev, active_bitmask);
    }
    }
exit:
    pthread_mutex_unlock(&stdev->trigger_lock);
    pthread_mutex_unlock(&stdev->lock);
    return ret;
}

/* VTS recording sthal interface */
__attribute__ ((visibility ("default")))
int sound_trigger_open_recording()
{
    struct sound_trigger_device *stdev = &g_stdev;
    int ret = 0;
    char fn[256];

    ALOGV("%s", __func__);

    pthread_mutex_lock(&stdev->lock);
    pthread_mutex_lock(&stdev->recording_lock);

    if (!stdev->sthal_opened) {
        ALOGE("%s: stdev has not been opened", __func__);
        ret = -EFAULT;
        goto exit;
    }

    if (stdev->voicecall_state == VOICECALL_STARTED) {
        ALOGI("%s VoiceCall in progress", __func__);
        ret = -EBUSY;
        goto exit;
    }

    if (stdev->is_recording) {
        ALOGW("%s: VTS is already recording currently", __func__);

        /* workaround to forcefully close current execution */
        /* close streaming pcm node */
        if(stdev->recording_pcm) {
            pcm_close(stdev->recording_pcm);
            stdev->recording_pcm = NULL;
        }

        /* disable VTS MIC controls */
        int active_bitmask = stdev_active_callback_bitmask(stdev);
        if (!active_bitmask && !stdev->is_streaming) {
            stdev_vts_set_mic(stdev, false);
        }
        stdev->is_recording = false;
        ALOGI("%s: Forcefully closed current recording", __func__);
    }

    /* Check & enable VTS MIC controls */
    stdev_vts_set_mic(stdev, true);

    snprintf(fn, sizeof(fn), "/dev/snd/pcmC%uD%u%c", VTS_SOUND_CARD, VTS_RECORD_DEVICE_NODE, 'c');
    ALOGI("%s: Opening PCM Device %s", __func__, fn);

    /* open vts streaming PCM node */
    stdev->recording_pcm = pcm_open(VTS_SOUND_CARD, VTS_RECORD_DEVICE_NODE, PCM_IN, &pcm_config_vt_capture);
    if (stdev->recording_pcm && !pcm_is_ready(stdev->recording_pcm)) {
        ALOGE("%s: failed to open recording PCM (%s)", __func__, pcm_get_error(stdev->recording_pcm));
        ret = -EFAULT;
        goto exit;
    }

    stdev->is_recording = true;
    ret = 1;
exit:
    pthread_mutex_unlock(&stdev->recording_lock);
    pthread_mutex_unlock(&stdev->lock);
    return ret;
}

__attribute__ ((visibility ("default")))
size_t sound_trigger_read_recording_samples(
        void *buffer,
        size_t  buffer_len)
{
    struct sound_trigger_device *stdev = &g_stdev;
    // int i;
    size_t ret = 0;

    //ALOGV("%s", __func__);
    pthread_mutex_lock(&stdev->recording_lock);

    if (!stdev->sthal_opened) {
        ALOGE("%s: stdev has not been opened", __func__);
        ret = -EFAULT;
        goto exit;
    }

    if (stdev->voicecall_state == VOICECALL_STARTED) {
        ALOGI("%s VoiceCall in progress", __func__);
        ret = -EBUSY;
        goto exit;
    }

    if (!stdev->is_recording) {
        ALOGE("%s: VTS Recording PCM Node is not opened", __func__);
        ret = -EINVAL;
        goto exit;
    }

    if(stdev->recording_pcm)
        ret = pcm_read(stdev->recording_pcm, buffer, buffer_len);
    if (ret == 0) {
        ALOGVV("%s: --Sent %zu bytes to buffer", __func__, buffer_len);
    } else {
        ALOGE("%s: Read Fail = %s", __func__, pcm_get_error(stdev->recording_pcm));
    }

exit:
    pthread_mutex_unlock(&stdev->recording_lock);
    return ret;
}

__attribute__ ((visibility ("default")))
int sound_trigger_close_recording()
{
    struct sound_trigger_device *stdev = &g_stdev;
//    int i;
    size_t ret = 0;

    ALOGV("%s", __func__);

    pthread_mutex_lock(&stdev->lock);
    pthread_mutex_lock(&stdev->recording_lock);

    if (!stdev->sthal_opened) {
        ALOGE("%s: stdev has not been opened", __func__);
        ret = -EFAULT;
        goto exit;
    }

    if (stdev->voicecall_state == VOICECALL_STARTED) {
        ALOGI("%s VoiceCall in progress", __func__);
        ret = -EBUSY;
        goto exit;
    }

    /* Error handling if models stop-recognition failed */
    if ((stdev->model_stopfailedhandles[HOTWORD_INDEX] != HANDLE_NONE) ||
        (stdev->model_stopfailedhandles[ODMVOICE_INDEX] != HANDLE_NONE)) {
        handle_stop_recognition_l(stdev);
        ALOGI("%s stop-recognition error state handled", __func__);
    }

    if (!stdev->is_recording) {
        ALOGE("%s: VTS Recording PCM Node is not opened", __func__);
        ret = -EINVAL;
        goto exit;
    }

    /* close streaming pcm node */
    pcm_close(stdev->recording_pcm);
    stdev->recording_pcm = NULL;

    /* disable VTS MIC controls */
    int active_bitmask = stdev_active_callback_bitmask(stdev);
    if (!active_bitmask && !stdev->is_streaming) {
        stdev_vts_set_mic(stdev, false);
    }

    stdev->is_recording = false;
exit:
    pthread_mutex_unlock(&stdev->recording_lock);
    pthread_mutex_unlock(&stdev->lock);
    return ret;
}

__attribute__ ((visibility ("default")))
int sound_trigger_headset_status(int is_connected)
{
    struct sound_trigger_device *stdev = &g_stdev;
    int active_bitmask = 0;
    int ret = 0;

    pthread_mutex_lock(&stdev->lock);

    goto exit;  //need to debug further for headset

    if (!stdev->sthal_opened) {
        ALOGE("%s: stdev has not been opened", __func__);
        ret = -EFAULT;
        goto exit;
    }
#ifdef SUPPORT_BARGEIN_MODE
    //maybe called during training, so can't add recognize_started condition
    if (stdev->is_bargein_mode_configed) {
        ALOGE("%s, just need to change status when in bargein mode",__func__);
        if (is_connected)
            stdev->active_mic = VTS_HEADSET_MIC;
        else
            stdev->active_mic = VTS_MAIN_MIC;

        goto exit;
    }
#endif
    /* check whether vts mic is configured or not */
    if (stdev->is_mic_configured) {
        int tmp_streaming = stdev->is_streaming;
        int tmp_recording = stdev->is_recording;
        int tmp_seamlessrecording = stdev->is_seamless_recording;
        //Check if recording is in progress
        if (stdev->is_recording) {
            ALOGI("%s: Close VTS Record PCM to reconfigure active Mic", __func__);
            // Close record PCM before changing MIC
            if (stdev->recording_pcm) {
                pthread_mutex_lock(&stdev->recording_lock);
                pcm_close(stdev->recording_pcm);
                stdev->recording_pcm = NULL;
                pthread_mutex_unlock(&stdev->recording_lock);
            }
            stdev->is_recording = false;
        }

        //Check if seamless capture is in progress
        pthread_mutex_lock(&stdev->streaming_pcm_lock);
        if (stdev->is_streaming) {
            ALOGI("%s: Close VTS Seamless PCM to reconfigure active Mic", __func__);
            // Close seamless PCM before changing MIC
            if (stdev->streaming_pcm) {
                pthread_mutex_lock(&stdev->trigger_lock);
                pcm_close(stdev->streaming_pcm);
                stdev->streaming_pcm = NULL;
                pthread_mutex_unlock(&stdev->trigger_lock);
            }
            stdev->is_seamless_recording = false;
        }
        pthread_mutex_unlock(&stdev->streaming_pcm_lock);

        // Power off the VTS, but then re-enable any algorithms that have callbacks.
        active_bitmask = stdev_active_callback_bitmask(stdev);
        stdev_vts_set_power(stdev, 0);
        /* update active mic only after disabling previous mic configuraiton */
        if (is_connected)
            stdev->active_mic = VTS_HEADSET_MIC;
        else
            stdev->active_mic = VTS_MAIN_MIC;

        ALOGI("%s: Active MIC Changed to [%s] Active Models: 0x%x", __func__,
                    (is_connected ? "HEADSET MIC" : "MAIN MIC"), active_bitmask);

        // Restore recording status
        stdev->is_recording = tmp_recording;

        if (active_bitmask || tmp_streaming) {
            active_bitmask |= tmp_streaming;
            ALOGI("%s: Re-started Models: 0x%x", __func__, active_bitmask);
            stdev_vts_set_power(stdev, active_bitmask);
            stdev->is_streaming = tmp_streaming;
            stdev->is_seamless_recording = tmp_seamlessrecording;
        }

        //Check if recording enabled then start again
        if (stdev->is_recording) {
            ALOGI("%s: Re-route active Mic for recording", __func__);
            /* Check & enable VTS MIC controls */
            stdev_vts_set_mic(stdev, true);

            /* open vts streaming PCM node */
            if (!stdev->recording_pcm) {
                stdev->recording_pcm = pcm_open(VTS_SOUND_CARD, VTS_RECORD_DEVICE_NODE, PCM_IN, &pcm_config_vt_capture);
                if (stdev->recording_pcm && !pcm_is_ready(stdev->recording_pcm)) {
                    ALOGE("%s: failed to open recording PCM", __func__);
                    ret = -EFAULT;
                    goto exit;
                }
            }
            ALOGI("%s: VTS Record reconfiguration & open PCM Completed", __func__);
        }

        //Check if seamless capture enable then start again
        if (stdev->is_streaming) {
            /* open vts streaming PCM node */
            if (stdev->is_seamless_recording && !stdev->streaming_pcm) {
                stdev->streaming_pcm = pcm_open(VTS_SOUND_CARD, VTS_TRICAP_DEVICE_NODE, PCM_IN, &pcm_config_vt_capture);
                if (stdev->streaming_pcm && !pcm_is_ready(stdev->streaming_pcm)) {
                    ALOGE("%s: failed to open streaming PCM", __func__);
                    if (stdev->recording_pcm) {
                        pthread_mutex_lock(&stdev->recording_lock);
                        pcm_close(stdev->recording_pcm);
                        stdev->recording_pcm = NULL;
                        pthread_mutex_unlock(&stdev->recording_lock);
                    }
                    ret = -EFAULT;
                    goto exit;
                }
            }
        }
    } else {
        /* update the active mic information */
        if (is_connected)
            stdev->active_mic = VTS_HEADSET_MIC;
        else
            stdev->active_mic = VTS_MAIN_MIC;
    }

exit:
    pthread_mutex_unlock(&stdev->lock);
    return ret;
}

__attribute__ ((visibility ("default")))
int sound_trigger_voicecall_status(int callstate)
{
    struct sound_trigger_device *stdev = &g_stdev;
    int ret = 0;

    ALOGI("%s, callstate %d", __func__, callstate);
    pthread_mutex_lock(&stdev->lock);

    if (!stdev->sthal_opened) {
        ALOGE("%s: stdev has not been opened", __func__);
        ret = -EFAULT;
        goto exit;
    }

    if (stdev->voicecall_state == callstate) {
        ALOGE("%s: same as previous state", __func__);
        ret = 0;
        goto exit;
    }

    stdev->voicecall_state = callstate;

    if (stdev->audio_input_state) {
        ALOGE("%s: audio_input_state is true, can't start Recongition", __func__);
        goto exit;
    }

    if (!stdev->recognize_started && (callstate == VOICECALL_STOPPED)) {
        ALOGE("%s: voice trigger function not started", __func__);
        goto exit;
    }

    /* check whether vts mic is configured or not */
    if (callstate == VOICECALL_STARTED) {
        int tmp_recognize_started = stdev->recognize_started;

        //Check if recording capture is in progress
        if (stdev->is_recording) {
            ALOGI("%s: Close VTS Record PCM to reconfigure active Mic", __func__);
            // Close record PCM before changing MIC
            if (stdev->recording_pcm) {
                pthread_mutex_lock(&stdev->recording_lock);
                pcm_close(stdev->recording_pcm);
                stdev->recording_pcm = NULL;
                pthread_mutex_unlock(&stdev->recording_lock);
            }
            stdev->is_recording = false;
        }

        //Check if seamless capture is in progress
        pthread_mutex_lock(&stdev->streaming_pcm_lock);
        if (stdev->is_streaming) {
            ALOGI("%s: Close VTS Seamless PCM to reconfigure active Mic", __func__);
            // Close seamless PCM before changing MIC
            if (stdev->streaming_pcm) {
                pthread_mutex_lock(&stdev->trigger_lock);
                pcm_close(stdev->streaming_pcm);
                stdev->streaming_pcm = NULL;
                pthread_mutex_unlock(&stdev->trigger_lock);
            }
            stdev->is_seamless_recording = false;
            stdev->is_streaming = false;
        }
        pthread_mutex_unlock(&stdev->streaming_pcm_lock);
#ifdef SUPPORT_BARGEIN_MODE
        if (stdev->is_bargein_mode_configed) {
            disable_bargein(stdev);
        } else
#endif
        {
            if (stdev->is_mic_configured)
                stdev_vts_set_power(stdev, 0);
        }
        stdev->recognize_started = tmp_recognize_started;
        ALOGI("%s: VoiceCall START notification received,stdev->recognize_started:%d", __func__,stdev->recognize_started);
    } else {
        int active_bitmask = stdev_active_callback_bitmask(stdev);
        ALOGI("%s: VoiceCall STOP notification received,active_bitmask:%d", __func__, active_bitmask);
        if (active_bitmask) {
#ifdef SUPPORT_BARGEIN_MODE
            if (stdev->requires_bargein_mode) {
                enable_bargein(stdev,active_bitmask);
            } else
#endif
            {
                stdev_vts_set_power(stdev, active_bitmask);
            }
        } else
            ALOGE("%s, error status which shouldn't happen",__func__);
    }

exit:
    pthread_mutex_unlock(&stdev->lock);
    ALOGI("%s, exit", __func__);
    return ret;
}

int set_bargein(int state) {
    struct sound_trigger_device *stdev = &g_stdev;
    int ret = 0;
    int active_bitmask = 0;
    ALOGD("%s, state %d", __func__, state);

#ifdef SUPPORT_BARGEIN_MODE
    if (stdev->requires_bargein_mode == state) {
        ret = state;
        ALOGD("%s: stdev->requires_bargein_mode %d, no need change", __func__, stdev->requires_bargein_mode);
        goto exit;
    }
    stdev->requires_bargein_mode = state;
#endif
    if (!stdev->recognize_started || stdev->voicecall_state) {
        ALOGD("%s: wakeup off, no need to change state", __func__);
        goto exit;
    }

    if (stdev->is_streaming) {
        ALOGD("%s: detected state, no need to change state", __func__);
        goto exit;
    }

    if (stdev->audio_input_state) {
        ALOGD("%s: there's normal recording, can't set bargein", __func__);
        goto exit;
    }

    if (state) {
        if (stdev->is_mic_configured) {
            if (stdev->is_recording) {
                ALOGI("%s: Close VTS Record PCM to switch to ABOX Bargein mode", __func__);
                if (stdev->recording_pcm) {
                    pcm_close(stdev->recording_pcm);
                    stdev->recording_pcm = NULL;
                }
                stdev->is_recording = false;
            }

            if (stdev->is_streaming) {
                    ALOGI("%s: Close VTS Seamless PCM to switch to ABOX bargein mode", __func__);
                if (stdev->streaming_pcm) {
                    pcm_close(stdev->streaming_pcm);
                    stdev->streaming_pcm = NULL;
                }

                stdev->is_seamless_recording = false;
                stdev->is_streaming = false;
            }

            stdev_vts_set_power(stdev, 0);
        }

        active_bitmask = stdev_active_callback_bitmask(stdev);
#ifdef SUPPORT_BARGEIN_MODE
        enable_bargein(stdev, active_bitmask);
#endif
        ret = 1;
    } else {
        int tmp_recognize_started = stdev->recognize_started;
#ifdef SUPPORT_BARGEIN_MODE
        disable_bargein(stdev);
#endif
        if (stdev->is_recording) {
            ALOGI("%s: Close BargeIn Record PCM ", __func__);
            if (stdev->recording_pcm) {
                pcm_close(stdev->recording_pcm);
                stdev->recording_pcm = NULL;
                stdev->is_recording = false;
            }
        }

        if (stdev->is_streaming) {
            ALOGI("%s: Close BargeIn Streaming PCM", __func__);
            if (stdev->streaming_pcm) {
                pcm_close(stdev->streaming_pcm);
                stdev->streaming_pcm = NULL;
            }

            stdev->is_streaming = false;
            stdev->is_seamless_recording = false;
        }
        if (!stdev->audio_input_state) {
            active_bitmask = stdev_active_callback_bitmask(stdev);
            //stdev_vts_set_power(stdev, 0);
            if (active_bitmask) {
                stdev_vts_set_power(stdev, active_bitmask);
            }
        }
        ret = 0;
        stdev->recognize_started = tmp_recognize_started;
    }

exit:
    ALOGD("%s, exit", __func__);

    return 0;
}

__attribute__ ((visibility ("default")))
int sound_trigger_update_ahal_status(bool state)
{
    struct sound_trigger_device *stdev = &g_stdev;
    int ret = 0;

    ALOGD("%s, state %d", __func__, state);
    pthread_mutex_lock(&stdev->lock);
    if (!stdev->sthal_opened) {
        ALOGE("%s: stdev has not been opened", __func__);
        ret = -EFAULT;
        goto exit;
    }

    set_bargein(state);

exit:
    pthread_mutex_unlock(&stdev->lock);
    ALOGD("%s, exit", __func__);

    return ret;
}

__attribute__ ((visibility ("default")))
int sound_trigger_update_ahal_input_status(bool state) {
    struct sound_trigger_device *stdev = &g_stdev;
    int active_bitmask = 0;
    int ret = 0;

    pthread_mutex_lock(&stdev->lock);
    ALOGD("%s, state %d", __func__, state);
    if (!stdev->sthal_opened) {
        ALOGE("%s: stdev has not been opened", __func__);
        ret = -EFAULT;
        goto exit;
    }

    if (stdev->audio_input_state == state) {
        ALOGE("%s: same as previous state", __func__);
        ret = 0;
        goto exit;
    }

    stdev->audio_input_state = state;

    if (stdev->voicecall_state) {
        ALOGE("%s: voicecall_state is true, can't start Recongition", __func__);
        goto exit;
    }

    if (!stdev->recognize_started) {
        ALOGD("%s: wakeup off, no need to voice trigger state", __func__);
        goto exit;
    }

    //stop voice recognition when normal recording started
    if (stdev->audio_input_state) {
        int tmp_recognize_started = stdev->recognize_started;

        //should stop streaming read thread here

        if (stdev->is_recording) {
            ALOGI("%s: Close VTS Record PCM", __func__);
            if (stdev->recording_pcm) {
                pthread_mutex_lock(&stdev->recording_lock);
                pcm_close(stdev->recording_pcm);
                stdev->recording_pcm = NULL;
                pthread_mutex_unlock(&stdev->recording_lock);
            }
            stdev->is_recording = false;
        }

        pthread_mutex_lock(&stdev->streaming_pcm_lock);
        if (stdev->is_streaming) {
            ALOGI("%s: Close VTS Seamless PCM", __func__);
            if (stdev->streaming_pcm) {
                pthread_mutex_lock(&stdev->trigger_lock);
                pcm_close(stdev->streaming_pcm);
                stdev->streaming_pcm = NULL;
                pthread_mutex_unlock(&stdev->trigger_lock);
            }

            stdev->is_seamless_recording = false;
            ALOGD("%s stdev->is_streaming %d", __func__, stdev->is_streaming);
            stdev->is_streaming = false;
        }
        pthread_mutex_unlock(&stdev->streaming_pcm_lock);
#ifdef SUPPORT_BARGEIN_MODE
        if (stdev->is_bargein_mode_configed) {
            disable_bargein(stdev);
        } else
#endif
        {
            if (stdev->is_mic_configured)
                stdev_vts_set_power(stdev, 0);
        }

        stdev->recognize_started = tmp_recognize_started;
    } else {
        active_bitmask = stdev_active_callback_bitmask(stdev);
        if (active_bitmask) {
#ifdef SUPPORT_BARGEIN_MODE
            ALOGD("%s, requires_bargein_mode:%d,is_bargein_mode_configed:%d",
                __func__, stdev->requires_bargein_mode, stdev->is_bargein_mode_configed);

            if (stdev->requires_bargein_mode) {
                if (!stdev->is_bargein_mode_configed)
                    enable_bargein(stdev,active_bitmask);
            } else
#endif
            {
                stdev_vts_set_power(stdev, active_bitmask);
            }
        }
    }

exit:
    pthread_mutex_unlock(&stdev->lock);
    ALOGD("%s, exit", __func__);

    return ret;
}
#ifdef SUPPORT_BARGEIN_MODE
static void enable_bargein(struct sound_trigger_device *stdev, int enabled_algorithms) {
    if (!(enabled_algorithms & ((0x1 << HOTWORD_INDEX) | (0x1 << ODMVOICE_INDEX)))) {
        ALOGE("%s: Abnormal status, don't enable bargein", __func__);
        return;
    }

    if (enabled_algorithms & (0x1 << HOTWORD_INDEX)) {
        stdev->recognize_started |= (0x1 << HOTWORD_INDEX);
        ALOGI("%s: Google Model recognization started", __func__);
    }

    if (enabled_algorithms & (0x1 << ODMVOICE_INDEX)) {
        stdev->recognize_started |= (0x1 << ODMVOICE_INDEX);
        ALOGI("%s: ODMVoice Model recognition mode started", __func__);
    }

#ifdef SUPPORT_AUDIO_MONITOR
    if (stdev->sound_card_state)
        return;
#endif
    if (stdev->notify_set_bargein_route)
        stdev->notify_set_bargein_route(true);

    stdev->is_bargein_mode_configed = true;
}

static void disable_bargein(struct sound_trigger_device *stdev) {
    /* Stop recognition of previous started models */
    if (stdev->recognize_started & (0x1 << HOTWORD_INDEX)) {
        stdev->recognize_started &= ~(0x1 << HOTWORD_INDEX);
        ALOGD("%s: Google Model recognization stopped", __func__);
    }

    if (stdev->recognize_started & (0x1 << ODMVOICE_INDEX)) {
        ALOGD("%s: ODMVoice Model recognition mode stopped", __func__);
        stdev->recognize_started &= ~(0x1 << ODMVOICE_INDEX);
    }

#ifdef SUPPORT_AUDIO_MONITOR
    if (stdev->sound_card_state)
        return;
#endif
    if (stdev->notify_set_bargein_route)
        stdev->notify_set_bargein_route(false);

    stdev->is_bargein_mode_configed = false;
}

__attribute__ ((visibility ("default")))
int sound_trigger_update_sound_card_status(int state)
{
    struct sound_trigger_device *stdev = &g_stdev;
    int ret = 0;
    ALOGD("%s: update sound card status %d, stdev->is_bargein_mode_configed:%d, stdev->requires_bargein_mode:%d", __func__, state, stdev->is_bargein_mode_configed,stdev->requires_bargein_mode);

    pthread_mutex_lock(&stdev->lock);
    if (!stdev->sthal_opened) {
        ALOGE("%s: stdev has not been opened", __func__);
        ret = 0;
        goto exit;
    }

    if (stdev->sound_card_state == state) {
        ALOGE("%s: same as previous state, stdev->sound_card_state:%d", __func__,stdev->sound_card_state);
        ret = 0;
        goto exit;
    }

    stdev->sound_card_state = state;

    if (stdev->recognize_started && !stdev->voicecall_state
        && (state == NORMAL_STATUS) && stdev->requires_bargein_mode) {
        int active_bitmask = stdev_active_callback_bitmask(stdev);
        enable_bargein(stdev, active_bitmask);
    }

exit:
    pthread_mutex_unlock(&stdev->lock);
    ALOGD("%s, exit", __func__);

    return ret;
}
#endif

static int stdev_close(hw_device_t *device)
{
    struct sound_trigger_device *stdev = (struct sound_trigger_device *)device;
    int ret = 0, i;

    ALOGV("%s", __func__);

    pthread_mutex_lock(&stdev->lock);
    if (!stdev->sthal_opened) {
        ALOGE("%s: device already closed", __func__);
        ret = -EFAULT;
        goto exit;
    }
    stdev_join_callback_thread(stdev, false);
    stdev_close_mixer(stdev);
#ifdef MMAP_INTERFACE_ENABLED
    if (munmap(stdev->mapped_addr,VTSDRV_MISC_MODEL_BIN_MAXSZ) < 0) {
        ALOGE("%s: munmap failed  %s", __func__, strerror(errno));
    }

    close(stdev->vtsdev_fd);
    stdev->vtsdev_fd = -1;
#endif
    memset(stdev->recognition_callbacks, 0, sizeof(stdev->recognition_callbacks));
    memset(stdev->sound_model_callbacks, 0, sizeof(stdev->sound_model_callbacks));
    for (i = 0; i < MAX_SOUND_MODELS; ++i) {
        if (stdev->configs[i]) {
            free(stdev->configs[i]);
            stdev->configs[i] = NULL;
        }
    }

    stdev->sthal_opened = false;

exit:
    pthread_mutex_unlock(&stdev->lock);
    return ret;
}

static int stdev_open(
        const hw_module_t *module,
        const char *name,
        hw_device_t **device)
{
    struct sound_trigger_device *stdev;
    int ret = -EINVAL;
    int forcereset = 1;

    ALOGV("%s", __func__);

    if (strcmp(name, SOUND_TRIGGER_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    stdev = &g_stdev;
    pthread_mutex_lock(&stdev->lock);

    if (stdev->sthal_opened) {
        ALOGE("%s: Only one sountrigger can be opened at a time", __func__);
        ret = -EBUSY;
        goto exit;
    }

#ifdef MMAP_INTERFACE_ENABLED
        /* Open VTS Misc device for loading Model binary through MMAP interface */
        stdev->vtsdev_fd = open("/dev/vts_fio_dev", O_RDWR);
        if (stdev->vtsdev_fd < 0) {
            ALOGE("%s: Failed to open VTS-Misc device %s", __func__, strerror(errno));
            goto exit;
        }

        /* memory map VTS misc driver */
        stdev->mapped_addr = mmap(NULL, VTSDRV_MISC_MODEL_BIN_MAXSZ,
                            PROT_READ | PROT_WRITE, MAP_SHARED, stdev->vtsdev_fd, 0);
        if (stdev->mapped_addr == MAP_FAILED) {
            ALOGE("%s: VTS Device MMAP failed", __func__);
            close(stdev->vtsdev_fd);
            goto exit;
        }
        ALOGI("%s: VTS device opened Successfully for MMAP Interface", __func__);
#endif

    ret = stdev_init_mixer(stdev);
    if (ret) {
        ALOGE("Error mixer init");
        goto exit;
    }

    stdev->device.common.tag = HARDWARE_DEVICE_TAG;
    stdev->device.common.version = SOUND_TRIGGER_DEVICE_API_VERSION_1_0;
    stdev->device.common.module = (struct hw_module_t *)module;
    stdev->device.common.close = stdev_close;
#ifdef STHAL_2_3
    stdev->device.common.version = SOUND_TRIGGER_DEVICE_API_VERSION_1_3;
    stdev->device.get_properties_extended = stdev_get_properties_extended;
    stdev->device.start_recognition_extended = stdev_start_recognition_extended;
    stdev->device.set_parameter = stdev_set_parameter;
    stdev->device.get_parameter = stdev_get_parameter;
    stdev->device.query_parameter = stdev_query_parameter;
#endif
    stdev->device.get_properties = stdev_get_properties;
    stdev->device.load_sound_model = stdev_load_sound_model;
    stdev->device.unload_sound_model = stdev_unload_sound_model;
    stdev->device.start_recognition = stdev_start_recognition;
    stdev->device.stop_recognition = stdev_stop_recognition;
    stdev->send_socket = stdev->term_socket = stdev->uevent_socket = -1;
    stdev->streaming_pcm = NULL;
    stdev->is_seamless_recording = false;
    stdev->sthal_opened = true;
    stdev->recognize_started = 0;
    stdev->is_mic_configured = 0;
    stdev->active_mic = VTS_MAIN_MIC;
    stdev->is_recording = false;
    stdev->recording_pcm = NULL;
    stdev->voicecall_state = VOICECALL_STOPPED;
    stdev->recog_cbstate = RECOG_CB_NONE;
    stdev->audio_input_state = false;
#ifdef SUPPORT_BARGEIN_MODE
    stdev->is_bargein_mode_configed = false;
    stdev->requires_bargein_mode = false;
    stdev->sound_card_state = NORMAL_STATUS;
#endif

    stdev->kw_length = KW_LENGTH;
    stdev->kw_type = 0;

    int i;
    for (i = 0; i < MAX_SOUND_MODELS; ++i) {
      stdev->model_handles[i] = -1;
      stdev->model_execstate[i] = MODEL_STATE_NONE;
      stdev->model_stopfailedhandles[i] = HANDLE_NONE;
    }

    *device = &stdev->device.common; // same address as stdev

    if (access(AUDIO_PRIMARY_HAL_LIBRARY_PATH, R_OK) == 0) {
        stdev->audio_primary_lib = dlopen(AUDIO_PRIMARY_HAL_LIBRARY_PATH, RTLD_NOW);
        if (stdev->audio_primary_lib == NULL) {
            ALOGE("%s: DLOPEN failed for %s", __func__, AUDIO_PRIMARY_HAL_LIBRARY_PATH);
            goto hal_exit;
        } else {
#ifdef SUPPORT_BARGEIN_MODE
            ALOGV("%s: DLOPEN successful for %s", __func__, AUDIO_PRIMARY_HAL_LIBRARY_PATH);
            stdev->notify_set_bargein_route =
                        (int (*)(bool))dlsym(stdev->audio_primary_lib,
                                                        "notify_set_bargein_route");
            if (!stdev->notify_set_bargein_route) {
                ALOGE("%s(%d) : Error in grabbing function from %s, error(%s)", __func__, __LINE__,
                    AUDIO_PRIMARY_HAL_LIBRARY_PATH, dlerror());
                stdev->notify_set_bargein_route = 0;
//                goto notify_exit;
            }
#endif
            stdev->notify_set_vts_route =
                        (int (*)(bool, int))dlsym(stdev->audio_primary_lib,
                                                        "notify_set_vts_route");
            if (!stdev->notify_set_vts_route) {
                ALOGE("%s(%d) : Error in grabbing function from %s, error(%s)", __func__, __LINE__,
                    AUDIO_PRIMARY_HAL_LIBRARY_PATH, dlerror());
                stdev->notify_set_vts_route = 0;
  //              goto notify_exit;
            }
        }
    } else {
        ALOGE("%s: access failed for %s", __func__, AUDIO_PRIMARY_HAL_LIBRARY_PATH);
    }

    if (set_mixer_ctrls(stdev, vts_forcereset_ctlname, &forcereset, 1, false)) {
        ALOGE("%s: VTS Force Reset configuration Failed", __func__);
        goto exit;
    }

    pthread_mutex_unlock(&stdev->lock);
    return 0;

//notify_exit:
    if(stdev->audio_primary_lib)
        dlclose(stdev->audio_primary_lib);
hal_exit:
    stdev_close_mixer(stdev);
exit:
    pthread_mutex_unlock(&stdev->lock);

    ALOGI("%s: failed to open SoundTrigger HW Device", __func__);
    return ret;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = stdev_open,
};

struct sound_trigger_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = SOUND_TRIGGER_MODULE_API_VERSION_1_0,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = SOUND_TRIGGER_HARDWARE_MODULE_ID,
        .name = "Exynos Primary SoundTrigger HAL",
        .author = "Samsung SLSI",
        .methods = &hal_module_methods,
    },
};
