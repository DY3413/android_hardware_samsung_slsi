#include <stdint.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <linux/rtnetlink.h>
#include <netpacket/packet.h>
#include <linux/filter.h>
#include <linux/errqueue.h>

#include <linux/pkt_sched.h>
#include <netlink/object-api.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/types.h>

#include "nl80211_copy.h"
#include "sync.h"

#define LOG_TAG  "WifiHAL[offload]"

#include <log/log.h>

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"

typedef enum {
    MKEEP_ALIVE_ATTRIBUTE_ID = WIFI_HAL_ATTR_START,
    MKEEP_ALIVE_ATTRIBUTE_IP_PKT,
    MKEEP_ALIVE_ATTRIBUTE_IP_PKT_LEN,
    MKEEP_ALIVE_ATTRIBUTE_SRC_MAC_ADDR,
    MKEEP_ALIVE_ATTRIBUTE_DST_MAC_ADDR,
    MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC,
    MKEEP_ALIVE_ATTRIBUTE_MAX
} WIFI_MKEEP_ALIVE_ATTRIBUTE;

typedef enum {
    START_MKEEP_ALIVE,
    STOP_MKEEP_ALIVE,
} GetCmdType;

///////////////////////////////////////////////////////////////////////////////
class MKeepAliveCommand : public WifiCommand
{
    u8 mIndex;
    u8 *mIpPkt;
    u16 mIpPktLen;
    u8 *mSrcMacAddr;
    u8 *mDstMacAddr;
    u32 mPeriodMsec;
    GetCmdType mType;

public:

    // constructor for start sending
    MKeepAliveCommand(wifi_interface_handle iface, u8 index, u8 *ip_packet, u16 ip_packet_len,
            u8 *src_mac_addr, u8 *dst_mac_addr, u32 period_msec, GetCmdType cmdType)
        : WifiCommand(iface, 0), mIndex(index), mIpPkt(ip_packet),
        mIpPktLen(ip_packet_len), mSrcMacAddr(src_mac_addr), mDstMacAddr(dst_mac_addr),
        mPeriodMsec(period_msec), mType(cmdType)
    { }

    // constructor for stop sending
    MKeepAliveCommand(wifi_interface_handle iface, u8 index, GetCmdType cmdType)
        : WifiCommand(iface, 0), mIndex(index), mType(cmdType)
    {
        mIpPkt = NULL;
        mIpPktLen = 0;
        mSrcMacAddr = NULL;
        mDstMacAddr = NULL;
        mPeriodMsec = 0;
    }

    int createRequest(WifiRequest &request) {
        int result;

        switch (mType) {
            case START_MKEEP_ALIVE:
            {
                result = request.create(GOOGLE_OUI, SLSI_NL80211_VENDOR_SUBCMD_START_KEEP_ALIVE_OFFLOAD);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to create start keep alive request; result = %d", result);
                    return result;
                }

                nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);

                result = request.put_u8(MKEEP_ALIVE_ATTRIBUTE_ID, mIndex);
                if (result < 0) {
                    ALOGE("Failed to put id request; result = %d", result);
                    return result;
                }

                result = request.put_u16(MKEEP_ALIVE_ATTRIBUTE_IP_PKT_LEN, mIpPktLen);
                if (result < 0) {
                    ALOGE("Failed to put ip pkt len request; result = %d", result);
                    return result;
                }

                result = request.put(MKEEP_ALIVE_ATTRIBUTE_IP_PKT, (u8*)mIpPkt, mIpPktLen);
                if (result < 0) {
                    ALOGE("Failed to put ip pkt request; result = %d", result);
                    return result;
                }

                result = request.put_addr(MKEEP_ALIVE_ATTRIBUTE_SRC_MAC_ADDR, mSrcMacAddr);
                if (result < 0) {
                    ALOGE("Failed to put src mac address request; result = %d", result);
                    return result;
                }

                result = request.put_addr(MKEEP_ALIVE_ATTRIBUTE_DST_MAC_ADDR, mDstMacAddr);
                if (result < 0) {
                    ALOGE("Failed to put dst mac address request; result = %d", result);
                    return result;
                }

                result = request.put_u32(MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC, mPeriodMsec);
                if (result < 0) {
                    ALOGE("Failed to put period request; result = %d", result);
                    return result;
                }

                request.attr_end(data);
                break;
            }

            case STOP_MKEEP_ALIVE:
            {
                result = request.create(GOOGLE_OUI, SLSI_NL80211_VENDOR_SUBCMD_STOP_KEEP_ALIVE_OFFLOAD);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to create stop keep alive request; result = %d", result);
                    return result;
                }

                nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);

                result = request.put_u8(MKEEP_ALIVE_ATTRIBUTE_ID, mIndex);
                if (result < 0) {
                    ALOGE("Failed to put id request; result = %d", result);
                    return result;
                }

                request.attr_end(data);
                break;
            }

            default:
                ALOGE("Unknown wifi keep alive command");
                result = WIFI_ERROR_UNKNOWN;
        }
        return result;
    }

    int start() {
        ALOGD("Start mkeep_alive command");
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to create keep alive request; result = %d", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to register keep alive response; result = %d", result);
        }
        return result;
    }

    virtual int handleResponse(WifiEvent& reply) {

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        switch (mType) {
            case START_MKEEP_ALIVE:
            case STOP_MKEEP_ALIVE:
                break;

            default:
                ALOGW("Unknown mkeep_alive command");
        }
        return NL_OK;
    }

    virtual int handleEvent(WifiEvent& event) {
        /* NO events! */
        return NL_SKIP;
    }
};


/* API to send specified mkeep_alive packet periodically. */
wifi_error wifi_start_sending_offloaded_packet(wifi_request_id index, wifi_interface_handle iface,
        u16 ether_type, u8 *ip_packet, u16 ip_packet_len, u8 *src_mac_addr, u8 *dst_mac_addr,
        u32 period_msec)
{
    if ((index > 0 && index <= N_AVAIL_ID) && (ip_packet != NULL) && (src_mac_addr != NULL)
            && (dst_mac_addr != NULL) && (period_msec > 0)
            && (ip_packet_len <= MKEEP_ALIVE_IP_PKT_MAX)) {
        MKeepAliveCommand *cmd = new MKeepAliveCommand(iface, index, ip_packet, ip_packet_len,
                src_mac_addr, dst_mac_addr, period_msec, START_MKEEP_ALIVE);
        wifi_error result = (wifi_error)cmd->start();
        delete cmd;
        return result;
    } else {
        ALOGE("Invalid mkeep_alive parameters");
        return  WIFI_ERROR_INVALID_ARGS;
    }
}

/* API to stop sending mkeep_alive packet. */
wifi_error wifi_stop_sending_offloaded_packet(wifi_request_id index, wifi_interface_handle iface)
{
    if (index > 0 && index <= N_AVAIL_ID) {
        MKeepAliveCommand *cmd = new MKeepAliveCommand(iface, index, STOP_MKEEP_ALIVE);
        wifi_error result = (wifi_error)cmd->start();
        delete cmd;
        return result;
    } else {
        ALOGE("Invalid mkeep_alive parameters");
        return  WIFI_ERROR_INVALID_ARGS;
    }
}
