#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

#if !defined(NT)
#define NT
#endif //NT

#if !defined(NDIS60)
#define NDIS60 1
#endif //NDIS60

#if !defined(NDIS_SUPPORT_NDIS6)
#define NDIS_SUPPORT_NDIS6 1
#endif //NDIS_SUPPORT_NDIS6

#include <ntddk.h>
#include <fwpsk.h>
#include <fwpmk.h>

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
    struct in_addr              localIp;
    struct in_addr              remoteIp;

    SERVICE_PORT                localPort;
    SERVICE_PORT                remotePort;

    CHAR                        fqDnsName[0xff]; //RFC1035
    CHAR                        domainName[0xff];

    // IP strings
    CHAR                        localIpStr[16];
    CHAR                        remoteIpStr[16];
} ATF_FLT_DATA, *PATF_FLT_DATA;
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
// Filter callback for IPv4 (TCP & datagram)
//
ATF_ERROR AtfFilterCallbackIpv4(
    _In_ const FWPS_INCOMING_VALUES0 *fixedValues,
    _Inout_ FWPS_CLASSIFY_OUT0 *classifyOut,
    _In_ enum _flow_direction dir
);

//
// Returns a TRUE is a WFP filter layer guid is to be enabled 
//  This data is supplied by the ini file and stored in filter.c's CONFIG_CTX object
//
BOOLEAN AtfFilterIsLayerEnabled(const GUID *guid);

//
// Returns the current config. Used to append an IPv4 blacklist to the existing config
//
CONFIG_CTX *AtfFilterGetCurrentConfig(VOID);

