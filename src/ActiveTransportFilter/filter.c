#include <ntddk.h>

#include <ntstrsafe.h>
#include <ip2string.h>
#include <inaddr.h>

#include <initguid.h>
#include <guiddef.h>

#include "filter.h"

#include "../common/errors.h"
#include "trace.h"

#include "config.h"

//
// Current config context structure (may be modified by config.cpp)
//
static CONFIG_CTX* gConfigCtx = NULL;

//
// For debugging, print the target ip from DWORd to string
//
static VOID AtfFilterPrintIP(enum _flow_direction dir, const ATF_FLT_DATA_IPV4 *data);

VOID AtfFilterInit(VOID)
{
    gConfigCtx = NULL;
}

//
// If an existing config exists, then
VOID AtfFilterStoreDefaultConfig(const CONFIG_CTX *configCtx)
{
    if (gConfigCtx != NULL) {
        AtfFilterFlushConfig();
    }

    gConfigCtx = (CONFIG_CTX *)configCtx;

    ATF_DEBUG(AtfFilterStoreDefaultConfig, "Successfully loaded filter config");
}

VOID AtfFilterFlushConfig(VOID)
{
    if (gConfigCtx) {
        AtfFreeConfig(gConfigCtx);
        gConfigCtx = NULL;
    }
}
    
//
// Returns TRUE if a config exists
//  A filter config must exist for the WFP callouts to be registered
//
BOOLEAN AtfFilterIsInitialized(VOID)
{
    if (gConfigCtx != NULL) {
        return TRUE;
    }

    return FALSE;
}

BOOLEAN AtfFilterIsLayerEnabled(const GUID *guid)
{
    // IOCTL locks are sufficient for this function, as WFP is in the process of initializing

    // Sanity checks just in case
    if (!guid) {
        return FALSE;
    }

    if (!AtfFilterIsInitialized()) {
        return FALSE;
    }

    if (gConfigCtx->numOfLayers == 0) {
        ATF_DEBUG(AtfFilterIsLayerEnabled, "Warning: 0 enabled WFP layers in config");
        return FALSE;
    }

    for (UINT8 i = 0; i < gConfigCtx->numOfLayers; i++) {
        if (IsEqualGUID(guid, gConfigCtx->enabledLayers[i].layerGuid)) {
            return gConfigCtx->enabledLayers[i].enabled;
        }
    }

    return FALSE;
}

//
// Filter callback for IPv4 (TCP) 
//
ATF_ERROR AtfFilterCallbackTcpIpv4(enum _flow_direction dir, const ATF_FLT_DATA_IPV4 *data)
{
    if (data == NULL) {
        return ATF_BAD_PARAMETERS;
    }    

    AtfFilterPrintIP(dir, data);

    return ATF_ERROR_OK;
}

static VOID AtfFilterPrintIP(enum _flow_direction dir, const ATF_FLT_DATA_IPV4 *data)
{
    // Convert from big-endian to little-endian and print flow to debug

    // Source IP
    struct in_addr leSrcIp; 
    leSrcIp.S_un.S_addr = reverse_byte_order_uint32_t((UINT32)data->source.S_un.S_addr);

    CHAR ipAddrStrSrc[32] = { 0 };
    RtlIpv4AddressToStringA(&leSrcIp, ipAddrStrSrc);

    // Dest IP
    struct in_addr leDestIp;
    leDestIp.S_un.S_addr = reverse_byte_order_uint32_t((UINT32)data->dest.S_un.S_addr);

    CHAR ipAddrStrDst[32] = { 0 };
    RtlIpv4AddressToStringA(&leDestIp, ipAddrStrDst);

    if (dir == _flow_direction_inbound) {
        ATF_DEBUGA("New TCP flow: INBOUND (src: %s:%d, dst: %s:%d)\n", 
            ipAddrStrSrc, data->sourcePort, ipAddrStrDst, data->destPort);
    } else if (dir == _flow_direction_outbound) {
        ATF_DEBUGA("New TCP flow: OUTBOUND (src: %s:%d, dst: %s:%d)\n", 
            ipAddrStrDst, data->destPort, ipAddrStrSrc, data->sourcePort);
    }

    RtlZeroMemory(ipAddrStrSrc, 32);
    RtlZeroMemory(ipAddrStrDst, 32);
}