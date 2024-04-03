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
static VOID AtfFilterPrintIP(enum _flow_direction dir, const ATF_FLT_DATA *data);

//
// Initialize the filter engine
//
VOID AtfFilterInit(VOID)
{
    gConfigCtx = NULL;
}

//
// If an existing config exists, then replace it
//
VOID AtfFilterStoreDefaultConfig(const CONFIG_CTX *configCtx)
{
    if (gConfigCtx != NULL) {
        AtfFilterFlushConfig();
    }

    gConfigCtx = (CONFIG_CTX *)configCtx;

    ATF_DEBUG(AtfFilterStoreDefaultConfig, "Successfully loaded filter config");
}

//
// Return the current config
//
CONFIG_CTX *AtfFilterGetCurrentConfig(VOID)
{
    return gConfigCtx;
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
// Source and dest IPs depend on the direction
//
static ATF_ERROR AtfFilterParsePacket(
    _In_ const FWPS_INCOMING_VALUES0 *fixedValues,
    _Out_ ATF_FLT_DATA *dataOut
)
{
    VALIDATE_PARAMETER(dataOut);
    VALIDATE_PARAMETER(fixedValues);

    RtlZeroMemory(dataOut, sizeof(ATF_FLT_DATA));

    dataOut->localIp.S_un.S_addr = fixedValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_LOCAL_ADDRESS].value.uint32;
    dataOut->remoteIp.S_un.S_addr = fixedValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_REMOTE_ADDRESS].value.uint32;

    dataOut->localPort = (SERVICE_PORT)fixedValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_LOCAL_PORT].value.uint16;
    dataOut->remotePort = (SERVICE_PORT)fixedValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_REMOTE_PORT].value.uint16;

    // Parse IP strings
    IN_ADDR localIp;
    localIp.S_un.S_addr = reverse_byte_order_uint32_t(dataOut->localIp.S_un.S_addr);
    if (!RtlIpv4AddressToStringA(&localIp, dataOut->localIpStr)) {
        return ATF_BAD_DATA;
    }

    IN_ADDR remoteIp;
    remoteIp.S_un.S_addr = reverse_byte_order_uint32_t(dataOut->remoteIp.S_un.S_addr);
    if (!RtlIpv4AddressToStringA(&remoteIp, dataOut->remoteIpStr)) {
        return ATF_BAD_DATA;
    }

    return ATF_ERROR_OK;
}

//
// Filter callback for IPv4 (TCP) 
//
ATF_ERROR AtfFilterCallbackTcpIpv4(
    _In_ const FWPS_INCOMING_VALUES0 *fixedValues,
    _Inout_ FWPS_CLASSIFY_OUT0 *classifyOut,
    _In_ enum _flow_direction dir
)
{
    UNREFERENCED_PARAMETER(dir);

    VALIDATE_PARAMETER(fixedValues);
    VALIDATE_PARAMETER(classifyOut);

    // Parse packet
    ATF_FLT_DATA data;
    if (AtfFilterParsePacket(fixedValues, &data) != ATF_ERROR_OK) {
        // Error in parsing data. This should never happen as the packet comes from WFP, but still, handle it.
        return ATF_ERROR_OK;
    }

    // Default action is PASS
    ATF_ERROR atfError = ATF_FILTER_SIGNAL_PASS;

    // Do not do any action on inbound (if disabled)
    if (dir == _flow_direction_inbound && !gConfigCtx->alertInbound) {
        return atfError;
    }

    // Do not do any action on outbound (if disabled)
    if (dir == _flow_direction_outbound && !gConfigCtx->alertOutbound) {
        return atfError;
    }

    // Default state is PASS
    BOOLEAN srcInTrie = FALSE;
    BOOLEAN destInTrie = FALSE;

    // Search tries (trees?)
    CHAR *badIp = NULL;
    if (AtfIpv4TrieSearch(gConfigCtx->ipv4TrieCtx, data.localIp)) {
        srcInTrie = TRUE;
        badIp = data.localIpStr;
    }
    if (AtfIpv4TrieSearch(gConfigCtx->ipv4TrieCtx, data.remoteIp)) {
        destInTrie = TRUE;
        badIp = data.remoteIpStr;
    }

    if (gConfigCtx->ipv4BlocklistAction == ACTION_BLOCK && (srcInTrie | destInTrie)) {
        atfError = ATF_FILTER_SIGNAL_BLOCK;
    } else if (gConfigCtx->ipv4BlocklistAction == ACTION_ALERT && (srcInTrie | destInTrie)) {
        atfError = ATF_FILTER_SIGNAL_ALERT;
    }

    // Report/log
    if (atfError != ATF_FILTER_SIGNAL_PASS) {
        ATF_DEBUGA("SIGNAL %s (%s): IP: %s (local:%s:%d -> remote:%s:%d)", 
            atfError == ATF_FILTER_SIGNAL_BLOCK ? actionNames[ACTION_BLOCK] : actionNames[ACTION_ALERT],
            dir == _flow_direction_inbound ? "INBOUND" : "OUTBOUND",
            badIp,
            data.localIpStr, data.localPort,
            data.remoteIpStr, data.remotePort
        );
    }

    // Do ops
    switch(atfError)
    {
    case ATF_FILTER_SIGNAL_PASS:
        {
            return atfError;
        } 
        break;
    case ATF_FILTER_SIGNAL_BLOCK:
        {

        }
        break;
    case ATF_FILTER_SIGNAL_ALERT:
        {

        } 
        break;
    default:
        {
            ATF_ERROR(AtfFilterCallbackTcpIpv4Inbound, atfError);
        }
        break;
    }

    return ATF_ERROR_OK;
}
