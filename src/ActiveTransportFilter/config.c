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
#include <string.h>

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
static BOOLEAN AtfCfgSanityCheck(const ATF_CONFIG_HDR *data, SIZE_T dataSize);

//
// DNS pool sanity check
//
static BOOLEAN AtfCfgValidateDnsPool(const VOID *pool, SIZE_T poolSize);

//
// Create the default config
//
ATF_ERROR AtfCfgAllocDefaultConfig(const ATF_CONFIG_HDR *data, SIZE_T dataSize, CONFIG_CTX **cfgCtx)
{
    if (!cfgCtx || dataSize == 0) {
        return ATF_BAD_PARAMETERS;
    }
    *cfgCtx = NULL;

    ATF_ERROR atfError = ATF_ERROR_OK;

    if (!AtfCfgSanityCheck(data, dataSize)) {
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
    ADD_WFP_LAYER(data->enableLayerIpv4Datagram, &FWPM_LAYER_DATAGRAM_DATA_V4);
    ADD_WFP_LAYER(data->enableLayerIcmpv4, &FWPM_CONDITION_ORIGINAL_ICMP_TYPE);

    out->numOfIpv4Addresses                     = data->numOfIpv4Addresses;
    out->numOfIpv6Addresses                     = data->numOfIpv6Addresses;

    // Set actions
    out->ipv4BlocklistAction                    = data->ipv4BlocklistAction;
    out->ipv6BlocklistAction                    = data->ipv6BlocklistAction;
    out->dnsBlocklistAction                     = data->dnsBlocklistAction;

    // Directions
    out->alertInbound                           = data->alertInbound;
    out->alertOutbound                          = data->alertOutbound;

    //
    // Allocate a new trie pool even if we don't have any blacklisted IPs
    //
    atfError = AtfIpv4TrieAllocCtx(&out->ipv4TrieCtx);
    if (atfError) {
        return atfError;
    }

    if (out->numOfIpv4Addresses) {
        const SIZE_T sizeOfIpv4Pool = out->numOfIpv4Addresses * sizeof(struct in_addr);
        out->ipv4AddressPool = (struct in_addr *)ATF_MALLOC(sizeOfIpv4Pool);
        if (!out) {
            ATF_FREE(out);
            return ATF_NO_MEMORY_AVAILABLE;
        }

        RtlCopyMemory(out->ipv4AddressPool, data->ipv4BlackList, sizeOfIpv4Pool);

        atfError = AtfIpv4TrieInsertPool(
            out->ipv4TrieCtx, 
            (const struct in_addr *)out->ipv4AddressPool, 
            out->numOfIpv4Addresses
        );
        if (atfError) {
            return atfError;
        }
    }

    if (out->numOfIpv6Addresses) {
        const SIZE_T sizeOfIpv6Pool = out->numOfIpv6Addresses * sizeof(IPV6_RAW_ADDRESS);
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

    // Alloc raw dns list pool
    const VOID *dnsPoolPtr = (VOID *)((DWORD_PTR)data + data->dnsBufferSize);
    if (data->dnsBufferSize > 0 && AtfCfgValidateDnsPool(dnsPoolPtr, data->dnsBufferSize)) {
        out->rawDnsBuffer = ATF_MALLOC(data->dnsBufferSize);
        RtlCopyMemory(out->rawDnsBuffer, (VOID *)((DWORD_PTR)data + data->dnsBufferSize), data->dnsBufferSize);
        out->rawDnsBufferSize = data->dnsBufferSize;
    }

    *cfgCtx = out;

    return ATF_ERROR_OK;
}

static BOOLEAN AtfCfgCheckValidTld(const LPSTR str, __out_opt SIZE_T *outSize)
{
    if (!str) {
        return FALSE;
    }

    static const SIZE_T maxDomainSize = 0xff - 1;

    SIZE_T len = 0;
    RtlStringCchLengthA(str, maxDomainSize, &len);
    if (len >= maxDomainSize) {
        return FALSE;
    }

    if (!strstr(str, ".")) {
        return FALSE;
    }

    if (outSize) {
        *outSize = len;
    }

    return TRUE;
}

//
// Validates DNS pool
//
static BOOLEAN AtfCfgValidateDnsPool(const VOID *pool, SIZE_T poolSize) 
{
    if (!pool || poolSize == 0) {
        return FALSE;
    }

    CHAR *ptr = (CHAR *)pool;
    const CHAR *maxPtr = (const CHAR *)((DWORD_PTR)pool + poolSize);

    while (ptr < maxPtr) {
        SIZE_T domainSize = 0;

        if (!AtfCfgCheckValidTld(ptr, &domainSize)) {
            return FALSE;
        }

        ptr += domainSize;
    }

    return TRUE;
}

//
// Append a new blocklist array to the config
//
ATF_ERROR AtfCfgAddIpv4Blacklist(CONFIG_CTX *ctx, const VOID *blacklist, SIZE_T bufLen)
{
    ATF_ERROR atfError = ATF_ERROR_OK;

    const SIZE_T numOfIps = bufLen / sizeof(struct in_addr);

    if (!ctx || !blacklist || !bufLen || bufLen % sizeof(struct in_addr)) {
        return ATF_BAD_PARAMETERS;
    }

    if (bufLen > BLACKLIST_IPV4_MAX_SIZE) {
        return ATF_IOCTL_BUFFER_TOO_LARGE;
    }

    //TODO: cleanup the pool
    if (ctx->ipv4AddressPool) {
        //Append
        struct in_addr *newPool = ATF_MALLOC(bufLen + (ctx->numOfIpv4Addresses * sizeof(struct in_addr)));
        if (!newPool) {
            return ATF_NO_MEMORY_AVAILABLE;
        }

        RtlCopyMemory(newPool, ctx->ipv4AddressPool, (ctx->numOfIpv4Addresses * sizeof(struct in_addr)));
        RtlCopyMemory(&newPool[ctx->numOfIpv4Addresses], blacklist, bufLen);

        ATF_FREE(ctx->ipv4AddressPool);
        ctx->ipv4AddressPool = newPool;
        ctx->numOfIpv4Addresses += numOfIps;
    } else {
        //Assign
        atfError = AtfIpv4TrieAllocCtx(&ctx->ipv4TrieCtx);
        if (atfError) {
            return atfError;
        }

        ctx->ipv4AddressPool = (struct in_addr *)ATF_MALLOC(bufLen);
        if (!ctx->ipv4AddressPool) {
            return ATF_NO_MEMORY_AVAILABLE;
        }

        RtlCopyMemory(ctx->ipv4AddressPool, blacklist, bufLen);
    }

    // Append IP pool to trie
    atfError = AtfIpv4TrieInsertPool(ctx->ipv4TrieCtx, (const struct in_addr *)blacklist, numOfIps);
    if (atfError) {
        return atfError;
    }

    return atfError;
}

static BOOLEAN AtfCfgSanityCheck(const ATF_CONFIG_HDR *data, SIZE_T dataSize)
{
    if (!data || dataSize == 0) {
        return FALSE;
    }

    // Size checks
    if (data->magic != FILTER_TRANSPORT_MAGIC || 
        data->structHeaderSize != sizeof(ATF_CONFIG_HDR) ||
        dataSize != (sizeof(ATF_CONFIG_HDR) + data->dnsBufferSize)
    ) {
        ATF_ERROR(AtfCfgSanityCheck, ATF_CORRUPT_CONFIG);
        return FALSE;
    }

    if (!(
        // At least one layer needs to be enabled, otherwise fail
        data->enableLayerIpv4TcpInbound |
        data->enableLayerIpv4TcpOutbound |
        data->enableLayerIpv6TcpInbound |
        data->enableLayerIpv6TcpOutbound |
        data->enableLayerIpv4Datagram |
        data->enableLayerIcmpv4        
        ))
    {
        ATF_DEBUG(AtfCfgSanityCheck, "All layers have been disabled by user, ATF will not be initialized.");
        return FALSE;
    }

    if (!data->numOfIpv4Addresses) {
        ATF_DEBUG(AtfCfgSanityCheck, "No ipv4 addresses found in user ini, continuing.");
    } else if (data->numOfIpv4Addresses > MAX_IPV4_ADDRESSES_BLACKLIST) {
        ATF_DEBUG(AtfCfgSanityCheck, "ini cannot exceed MAX_IPV4_ADDRESSES_BLACKLIST addresses");
    }

    if (!data->numOfIpv6Addresses) {
        ATF_DEBUG(AtfCfgSanityCheck, "No ipv6 addresses found in user ini, continuing.");
    } else if (data->numOfIpv6Addresses > MAX_IPV6_ADDRESSES_BLACKLIST) {
        ATF_DEBUG(AtfCfgSanityCheck, "ini cannot exceed MAX_IPV6_ADDRESSES_BLACKLIST addresses");
    }

    // Check that all IPs in the blacklist are > 0.0.0.0
    for (UINT16 i = 0; i < data->numOfIpv4Addresses; i++) {
        if (data->ipv4BlackList[i].S_un.S_addr == 0x00000000) {
            ATF_DEBUG(AtfCfgSanityCheck, "An ipv4 gateway address was provided. Bad config.");
            return FALSE;
        }
    }

    for (UINT16 i = 0; i < data->numOfIpv6Addresses; i++) {
        if (!(data->ipv6Blacklist[i].a.q.qword[0] | data->ipv6Blacklist[i].a.q.qword[1])) {
            ATF_DEBUG(AtfCfgSanityCheck, "An ipv6 gateway address was provided. Bad config.");
            return FALSE;
        }
    }

    // Check buffer sanity (TODO)
    {
        const char *p = (char *)((DWORD_PTR)data + sizeof(data));
        if (*p == '\0') {
            return FALSE;
        }
    }

    return TRUE;
}

//
// Free config object
//
VOID AtfCfgFreeCtx(CONFIG_CTX **cfgCtx)
{
    if (!cfgCtx) {
        return;
    }

    CONFIG_CTX *p = *cfgCtx;

    if (p->rawDnsBuffer && p->rawDnsBufferSize > 0) {
        RtlZeroMemory(p->rawDnsBuffer, p->rawDnsBufferSize);
        ATF_FREE(p->rawDnsBuffer);
    }

    if (p->ipv4TrieCtx) {
        AtfIpv4TrieFree(&p->ipv4TrieCtx);
    }

    if (p->ipv6AddressPool) {
        ATF_FREE(p->ipv6AddressPool);
    }

    if (p->ipv4AddressPool) {
        RtlZeroMemory(p->ipv4AddressPool, sizeof(struct in_addr) * p->numOfIpv4Addresses);
        ATF_FREE(p->ipv4AddressPool);
    }

    RtlZeroMemory(p, sizeof(CONFIG_CTX));
    ATF_FREE(p);
    *cfgCtx = NULL;
}

// eof