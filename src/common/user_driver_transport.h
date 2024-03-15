#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

//
// This header contains the transport configuration between usermode and kernelmode
//  Includes structures, constants, and objects, that can be safely transported to the ATF driver
//

#include <stdint.h>

//
// Object to represent an IPv6 address
//
#pragma pack(push, 1)
typedef struct _ipv6RawAddress {
    union {
        uint8_t                                             address[16];
        uint64_t                                            part[2];
    };
} IPV6_RAW_ADDRESS, *PIPV6_RAW_ADDRESS;
#pragma pack(pop)

//
// Object to represent an IPv4 address
//
typedef uint32_t                                            _ipv4RawAddress;
typedef _ipv4RawAddress                                     IPV4_RAW_ADDRESS;
typedef _ipv4RawAddress                                     *PIPV4_RAW_ADDRESS;

#define MAX_IPV4_ADDRESSES_BLACKLIST                        512
#define MAX_IPV6_ADDRESSES_BLACKLIST                        512

//
// Structure to represent the transport buffer which configures ATF.
//  Configured by the usermode service, through ini, and transported to the ATF driver which will then configure WFP
//
#pragma pack(push, 1)
typedef struct _user_driver_filter_transport_data {

    // Blacklist for all IPv6 addresses
    IPV6_RAW_ADDRESS                                        ipv6Blacklist[MAX_IPV4_ADDRESSES_BLACKLIST];

    // Blacklist for all IPv4 addresses
    IPV4_RAW_ADDRESS                                        ipv4BlackList[MAX_IPV6_ADDRESSES_BLACKLIST];
} USER_DRIVER_FILTER_TRANSPORT_DATA, *PUSER_DRIVER_FILTER_TRANSPORT_DATA;
#pragma pack(pop)