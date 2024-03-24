#if !defined(NT)
#define NT
#endif //NT

//
// Require ndis and wfp for layer GUIDs
//
#if !defined(NDIS60)
#define NDIS60 1
#endif //NDIS60

#if !defined(NDIS_SUPPORT_NDIS6)
#define NDIS_SUPPORT_NDIS6 1
#endif //NDIS_SUPPORT_NDIS6

#include <ntddk.h>
#include <fwpsk.h>
#include <fwpmk.h>

#include "config.h"

#include "trace.h"
#include "mem.h"
#include "../common/errors.h"
#include "../common/user_driver_transport.h"

//
// Simple define for checking if a layer needs to be enabled or not
//
#define ADD_WFP_LAYER(isEnabled, guidPtr) \
{ \
    out->enabledLayers[out->numOfLayers].layerGuid = guidPtr; \
    out->enabledLayers[out->numOfLayers].enabled = isEnabled; \
    out->numOfLayers++; \
} 

//
// Do check on the data coming in from user mode; validate
//
static BOOLEAN AtfIniConfigSanityCheck(const USER_DRIVER_FILTER_TRANSPORT_DATA *data);

//
// Sort IP address pool in ascending order
//
static VOID AtfConfigSortIpv4List(USER_DRIVER_FILTER_TRANSPORT_DATA *data);

//
// Create the default config
//
ATF_ERROR AtfAllocDefaultConfig(const USER_DRIVER_FILTER_TRANSPORT_DATA *data, CONFIG_CTX **cfgCtx)
{
    if (!cfgCtx) {
        return ATF_BAD_PARAMETERS;
    }

    *cfgCtx = NULL;

    if (!AtfIniConfigSanityCheck(data)) {
        return ATF_CORRUPT_CONFIG;
    }

    CONFIG_CTX *out = ATF_MALLOC(sizeof(CONFIG_CTX));
    if (!out) {
        return ATF_NO_MEMORY_AVAILABLE;
    }

    out->isValidConfig = TRUE;

    ADD_WFP_LAYER(data->enableLayerIpv4TcpInbound, &FWPM_LAYER_INBOUND_TRANSPORT_V4);
    ADD_WFP_LAYER(data->enableLayerIpv4TcpOutbound, &FWPM_LAYER_OUTBOUND_TRANSPORT_V4);
    ADD_WFP_LAYER(data->enableLayerIpv6TcpInbound, &FWPM_LAYER_INBOUND_TRANSPORT_V6);
    ADD_WFP_LAYER(data->enableLayerIpv6TcpOutbound, &FWPM_LAYER_OUTBOUND_TRANSPORT_V6);
    ADD_WFP_LAYER(data->enableLayerIcmpv4, &FWPM_CONDITION_ORIGINAL_ICMP_TYPE);

    out->numOfIpv4Addresses                     = data->numOfIpv4Addresses;
    out->numOfIpv6Addresses                     = data->numOfIpv6Addresses;

    if (out->numOfIpv4Addresses) {
        const UINT32 sizeOfIpv4Pool = out->numOfIpv4Addresses * sizeof(struct in_addr);
        out->ipv4AddressPool = (struct in_addr *)ATF_MALLOC(sizeOfIpv4Pool);
        if (!out) {
            ATF_FREE(out);
            return ATF_NO_MEMORY_AVAILABLE;
        }

        RtlCopyMemory(out->ipv4AddressPool, data->ipv4BlackList, sizeOfIpv4Pool);
    }

    if (out->numOfIpv6Addresses) {
        const UINT32 sizeOfIpv6Pool = out->numOfIpv6Addresses * sizeof(IPV6_RAW_ADDRESS);
        out->ipv4AddressPool = (struct in_addr *)ATF_MALLOC(sizeOfIpv6Pool);
        if (!out) {
            if (out->ipv4AddressPool) {
                ATF_FREE(out->ipv4AddressPool);
            }
            ATF_FREE(out);
            return ATF_NO_MEMORY_AVAILABLE;
        }

        RtlCopyMemory(out->ipv6AddressPool, data->ipv6Blacklist, sizeOfIpv6Pool);
    }

    *cfgCtx = out;

    return ATF_ERROR_OK;
}

static VOID AtfConfigSortIpv4List(USER_DRIVER_FILTER_TRANSPORT_DATA *data)
{
    if (!data || !data->ipv4BlackList || !data->numOfIpv4Addresses) {
        return;
    }


}

VOID AtfFreeConfig(CONFIG_CTX *ctx)
{
    UNREFERENCED_PARAMETER(ctx);
}

static BOOLEAN AtfIniConfigSanityCheck(const USER_DRIVER_FILTER_TRANSPORT_DATA *data)
{
    if (!data) {
        return FALSE;
    }

    if (data->magic != FILTER_TRANSPORT_MAGIC || data->size != sizeof(USER_DRIVER_FILTER_TRANSPORT_DATA)) {
        ATF_ERROR(AtfIniConfigSanityCheck, ATF_CORRUPT_CONFIG);
        return FALSE;
    }

    if (!(
        // At least one layer needs to be enabled, otherwise fail
        data->enableLayerIpv4TcpInbound |
        data->enableLayerIpv4TcpOutbound |
        data->enableLayerIpv6TcpInbound |
        data->enableLayerIpv6TcpOutbound |
        data->enableLayerIcmpv4        
        ))
    {
        ATF_DEBUG(AtfIniConfigSanityCheck, "All layers have been disabled by user, ATF will not be initialized.");
        return FALSE;
    }

    if (!data->numOfIpv4Addresses) {
        ATF_DEBUG(AtfIniConfigSanityCheck, "No ipv4 addresses found in user ini, continuing.");
    } else if (data->numOfIpv4Addresses > MAX_IPV4_ADDRESSES_BLACKLIST) {
        ATF_DEBUG(AtfIniConfigSanityCheck, "ini cannot exceed MAX_IPV4_ADDRESSES_BLACKLIST addresses");
    }

    if (!data->numOfIpv6Addresses) {
        ATF_DEBUG(AtfIniConfigSanityCheck, "No ipv6 addresses found in user ini, continuing.");
    } else if (data->numOfIpv6Addresses > MAX_IPV6_ADDRESSES_BLACKLIST) {
        ATF_DEBUG(AtfIniConfigSanityCheck, "ini cannot exceed MAX_IPV6_ADDRESSES_BLACKLIST addresses");
    }

    // Check that all IPs in the blacklist are > 0.0.0.0
    for (UINT16 i = 0; i < data->numOfIpv4Addresses; i++) {
        if (data->ipv4BlackList[i].S_un.S_addr == 0x00000000) {
            ATF_DEBUG(AtfIniConfigSanityCheck, "An ipv4 gateway address was provided. Bad config.");
            return FALSE;
        }
    }

    for (UINT16 i = 0; i < data->numOfIpv6Addresses; i++) {
        if (!(data->ipv6Blacklist[i].a.q.qword[0] | data->ipv6Blacklist[i].a.q.qword[1])) {
            ATF_DEBUG(AtfIniConfigSanityCheck, "An ipv6 gateway address was provided. Bad config.");
            return FALSE;
        }
    }

    return TRUE;
}