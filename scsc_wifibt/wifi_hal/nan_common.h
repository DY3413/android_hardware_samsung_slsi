#ifndef NAN_COMMON_H_
#define NAN_COMMON_H_
#define SLSI_WIFI_HAL_NAN_VERSION 1

#define CHECK_WIFI_STATUS_RETURN_FAIL(result, LOGSTR) \
    if (result != WIFI_SUCCESS) {\
        ALOGE(LOGSTR" [result:%d]", result);\
        return result;\
    }

#define CHECK_CONFIG_PUT_8_RETURN_FAIL(config, val, nan_attribute, request, result, FAIL_STR) \
    if (config) {\
        result = request.put_u8(nan_attribute, val); \
        if (result != WIFI_SUCCESS) {\
            ALOGE(FAIL_STR" [result:%d]", result);\
            return result;\
        }\
    }

#define CHECK_CONFIG_PUT_16_RETURN_FAIL(config, val, nan_attribute, request, result, FAIL_STR) \
    if (config) {\
        result = request.put_u16(nan_attribute, val); \
        if (result != WIFI_SUCCESS) {\
            ALOGE(FAIL_STR" [result:%d]", result);\
            return result;\
        }\
    }


#define CHECK_CONFIG_PUT_32_RETURN_FAIL(config, val, nan_attribute, request, result, FAIL_STR) \
    if (config) {\
        result = request.put_u32(nan_attribute, val); \
        if (result != WIFI_SUCCESS) {\
            ALOGE(FAIL_STR" [result:%d]", result);\
            return result;\
        }\
    }

#define CHECK_CONFIG_PUT_RETURN_FAIL(config, valptr, len, nan_attribute, request, result, FAIL_STR) \
    if (config) {\
        result = request.put(nan_attribute, valptr, len); \
        if (result != WIFI_SUCCESS) {\
            ALOGE(FAIL_STR" [result:%d]", result);\
            return result;\
        }\
    }

typedef enum {
    NAN_REQ_ATTR_MASTER_PREF = 1,
    NAN_REQ_ATTR_CLUSTER_LOW,
    NAN_REQ_ATTR_CLUSTER_HIGH,
    NAN_REQ_ATTR_HOP_COUNT_LIMIT_VAL,
    NAN_REQ_ATTR_SID_BEACON_VAL,

    NAN_REQ_ATTR_SUPPORT_2G4_VAL,
    NAN_REQ_ATTR_SUPPORT_5G_VAL,

    NAN_REQ_ATTR_RSSI_CLOSE_2G4_VAL,
    NAN_REQ_ATTR_RSSI_MIDDLE_2G4_VAL,
    NAN_REQ_ATTR_RSSI_PROXIMITY_2G4_VAL,
    NAN_REQ_ATTR_BEACONS_2G4_VAL = 11,
    NAN_REQ_ATTR_SDF_2G4_VAL,
    NAN_REQ_ATTR_CHANNEL_2G4_MHZ_VAL,
    NAN_REQ_ATTR_RSSI_PROXIMITY_VAL,


    NAN_REQ_ATTR_RSSI_CLOSE_5G_VAL,
    NAN_REQ_ATTR_RSSI_CLOSE_PROXIMITY_5G_VAL,
    NAN_REQ_ATTR_RSSI_MIDDLE_5G_VAL,
    NAN_REQ_ATTR_RSSI_PROXIMITY_5G_VAL,
    NAN_REQ_ATTR_BEACON_5G_VAL,
    NAN_REQ_ATTR_SDF_5G_VAL,
    NAN_REQ_ATTR_CHANNEL_5G_MHZ_VAL = 21,

    NAN_REQ_ATTR_RSSI_WINDOW_SIZE_VAL,
    NAN_REQ_ATTR_OUI_VAL,
    NAN_REQ_ATTR_MAC_ADDR_VAL,
    NAN_REQ_ATTR_CLUSTER_VAL,
    NAN_REQ_ATTR_SOCIAL_CH_SCAN_DWELL_TIME,
    NAN_REQ_ATTR_SOCIAL_CH_SCAN_PERIOD,
    NAN_REQ_ATTR_RANDOM_FACTOR_FORCE_VAL,
    NAN_REQ_ATTR_HOP_COUNT_FORCE_VAL,
    NAN_REQ_ATTR_CONN_CAPABILITY_PAYLOAD_TX,
    NAN_REQ_ATTR_CONN_CAPABILITY_IBSS = 31,
    NAN_REQ_ATTR_CONN_CAPABILITY_WFD,
    NAN_REQ_ATTR_CONN_CAPABILITY_WFDS,
    NAN_REQ_ATTR_CONN_CAPABILITY_TDLS,
    NAN_REQ_ATTR_CONN_CAPABILITY_MESH,
    NAN_REQ_ATTR_CONN_CAPABILITY_WLAN_INFRA,
    NAN_REQ_ATTR_DISCOVERY_ATTR_NUM_ENTRIES,
    NAN_REQ_ATTR_DISCOVERY_ATTR_VAL,
    NAN_REQ_ATTR_CONN_TYPE,
    NAN_REQ_ATTR_NAN_ROLE,
    NAN_REQ_ATTR_TRANSMIT_FREQ = 41,
    NAN_REQ_ATTR_AVAILABILITY_DURATION,
    NAN_REQ_ATTR_AVAILABILITY_INTERVAL,
    NAN_REQ_ATTR_MESH_ID_LEN,
    NAN_REQ_ATTR_MESH_ID,
    NAN_REQ_ATTR_INFRASTRUCTURE_SSID_LEN,
    NAN_REQ_ATTR_INFRASTRUCTURE_SSID,
    NAN_REQ_ATTR_FURTHER_AVAIL_NUM_ENTRIES,
    NAN_REQ_ATTR_FURTHER_AVAIL_VAL,
    NAN_REQ_ATTR_FURTHER_AVAIL_ENTRY_CTRL,
    NAN_REQ_ATTR_FURTHER_AVAIL_CHAN_CLASS = 51,
    NAN_REQ_ATTR_FURTHER_AVAIL_CHAN,
    NAN_REQ_ATTR_FURTHER_AVAIL_CHAN_MAPID,
    NAN_REQ_ATTR_FURTHER_AVAIL_INTERVAL_BITMAP,
    NAN_REQ_ATTR_PUBLISH_ID,
    NAN_REQ_ATTR_PUBLISH_TTL,
    NAN_REQ_ATTR_PUBLISH_PERIOD,
    NAN_REQ_ATTR_PUBLISH_TYPE,
    NAN_REQ_ATTR_PUBLISH_TX_TYPE,
    NAN_REQ_ATTR_PUBLISH_COUNT,
    NAN_REQ_ATTR_PUBLISH_SERVICE_NAME_LEN = 61,
    NAN_REQ_ATTR_PUBLISH_SERVICE_NAME,
    NAN_REQ_ATTR_PUBLISH_MATCH_ALGO,
    NAN_REQ_ATTR_PUBLISH_SERVICE_INFO_LEN,
    NAN_REQ_ATTR_PUBLISH_SERVICE_INFO,
    NAN_REQ_ATTR_PUBLISH_RX_MATCH_FILTER_LEN,
    NAN_REQ_ATTR_PUBLISH_RX_MATCH_FILTER,
    NAN_REQ_ATTR_PUBLISH_TX_MATCH_FILTER_LEN,
    NAN_REQ_ATTR_PUBLISH_TX_MATCH_FILTER,
    NAN_REQ_ATTR_PUBLISH_RSSI_THRESHOLD_FLAG,
    NAN_REQ_ATTR_PUBLISH_CONN_MAP = 71,
    NAN_REQ_ATTR_PUBLISH_RECV_IND_CFG,
    NAN_REQ_ATTR_SUBSCRIBE_ID,
    NAN_REQ_ATTR_SUBSCRIBE_TTL,
    NAN_REQ_ATTR_SUBSCRIBE_PERIOD,
    NAN_REQ_ATTR_SUBSCRIBE_TYPE,
    NAN_REQ_ATTR_SUBSCRIBE_RESP_FILTER_TYPE,
    NAN_REQ_ATTR_SUBSCRIBE_RESP_INCLUDE,
    NAN_REQ_ATTR_SUBSCRIBE_USE_RESP_FILTER,
    NAN_REQ_ATTR_SUBSCRIBE_SSI_REQUIRED,
    NAN_REQ_ATTR_SUBSCRIBE_MATCH_INDICATOR  = 81,
    NAN_REQ_ATTR_SUBSCRIBE_COUNT,
    NAN_REQ_ATTR_SUBSCRIBE_SERVICE_NAME_LEN,
    NAN_REQ_ATTR_SUBSCRIBE_SERVICE_NAME,
    NAN_REQ_ATTR_SUBSCRIBE_SERVICE_INFO_LEN,
    NAN_REQ_ATTR_SUBSCRIBE_SERVICE_INFO,
    NAN_REQ_ATTR_SUBSCRIBE_RX_MATCH_FILTER_LEN,
    NAN_REQ_ATTR_SUBSCRIBE_RX_MATCH_FILTER,
    NAN_REQ_ATTR_SUBSCRIBE_TX_MATCH_FILTER_LEN,
    NAN_REQ_ATTR_SUBSCRIBE_TX_MATCH_FILTER,
    NAN_REQ_ATTR_SUBSCRIBE_RSSI_THRESHOLD_FLAG = 91,
    NAN_REQ_ATTR_SUBSCRIBE_CONN_MAP,
    NAN_REQ_ATTR_SUBSCRIBE_NUM_INTF_ADDR_PRESENT,
    NAN_REQ_ATTR_SUBSCRIBE_INTF_ADDR,
    NAN_REQ_ATTR_SUBSCRIBE_RECV_IND_CFG,
    NAN_REQ_ATTR_FOLLOWUP_ID,
    NAN_REQ_ATTR_FOLLOWUP_REQUESTOR_ID,
    NAN_REQ_ATTR_FOLLOWUP_ADDR,
    NAN_REQ_ATTR_FOLLOWUP_PRIORITY,
    NAN_REQ_ATTR_FOLLOWUP_SERVICE_NAME_LEN,
    NAN_REQ_ATTR_FOLLOWUP_SERVICE_NAME = 101,
    NAN_REQ_ATTR_FOLLOWUP_TX_WINDOW,
    NAN_REQ_ATTR_FOLLOWUP_RECV_IND_CFG,
    NAN_REQ_ATTR_SUBSCRIBE_SID_BEACON_VAL,
    NAN_REQ_ATTR_DW_2G4_INTERVAL,
    NAN_REQ_ATTR_DW_5G_INTERVAL,
    NAN_REQ_ATTR_DISC_MAC_ADDR_RANDOM_INTERVAL,
    NAN_REQ_ATTR_PUBLISH_SDEA_LEN,
    NAN_REQ_ATTR_PUBLISH_SDEA,

    NAN_REQ_ATTR_RANGING_AUTO_RESPONSE,
    NAN_REQ_ATTR_SDEA_PARAM_NDP_TYPE = 111,
    NAN_REQ_ATTR_SDEA_PARAM_SECURITY_CFG,
    NAN_REQ_ATTR_SDEA_PARAM_RANGING_STATE,
    NAN_REQ_ATTR_SDEA_PARAM_RANGE_REPORT,
    NAN_REQ_ATTR_SDEA_PARAM_QOS_CFG,
    NAN_REQ_ATTR_RANGING_CFG_INTERVAL,
    NAN_REQ_ATTR_RANGING_CFG_INDICATION,
    NAN_REQ_ATTR_RANGING_CFG_INGRESS_MM,
    NAN_REQ_ATTR_RANGING_CFG_EGRESS_MM,
    NAN_REQ_ATTR_CIPHER_TYPE,
    NAN_REQ_ATTR_SCID_LEN = 121,
    NAN_REQ_ATTR_SCID,
    NAN_REQ_ATTR_SECURITY_KEY_TYPE,
    NAN_REQ_ATTR_SECURITY_PMK_LEN,
    NAN_REQ_ATTR_SECURITY_PMK,
    NAN_REQ_ATTR_SECURITY_PASSPHRASE_LEN,
    NAN_REQ_ATTR_SECURITY_PASSPHRASE,
    NAN_REQ_ATTR_RANGE_RESPONSE_CFG_PUBLISH_ID,
    NAN_REQ_ATTR_RANGE_RESPONSE_CFG_REQUESTOR_ID,
    NAN_REQ_ATTR_RANGE_RESPONSE_CFG_PEER_ADDR,
    NAN_REQ_ATTR_RANGE_RESPONSE_CFG_RANGING_RESPONSE = 131,
    NAN_REQ_ATTR_REQ_INSTANCE_ID,
    NAN_REQ_ATTR_NDP_INSTANCE_ID,
    NAN_REQ_ATTR_CHAN_REQ_TYPE,
    NAN_REQ_ATTR_CHAN,
    NAN_REQ_ATTR_DATA_INTERFACE_NAME_LEN,
    NAN_REQ_ATTR_DATA_INTERFACE_NAME,
    NAN_REQ_ATTR_APP_INFO_LEN,
    NAN_REQ_ATTR_APP_INFO,
    NAN_REQ_ATTR_SERVICE_NAME_LEN,
    NAN_REQ_ATTR_SERVICE_NAME = 141,
    NAN_REQ_ATTR_NDP_RESPONSE_CODE,
    NAN_REQ_ATTR_USE_NDPE_ATTR,
    NAN_REQ_ATTR_HAL_TRANSACTION_ID,
    NAN_REQ_ATTR_CONFIG_DISC_MAC_ADDR_RANDOM,
    NAN_REQ_ATTR_DISCOVERY_BEACON_INT,
    NAN_REQ_ATTR_NSS,
    NAN_REQ_ATTR_ENABLE_RANGING,
    NAN_REQ_ATTR_DW_EARLY_TERMINATION
} NAN_REQ_ATTRIBUTES;

typedef enum {
    NAN_REPLY_ATTR_STATUS_TYPE,
    NAN_REPLY_ATTR_VALUE,
    NAN_REPLY_ATTR_RESPONSE_TYPE,
    NAN_REPLY_ATTR_PUBLISH_SUBSCRIBE_TYPE,
    NAN_REPLY_ATTR_CAP_MAX_CONCURRENT_CLUSTER,
    NAN_REPLY_ATTR_CAP_MAX_PUBLISHES,
    NAN_REPLY_ATTR_CAP_MAX_SUBSCRIBES,
    NAN_REPLY_ATTR_CAP_MAX_SERVICE_NAME_LEN,
    NAN_REPLY_ATTR_CAP_MAX_MATCH_FILTER_LEN,
    NAN_REPLY_ATTR_CAP_MAX_TOTAL_MATCH_FILTER_LEN,
    NAN_REPLY_ATTR_CAP_MAX_SERVICE_SPECIFIC_INFO_LEN,
    NAN_REPLY_ATTR_CAP_MAX_VSA_DATA_LEN,
    NAN_REPLY_ATTR_CAP_MAX_MESH_DATA_LEN,
    NAN_REPLY_ATTR_CAP_MAX_NDI_INTERFACES,
    NAN_REPLY_ATTR_CAP_MAX_NDP_SESSIONS,
    NAN_REPLY_ATTR_CAP_MAX_APP_INFO_LEN,
    NAN_REPLY_ATTR_NDP_INSTANCE_ID,
    NAN_REPLY_ATTR_CAP_MAX_QUEUED_TRANSMIT_FOLLOWUP_MGS,
    NAN_REPLY_ATTR_CAP_MAX_NDP_SUPPORTED_BANDS,
    NAN_REPLY_ATTR_CAP_MAX_CIPHER_SUITES_SUPPORTED,
    NAN_REPLY_ATTR_CAP_MAX_SCID_LEN,
    NAN_REPLY_ATTR_CAP_NDP_SECURITY_SUPPORTED,
    NAN_REPLY_ATTR_CAP_MAX_SDEA_SERVICE_SPECIFIC_INFO_LEN,
    NAN_REPLY_ATTR_CAP_MAX_SUBSCRIBE_ADDRESS,
    NAN_REPLY_ATTR_CAP_NDPE_ATTR_SUPPORTED,
    NAN_REPLY_ATTR_HAL_TRANSACTION_ID
} NAN_RESP_ATTRIBUTES;

typedef enum {
    NAN_EVT_ATTR_MATCH_PUBLISH_SUBSCRIBE_ID = 0,
    NAN_EVT_ATTR_MATCH_REQUESTOR_INSTANCE_ID,
    NAN_EVT_ATTR_MATCH_ADDR,
    NAN_EVT_ATTR_MATCH_SERVICE_SPECIFIC_INFO_LEN,
    NAN_EVT_ATTR_MATCH_SERVICE_SPECIFIC_INFO,
    NAN_EVT_ATTR_MATCH_SDF_MATCH_FILTER_LEN,
    NAN_EVT_ATTR_MATCH_SDF_MATCH_FILTER,
    NAN_EVT_ATTR_MATCH_MATCH_OCCURED_FLAG,
    NAN_EVT_ATTR_MATCH_OUT_OF_RESOURCE_FLAG,
    NAN_EVT_ATTR_MATCH_RSSI_VALUE,
/*CONN_CAPABILITY*/
    NAN_EVT_ATTR_MATCH_CONN_CAPABILITY_IS_WFD_SUPPORTED = 10,
    NAN_EVT_ATTR_MATCH_CONN_CAPABILITY_IS_WFDS_SUPPORTED,
    NAN_EVT_ATTR_MATCH_CONN_CAPABILITY_IS_TDLS_SUPPORTED,
    NAN_EVT_ATTR_MATCH_CONN_CAPABILITY_IS_IBSS_SUPPORTED,
    NAN_EVT_ATTR_MATCH_CONN_CAPABILITY_IS_MESH_SUPPORTED,
    NAN_EVT_ATTR_MATCH_CONN_CAPABILITY_WLAN_INFRA_FIELD,
    NAN_EVT_ATTR_MATCH_NUM_RX_DISCOVERY_ATTR,
    NAN_EVT_ATTR_MATCH_RX_DISCOVERY_ATTR,
/*NANRECEIVEPOSTDISCOVERY DISCOVERY_ATTR,*/
    NAN_EVT_ATTR_MATCH_DISC_ATTR_TYPE,
    NAN_EVT_ATTR_MATCH_DISC_ATTR_ROLE,
    NAN_EVT_ATTR_MATCH_DISC_ATTR_DURATION = 20,
    NAN_EVT_ATTR_MATCH_DISC_ATTR_AVAIL_INTERVAL_BITMAP,
    NAN_EVT_ATTR_MATCH_DISC_ATTR_MAPID,
    NAN_EVT_ATTR_MATCH_DISC_ATTR_ADDR,
    NAN_EVT_ATTR_MATCH_DISC_ATTR_MESH_ID_LEN,
    NAN_EVT_ATTR_MATCH_DISC_ATTR_MESH_ID,
    NAN_EVT_ATTR_MATCH_DISC_ATTR_INFRASTRUCTURE_SSID_LEN,
    NAN_EVT_ATTR_MATCH_DISC_ATTR_INFRASTRUCTURE_SSID_VAL,

    NAN_EVT_ATTR_MATCH_NUM_CHANS,
    NAN_EVT_ATTR_MATCH_FAMCHAN,
/*FAMCHAN[32],*/
    NAN_EVT_ATTR_MATCH_FAM_ENTRY_CONTROL = 30,
    NAN_EVT_ATTR_MATCH_FAM_CLASS_VAL,
    NAN_EVT_ATTR_MATCH_FAM_CHANNEL,
    NAN_EVT_ATTR_MATCH_FAM_MAPID,
    NAN_EVT_ATTR_MATCH_FAM_AVAIL_INTERVAL_BITMAP,
    NAN_EVT_ATTR_MATCH_CLUSTER_ATTRIBUTE_LEN,
    NAN_EVT_ATTR_MATCH_CLUSTER_ATTRIBUTE,
    NAN_EVT_ATTR_PUBLISH_ID,
    NAN_EVT_ATTR_PUBLISH_REASON,
    NAN_EVT_ATTR_SUBSCRIBE_ID,
    NAN_EVT_ATTR_SUBSCRIBE_REASON = 40,
    NAN_EVT_ATTR_DISABLED_REASON,
    NAN_EVT_ATTR_FOLLOWUP_PUBLISH_SUBSCRIBE_ID,
    NAN_EVT_ATTR_FOLLOWUP_REQUESTOR_INSTANCE_ID,
    NAN_EVT_ATTR_FOLLOWUP_ADDR,
    NAN_EVT_ATTR_FOLLOWUP_DW_OR_FAW,
    NAN_EVT_ATTR_FOLLOWUP_SERVICE_SPECIFIC_INFO_LEN,
    NAN_EVT_ATTR_FOLLOWUP_SERVICE_SPECIFIC_INFO,
    NAN_EVT_ATTR_DISCOVERY_ENGINE_EVT_TYPE    ,
    NAN_EVT_ATTR_DISCOVERY_ENGINE_MAC_ADDR,
    NAN_EVT_ATTR_DISCOVERY_ENGINE_CLUSTER = 50,
    NAN_EVT_ATTR_SDEA,
    NAN_EVT_ATTR_SDEA_LEN,
    NAN_EVT_ATTR_SCID,
    NAN_EVT_ATTR_SCID_LEN,
    NAN_EVT_ATTR_SDEA_PARAM_CONFIG_NAN_DATA_PATH,
    NAN_EVT_ATTR_SDEA_PARAM_NDP_TYPE,
    NAN_EVT_ATTR_SDEA_PARAM_SECURITY_CONFIG,
    NAN_EVT_ATTR_SDEA_PARAM_RANGE_STATE,
    NAN_EVT_ATTR_SDEA_PARAM_RANGE_REPORT,
    NAN_EVT_ATTR_SDEA_PARAM_QOS_CFG = 60,
    NAN_EVT_ATTR_RANGE_MEASUREMENT_MM,
    NAN_EVT_ATTR_RANGEING_EVENT_TYPE,
    NAN_EVT_ATTR_SECURITY_CIPHER_TYPE,
    NAN_EVT_ATTR_STATUS,
    NAN_EVT_ATTR_SERVICE_INSTANCE_ID,
    NAN_EVT_ATTR_NDP_INSTANCE_ID,
    NAN_EVT_ATTR_NDP_RSP_CODE,
    NAN_EVT_ATTR_STATUS_CODE,
    NAN_EVT_ATTR_CHANNEL_INFO,
    NAN_EVT_ATTR_APP_INFO_LEN = 70,
    NAN_EVT_ATTR_APP_INFO,
    NAN_EVT_ATTR_CHANNEL,
    NAN_EVT_ATTR_CHANNEL_BW,
    NAN_EVT_ATTR_CHANNEL_NSS,
    NAN_EVT_ATTR_HAL_TRANSACTION_ID
} NAN_EVT_ATTRIBUTES;

#endif