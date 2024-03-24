#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

#include <ntddk.h>

// For handling WFP layer guids
#include <initguid.h>

#include "../common/user_driver_transport.h"
#include "../common/errors.h"

#include "config.h"

//https://stackoverflow.com/questions/32786493/reversing-byte-order-in-c
#define reverse_byte_order_uint32_t(num)     ( ((num & 0xFF000000) >> 24) | ((num & 0x00FF0000) >> 8) | ((num & 0x0000FF00) << 8) | ((num & 0x000000FF) << 24) )

enum _flow_direction {
    _flow_direction_outbound,
    _flow_direction_inbound
};

#pragma pack(push, 1)
typedef struct _atf_filter_conn_data {
    struct in_addr              source;
    struct in_addr              dest;

    SERVICE_PORT                sourcePort;
    SERVICE_PORT                destPort;
} ATF_FLT_DATA_IPV4, *PATF_FLT_DATA_IPV4;
#pragma pack(pop)

//
// Initialize the filter engine
//
VOID AtfFilterInit(VOID);

//
// Flush the current config entirely
//
VOID AtfFilterFlushConfig(VOID);

//
// Returns TRUE if a config exists
//  A filter config must exist for the WFP callouts to be registered
//
BOOLEAN AtfFilterIsInitialized(VOID);

//
// Store the current default ini configuration into the filter engine
//
VOID AtfFilterStoreDefaultConfig(const CONFIG_CTX *confgCtx);

//
// Filter callback for IPv4 (TCP) 
//
ATF_ERROR AtfFilterCallbackTcpIpv4(enum _flow_direction dir, const ATF_FLT_DATA_IPV4 *data);

//
// Returns a TRUE is a WFP filter layer guid is to be enabled 
//  This data is supplied by the ini file and stored in filter.c's CONFIG_CTX object
//
BOOLEAN AtfFilterIsLayerEnabled(const GUID *guid);

//
// Returns the current config. Used to append an IPv4 blacklist to the existing config
//
CONFIG_CTX *AtfFilterGetCurrentConfig(VOID);

