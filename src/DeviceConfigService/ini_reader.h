#pragma once

#include <Windows.h>

#include <inaddr.h>

#include <INIReader.h>

#include "../common/errors.h"
#include "../common/shared.h"
#include "../common/user_driver_transport.h"

#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <memory>

//
// Struct representing an IP blacklist from online
//  Supplied by ipv4_blacklist_urls_simple in the INI file
//
class IpBlacklistItem {
private:
    const std::string                           blacklistName;
    const std::string                           uri;

    // Parsed IPv4 addresses
    std::vector<struct in_addr>                 blacklist;

    // Raw output vector from CURL
    std::vector<char>                           rawDownloadBuffer;

public:
    IpBlacklistItem(const std::string &blacklistName, const std::string &uri) :
        uri(uri),
        blacklistName(blacklistName)
    {
    
    }

    ~IpBlacklistItem(void)
    {
    
    }

    ATF_ERROR DownloadAndParseBlacklist(void);

private:
    //
    // CURL download
    //
    ATF_ERROR downloadBlocklist(
        const std::string &uri, 
        std::vector<char> &rawBufOut) const;

    static size_t curlWriteCallback(
        const char *buf,
        size_t bufSize,
        size_t nmemb,
        std::vector<char> *out
    );


    //
    // Parse CURL output buffer into std::vector<struct in_addr>
    //
    ATF_ERROR parseBufIntoList(
        const std::vector<char> &buf, 
        std::vector<struct in_addr> &ipOut
    );

public:
    //
    // Return the ip list
    //
    const std::vector<struct in_addr> GetIps(void) const;

    //
    // Return blocklist domain
    //
    const std::string GetName(void) const;
};

class FilterConfig {
private:
    //
    // Global that stores the checksum for the last ini file read (TODO)
    //
    static inline shared::CRC32SUM              lastIniSum = -1;

    bool                                        enableLayerIpv4TcpInbound;
    bool                                        enableLayerIpv4TcpOutbound;
    bool                                        enableLayerIpv6TcpInbound;
    bool                                        enableLayerIpv6TcpOutbound;
    bool                                        enableLayerIpv4Datagram;
    bool                                        enableLayerIcmpv4;

    //
    // Direction switches
    //
    bool                                        alertInbound;
    bool                                        alertOutbound;

    // Blacklist from the default ini config ONLY
    std::vector<struct in_addr>                 blocklistIpv4;
    std::vector<IPV6_RAW_ADDRESS>               blocklistIpv6;

    // Blacklist from the additional, dynamic/online IP blocklists
    std::vector<struct in_addr>                 blocklistIpv4Online;

    //
    // Action configs
    //
    ACTION_OPTS                                 ipv4BlocklistAction;
    ACTION_OPTS                                 ipv6BlocklistAction;
    ACTION_OPTS                                 dnsBlocklistAction;

    //
    // DNS Sinkhole
    //
    bool                                        dnsProcessing;
    std::vector<std::string>                    dnsItemList;

    //
    // Transport buffer for IOCTL
    //
    ATF_CONFIG_HDR                              rawTransportDataHdr;
    std::shared_ptr<std::vector<std::byte>>     rawTransportBuffer;

    //
    // Ini filepath
    //
    const std::string                           iniFilePath;

    //
    // Reader class instance is locked until this object is released
    //
    INIReader                                   iniReader;

    //
    // Online IP blacklist
    //
    std::vector<IpBlacklistItem>                onlineIpBlacklists;

public:
    FilterConfig(std::string &iniFilePath) :
        enableLayerIpv4TcpInbound(false),
        enableLayerIpv4TcpOutbound(false),
        enableLayerIpv6TcpInbound(false),
        enableLayerIpv6TcpOutbound(false),
        enableLayerIcmpv4(false),

        dnsProcessing(false),

        alertInbound(false),
        alertOutbound(false),

        ipv4BlocklistAction(ACTION_PASS),
        ipv6BlocklistAction(ACTION_PASS),
        dnsBlocklistAction(ACTION_PASS),

        iniFilePath(iniFilePath),
        rawTransportDataHdr({ 0 }),
        iniReader(iniFilePath)
    {
    
    }

    ~FilterConfig(void)
    {
    
    }

    //
    // Parse the INI file and build structures
    //
    ATF_ERROR ParseIniFile(void);

    //
    // Returns the raw configuration data buffer
    //
    const std::shared_ptr<std::vector<std::byte>> GetSerializeConfigBuffer(void) const;

    //
    // Return the raw filter config, which will be transported to the driver
    //
    const ATF_CONFIG_HDR &GetRawFilterData(void) const;

    //
    // Returns the vector containing IPs retrieved from online blacklists
    //
    const std::vector<struct in_addr> &GetIpv4BlacklistOnline(void) const;

    //
    // Returns whether or not the ATF_CONFIG_HDR structure is initialized
    //
    bool IsIniDataInitialized(void) const;

    // Get the ini filepath
    const std::string &GetIniFilepath(void) const;

    //
    // Get number of IPs in ipv4 online blacklist
    //
    size_t GetNumOfIpv4BlacklistIps(void) const;

private:
    //
    // Parse the ipv4_blacklist_urls_simple object and download all IPs
    //
    ATF_ERROR parseOnlineIpBlacklists(void);

    //
    // Parse DNS sink hole config
    //
    ATF_ERROR parseDnsSinkholeConfig(void);

    //
    // Returns a vector for all keys in a given section
    //
    static inline const std::string sectionNameIpOnlineBlacklist = "ipv4_blacklist_urls_simple";
    ATF_ERROR getIniValuesBySection(   
        const std::string &sectionName, 
        std::vector<std::string> &keyList) const;

    //
    // Parse the internal config into a the usermode to driver transport struct
    //
    void genIoctlStruct(void);

    //
    // Parser for the action type
    //
    void parseActionType(std::string typeStr, ACTION_OPTS &opt);
};
