#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

#include <inaddr.h>

//
// This header contains the transport configuration between usermode and kernelmode
//  Includes structures, constants, and objects, that can be safely transported to the ATF driver
//

//
// When appending ipv4 blacklists, this will be the maximum size of the packet
//  The addresses may be appended in subsequent calls
//
#define BLACKLIST_IPV4_MAX_SIZE                             512

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

#define MAX_IPV4_ADDRESSES_BLACKLIST                        128
#define MAX_IPV6_ADDRESSES_BLACKLIST                        128

//
// Structure to represent the transport buffer which configures ATF.
//  Configured by the usermode service, through ini, and transported to the ATF driver which will then configure WFP
//
#define FILTER_TRANSPORT_MAGIC                              0x3af3bbcc

//
// Action types against the blocklist (see ini)
//
typedef enum {
    ACTION_PASS,        // Do nothing with the blocklist. Default value
    ACTION_BLOCK,       // Block/drop the packet on blocklist
    ACTION_ALERT        // Alert on blocklist
} ACTION_OPTS;

//
// Primary struct sent via IOCTL to configure filter.c
//
#pragma pack(push, 1)
typedef struct _atf_config_hdr {
    // Object sanity
    UINT32                                                  magic;
    UINT16                                                  structHeaderSize;

    // Layer config
    BOOLEAN                                                 enableLayerIpv4TcpInbound;
    BOOLEAN                                                 enableLayerIpv4TcpOutbound;
    BOOLEAN                                                 enableLayerIpv6TcpInbound;
    BOOLEAN                                                 enableLayerIpv6TcpOutbound;
    BOOLEAN                                                 enableLayerIpv4Datagram;
    BOOLEAN                                                 enableLayerIcmpv4;

    //
    // Alert on direction
    //
    BOOLEAN                                                 alertInbound;
    BOOLEAN                                                 alertOutbound;

    //
    // Action configs
    //
    ACTION_OPTS                                             ipv4BlocklistAction;
    ACTION_OPTS                                             ipv6BlocklistAction;
    ACTION_OPTS                                             dnsBlocklistAction;

    //
    // DNS config
    //
    BOOLEAN                                                 enableDnsBlackhole;

    // Blacklist for all IPv6 addresses
    //  Note: the default config (ini) will only contain the manually entered addresses, so it will
    //  never exceeed MAX_IPV4_ADDRESSES_BLACKLIST
    // Additional blacklist addresses can be appeneded with a separate command
    UINT16                                                  numOfIpv6Addresses;
    IPV6_RAW_ADDRESS                                        ipv6Blacklist[MAX_IPV4_ADDRESSES_BLACKLIST];

    // Blacklist for all IPv4 addresses
    UINT16                                                  numOfIpv4Addresses;
    struct in_addr                                          ipv4BlackList[MAX_IPV6_ADDRESSES_BLACKLIST];

    // Placeholder for DNS buffer
    UINT16                                                  dnsPlaceholderBufSize;

    //
    // DNS header follows at offset + sizeof(struct _atf_config_hdr)
    //
} ATF_CONFIG_HDR, *PATF_CONFIG_HDR;

#pragma pack(pop)