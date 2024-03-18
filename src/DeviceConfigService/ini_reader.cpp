#include <Windows.h>

#include <INIReader.h>

#include "ini_reader.h"

#include "../common/errors.h"
#include "../common/user_driver_transport.h"
#include "../common/shared.h"

#include <string>
#include <vector>
#include <cstring>
#include <cstdint>



ATF_ERROR FilterConfig::ParseIniFile(void)
{
    static const std::string unknownVal = "UNKNOWN";
    static const char standardDelimiter = ',';


    // Parse all values
    enableLayerIpv4TcpInbound = iniReader.GetBoolean("wfp_layer", "enable_layer_inbound_tcp_v4", false);
    enableLayerIpv4TcpOutbound = iniReader.GetBoolean("wfp_layer", "enable_layer_inbound_tcp_v6", false);
    enableLayerIpv6TcpInbound = iniReader.GetBoolean("wfp_layer", "enable_layer_outbound_tcp_v4", false);
    enableLayerIpv6TcpOutbound = iniReader.GetBoolean("wfp_layer", "enable_layer_outbound_tcp_v6", false);

    const std::string ipv4Blacklist = iniReader.Get("blacklist_ipv4", "ipv4_list", unknownVal);
    const std::string ipv6Blacklist = iniReader.Get("blacklist_ipv6", "ipv6_list", unknownVal);

    if (ipv4Blacklist != unknownVal) {
        const std::vector<std::string> out = shared::SplitStringByDelimiter(ipv4Blacklist, standardDelimiter);

        for (std::vector<std::string>::const_iterator i = out.begin(); i != out.end(); i++) {
            uint32_t out = 0;
            if (shared::ParseStringToIpv4(*i, out)) {
                blocklistIpv4.push_back((IPV4_RAW_ADDRESS)out);
            }
        }
    }

    genIoctlStruct();

    return ATF_ERROR_OK;
}

const USER_DRIVER_FILTER_TRANSPORT_DATA &FilterConfig::GetRawFilterData(void) const
{
    return rawTransportData;
}

void FilterConfig::genIoctlStruct(void)
{
    ZeroMemory(&rawTransportData, sizeof(USER_DRIVER_FILTER_TRANSPORT_DATA));

    rawTransportData.magic = FILTER_TRANSPORT_MAGIC;
    rawTransportData.size = sizeof(struct _user_driver_filter_transport_data);

    rawTransportData.enableLayerIpv4TcpInbound = enableLayerIpv4TcpInbound;
    rawTransportData.enableLayerIpv4TcpOutbound = enableLayerIpv4TcpOutbound;
    rawTransportData.enableLayerIpv6TcpInbound = enableLayerIpv6TcpInbound;
    rawTransportData.enableLayerIpv6TcpOutbound = enableLayerIpv6TcpOutbound;

    for (std::vector<IPV4_RAW_ADDRESS>::const_iterator i = blocklistIpv4.begin(); i != blocklistIpv4.end(); i++) {
        rawTransportData.ipv4BlackList[i - blocklistIpv4.begin()] = *i;
    }
}

std::vector<std::byte> FilterConfig::SerializeConfigBuffer(void) const
{
    std::vector<std::byte> out(sizeof(USER_DRIVER_FILTER_TRANSPORT_DATA));
    std::memcpy(out.data(), &this->rawTransportData, sizeof(USER_DRIVER_FILTER_TRANSPORT_DATA));
    return out;
}