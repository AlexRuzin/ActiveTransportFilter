#pragma once

#include <Windows.h>

#include <INIReader.h>

#include "../common/errors.h"
#include "../common/user_driver_transport.h"

#include <string>
#include <vector>

class FilterConfig {
private:
    bool                                        enableLayerIpv4TcpInbound;
    bool                                        enableLayerIpv4TcpOutbound;
    bool                                        enableLayerIpv6TcpInbound;
    bool                                        enableLayerIpv6TcpOutbound;

    std::vector<IPV4_RAW_ADDRESS>               blocklistIpv4;
    std::vector<IPV6_RAW_ADDRESS>               blocklistIpv6;


    USER_DRIVER_FILTER_TRANSPORT_DATA           rawTransportData;

    const std::string                           iniFilePath;

    INIReader                                   iniReader;

public:
    FilterConfig(std::string &iniFilePath) :
        enableLayerIpv4TcpInbound(false),
        enableLayerIpv4TcpOutbound(false),
        enableLayerIpv6TcpInbound(false),
        enableLayerIpv6TcpOutbound(false),

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
    // Return the raw filter config, which will be transported to the driver
    //
    const USER_DRIVER_FILTER_TRANSPORT_DATA &GetRawFilterData(void) const;
};
