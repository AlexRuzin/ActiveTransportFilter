#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

#include <ntddk.h>

#include "../common/user_driver_transport.h"
#include "../common/errors.h"

#include "config.h"

#pragma pack(push, 1)
typedef struct _atf_filter_conn_data {
    IPV4_RAW_ADDRESS            source;
    IPV4_RAW_ADDRESS            dest;

    SERVICE_PORT                sourcePort;
    SERVICE_PORT                destPort;
} ATF_FLT_DATA_IPV4, *PATF_FLT_DATA_IPV4;
#pragma pack(pop)

//
// Initialize the filter engine
//
VOID AtfFilterInit(VOID);

//
// Destroy the filter engine and cleanup
//
VOID AtfFilterCleanup(VOID);

//
// Flush the current config entirely
//
VOID AtfFilterFlushConfig(VOID);

//
// Store the current default ini configuration into the filter engine
//
VOID AtfFilterStoreDefaultConfig(const CONFIG_CTX *confgCtx);

//
// Filter callback for IPv4 (TCP) 
//
ATF_ERROR AtfFilterCallbackTcpIpv4Inbound(const ATF_FLT_DATA_IPV4 *data);



