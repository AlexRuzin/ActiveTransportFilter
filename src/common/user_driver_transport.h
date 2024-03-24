#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

#include <inaddr.h>

//
// This header contains the transport configuration between usermode and kernelmode
//  Includes structures, constants, and objects, that can be safely transported to the ATF driver
//

//
// Object to represent an IPv6 address (TODO: replace with standard API)
//
#pragma pack(push, 1)
typedef struct _ipv6RawAddress {
    union {
        struct {
            UINT8                                           byte[16];
        } b;

        struct {
            UINT32                                          dword[4];
        } d;

        struct {
            UINT64                                          qword[2];
        } q;
    } a;
} IPV6_RAW_ADDRESS, *PIPV6_RAW_ADDRESS;
#pragma pack(pop)

//
// Global definitions for ports (2^16 in size)
//
typedef UINT16                                              _servicePort;
typedef _servicePort                                        SERVICE_PORT;

#define MAX_IPV4_ADDRESSES_BLACKLIST                        512
#define MAX_IPV6_ADDRESSES_BLACKLIST                        512

//
// Structure to represent the transport buffer which configures ATF.
//  Configured by the usermode service, through ini, and transported to the ATF driver which will then configure WFP
//
#define FILTER_TRANSPORT_MAGIC                              0x3af3bbcc

#pragma pack(push, 1)
typedef struct _user_driver_filter_transport_data {
    // Object sanity
    UINT32                                                  magic;
    UINT16                                                  size;

    // Layer config
    UINT8                                                   enableLayerIpv4TcpInbound;
    UINT8                                                   enableLayerIpv4TcpOutbound;
    UINT8                                                   enableLayerIpv6TcpInbound;
    UINT8                                                   enableLayerIpv6TcpOutbound;
    UINT8                                                   enableLayerIcmpv4;

    // Blacklist for all IPv6 addresses
    //  Note: the default config (ini) will only contain the manually entered addresses, so it will
    //  never exceeed MAX_IPV4_ADDRESSES_BLACKLIST
    // Additional blacklist addresses can be appeneded with a separate command
    UINT16                                                  numOfIpv6Addresses;
    IPV6_RAW_ADDRESS                                        ipv6Blacklist[MAX_IPV4_ADDRESSES_BLACKLIST];

    // Blacklist for all IPv4 addresses
    UINT16                                                  numOfIpv4Addresses;
    struct in_addr                                          ipv4BlackList[MAX_IPV6_ADDRESSES_BLACKLIST];
} USER_DRIVER_FILTER_TRANSPORT_DATA, *PUSER_DRIVER_FILTER_TRANSPORT_DATA;
#pragma pack(pop)