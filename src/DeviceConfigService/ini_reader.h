#pragma once

#include <Windows.h>

#include <INIReader.h>

#include "../common/errors.h"
#include "../common/user_driver_transport.h"

#include <string>
#include <vector>

class FilterConfig {
private:
    typedef struct _filter_config {
        std::vector<IPV4_RAW_ADDRESS>           blocklistIpv4;
        std::vector<IPV6_RAW_ADDRESS>           blocklistIpv6;
    } FILTER_CONFIG, *PFILTER_CONFIG;

    FILTER_CONFIG                               iniConfigParsed;

    USER_DRIVER_FILTER_TRANSPORT_DATA           rawTransportData;

    const std::string                           iniFilePath;

public:
    FilterConfig(std::string &iniFilePath) :
        iniFilePath(iniFilePath),
        rawTransportData({ 0 })
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
