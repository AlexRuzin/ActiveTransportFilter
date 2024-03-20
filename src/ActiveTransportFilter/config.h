#pragma once

#include <ntddk.h>

#include "../common/errors.h"
#include "../common/user_driver_transport.h"

//
// Represents the entire filter configuration context
//  A pointer to this object is returned by AtfAllocDefaultConfig, when the usermode supplies
//  a USER_DRIVER_FILTER_TRANSPORT_DATA structure through IOCTL.
// 
//
#pragma pack(push, 1)
typedef struct _config_ctx {
    BOOLEAN                         isValidConfig; //placeholder

    // Layer configs
    BOOLEAN                         enableLayerIpv4TcpInbound;
    BOOLEAN                         enableLayerIpv4TcpOutbound;
    BOOLEAN                         enableLayerIpv6TcpInbound;
    BOOLEAN                         enableLayerIpv6TcpOutbound;
    BOOLEAN                         enableLayerIcmpv4;

    // A n-length pool, aligned by 32-bits, representing all known IPv4 addresses
    UINT16                          numOfIpv4Addresses;
    IPV4_RAW_ADDRESS                *ipv4AddressPool;

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
