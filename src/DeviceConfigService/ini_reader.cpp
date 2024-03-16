#include <Windows.h>

#include <INIReader.h>

#include "ini_reader.h"

#include "../common/errors.h"
#include "../common/user_driver_transport.h"
#include "../common/shared.h"


ATF_ERROR FilterConfig::ParseIniFile(void)
{
    static const std::string unknownVal = "UNKNOWN";
    static const char standardDelimiter = ',';


    // Parse all values
    enableLayerIpv4TcpInbound = iniReader.GetBoolean("wfp_layer", "enable_layer_inbound_tcp_v4", false);
    enableLayerIpv4TcpOutbound = iniReader.GetBoolean("wfp_layer", "enable_layer_inbound_tcp_v6", false);
    enableLayerIpv6TcpInbound = iniReader.GetBoolean("wfp_layer", "enable_layer_outbound_tcp_v4", false);
    enableLayerIpv6TcpOutbound = iniReader.GetBoolean("wfp_layer", "enable_layer_outbound_tcp_v6", false);

    const std::string ipv4Blacklist = iniReader.Get("blacklist_ipv4", "ipv6_list", unknownVal);
    const std::string ipv6Blacklist = iniReader.Get("blacklist_ipv6", "ipv6_list", unknownVal);

    if (ipv4Blacklist != unknownVal) {
        const std::vector<std::string> out = shared::SplitStringByDelimiter(ipv4Blacklist, standardDelimiter);

        for (std::vector<std::string>::const_iterator i = out.begin(); i != out.end(); i++) {
            uint32_t out = 0;
            if (shared::ConvertStringToInt(*i, out)) {
                blocklistIpv4.push_back((IPV4_RAW_ADDRESS)out);
            }
        }
    }

    return ATF_ERROR_OK;
}

const USER_DRIVER_FILTER_TRANSPORT_DATA &FilterConfig::GetRawFilterData(void) const
{
    return rawTransportData;
}