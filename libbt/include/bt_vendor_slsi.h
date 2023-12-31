/****************************************************************************
 *
 * Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd
 *
 ****************************************************************************/
#ifndef BT_VENDOR_SLSI_H__
#define BT_VENDOR_SLSI_H__

#include <log/log.h>
#ifndef FALSE
#define FALSE false
#endif

#ifndef TRUE
#define TRUE true
#endif

#ifndef BTVENDOR_DBG
#define BTVENDOR_DBG FALSE
#endif

#define BTVENDORE(param, ...) { ALOGE(param, ## __VA_ARGS__); }

#if (BTVENDOR_DBG == TRUE)
#define BTVENDORD(param, ...) { ALOGD(param, ## __VA_ARGS__); }
#define BTVENDORI(param, ...) { ALOGI(param, ## __VA_ARGS__); }
#define BTVENDORV(param, ...) { ALOGV(param, ## __VA_ARGS__); }
#define BTVENDORW(param, ...) { ALOGW(param, ## __VA_ARGS__); }
#else
#define BTVENDORD(param, ...) {}
#define BTVENDORI(param, ...) {}
#define BTVENDORV(param, ...) {}
#define BTVENDORW(param, ...) {}
#endif
#define SCSC_UNUSED(x) (void)(x)

#endif
