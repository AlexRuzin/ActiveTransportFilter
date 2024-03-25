#pragma once

#include <Windows.h>

#include <INIReader.h>

#include "../common/errors.h"
#include "../common/shared.h"
#include "../common/user_driver_transport.h"

#include <string>
#include <vector>
#include <cstring>
#include <cstdint>

//
// Struct representing an IP blacklist from online
//  Supplied by ipv4_blacklist_urls_simple in the INI file
//
class IpBlacklistItem {
private:
    const std::string                   blacklistName;
    const std::string                   uri;

public:
    IpBlacklistItem(const std::string &blacklistName, const std::string &uri) :
        uri(uri),
        blacklistName(blacklistName)
    {
    
    }

    ~IpBlacklistItem(void)
    {
    
    }
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
    bool                                        enableLayerIcmpv4;

    std::vector<struct in_addr>                 blocklistIpv4;
    std::vector<IPV6_RAW_ADDRESS>               blocklistIpv6;


    USER_DRIVER_FILTER_TRANSPORT_DATA           rawTransportData;

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

        iniFilePath(iniFilePath),
        rawTransportData({ 0 }),
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
    std::vector<std::byte> SerializeConfigBuffer(void) const;

    //
    // Return the raw filter config, which will be transported to the driver
    //
    const USER_DRIVER_FILTER_TRANSPORT_DATA &GetRawFilterData(void) const;

    //
    // Returns whether or not the USER_DRIVER_FILTER_TRANSPORT_DATA structure is initialized
    //
    bool IsIniDataInitialized(void) const;

    //
    // Flushes the current ini file
    //
    void FlushIniFile(void);

private:
    //
    // Parse the ipv4_blacklist_urls_simple object and download all IPs
    //
    ATF_ERROR parseOnlineIpBlacklists(void);

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
};
