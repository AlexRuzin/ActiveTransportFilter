#pragma once

#include <ntddk.h>

#include <initguid.h>
#include <guiddef.h>

#include <inaddr.h>

#include "../common/common.h"
#include "../common/errors.h"
#include "../common/user_driver_transport.h"

//
// Layers which will be enabled by the filter engine
//  This structure is populated by the config ini, and passed to filter.c, 
//  which will then instruct WFP whether or not to initialize a layer
// 
// Further layer descriptors and callbacks are in wfp.c, so adding a layer
//  requires modification of wfp.c (descList), ini file, USER_DRIVER_FILTER_TRANSPORT_DATA
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
//  a USER_DRIVER_FILTER_TRANSPORT_DATA structure through IOCTL.
// 
//
#pragma pack(push, 1)
typedef struct _config_ctx {
    BOOLEAN                         isValidConfig; //placeholder

    // List of WFP callouts to register, as configured by the user
    UINT8                           numOfLayers;
    ENABLED_LAYER                   enabledLayers[MAX_CALLOUT_LAYER_DATA];

    // A n-length pool, aligned by 32-bits, representing all known IPv4 addresses
    UINT16                          numOfIpv4Addresses;
    struct in_addr                  *ipv4AddressPool;

    // IPv6 blacklist pool
    UINT16                          numOfIpv6Addresses;
    IPV6_RAW_ADDRESS                *ipv6AddressPool;
} CONFIG_CTX, *PCONFIG_CTX;
#pragma pack(pop)

//
// Initialize the default configuration
//
ATF_ERROR AtfAllocDefaultConfig(const USER_DRIVER_FILTER_TRANSPORT_DATA *data, CONFIG_CTX **cfgCtx);

//
// Free the config context structure
//
VOID AtfFreeConfig(CONFIG_CTX *ctx);
