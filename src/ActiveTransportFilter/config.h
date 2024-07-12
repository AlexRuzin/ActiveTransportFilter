#pragma once

#include <ntddk.h>

#include <initguid.h>
#include <guiddef.h>

#include <inaddr.h>

#include "../common/common.h"
#include "../common/errors.h"
#include "../common/user_driver_transport.h"

#include "ipv4_trie.h"

//
// Layers which will be enabled by the filter engine
//  This structure is populated by the config ini, and passed to filter.c, 
//  which will then instruct WFP whether or not to initialize a layer
// 
// Further layer descriptors and callbacks are in wfp.c, so adding a layer
//  requires modification of wfp.c (descList), ini file, ATF_CONFIG_HDR
//
#pragma pack(push, 1)
typedef struct _enabled_layer {
    const GUID                  *layerGuid;
    BOOLEAN                     enabled;
} ENABLED_LAYER, *PENABLED_LAYER;
#pragma pack(pop)

//
// Represents the entire filter configuration context
//  A pointer to this object is returned by AtfAllocDefaultConfig, when the usermode supplies
//  a ATF_CONFIG_HDR structure through IOCTL.
// 
//
typedef struct _config_ctx {
    BOOLEAN                         isValidConfig; //placeholder

    // List of WFP callouts to register, as configured by the user
    size_t                          numOfLayers;
    ENABLED_LAYER                   enabledLayers[MAX_CALLOUT_LAYER_DATA];

    // A n-length pool, aligned by 32-bits, representing all known IPv4 addresses
    size_t                          numOfIpv4Addresses;
    struct in_addr                  *ipv4AddressPool;

    //
    // Sortied trie for fast processing
    //
    IPV4_TRIE_CTX                   *ipv4TrieCtx;

    // IPv6 blacklist pool
    size_t                          numOfIpv6Addresses;
    IPV6_RAW_ADDRESS                *ipv6AddressPool;

    //
    // Action switches
    //
    ACTION_OPTS                     ipv4BlocklistAction;
    ACTION_OPTS                     ipv6BlocklistAction;
    ACTION_OPTS                     dnsBlocklistAction; 

    //
    // Direction switches
    //
    BOOLEAN                         alertInbound;
    BOOLEAN                         alertOutbound;
} CONFIG_CTX, *PCONFIG_CTX;

//
// Initialize the default configuration
//
ATF_ERROR AtfAllocDefaultConfig(const ATF_CONFIG_HDR *data, CONFIG_CTX **cfgCtx);

//
// Append a new blocklist array to the config
//
ATF_ERROR AtfConfigAddIpv4Blacklist(CONFIG_CTX *ctx, const VOID *blacklist, size_t bufLen);

//
// Free the config context structure
//
VOID AtfFreeConfig(CONFIG_CTX *ctx);
