#include <Windows.h>

#pragma comment(lib, "Ws2_32.lib")

#include <INIReader.h>
#include <curl/curl.h>

#include "ini_reader.h"

#include "../common/errors.h"
#include "../common/user_driver_transport.h"
#include "../common/shared.h"
#include "../common/user_logging.h"

#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <cstdint>

//
// Global init for CURL
//  TODO: Global cleanup for CURL (on shutdown)
//
static bool globalCurlInit = false;

//
// IpBlacklistItem class methods
//

//
// Dispatch to downloadBlocklist() and parseBlocklist()
//
ATF_ERROR IpBlacklistItem::DownloadAndParseBlacklist(void)
{
    ATF_ERROR atfError = ATF_ERROR_OK;

    atfError = downloadBlocklist(uri, rawDownloadBuffer);
    if (atfError) {
        LOG_ERROR("downloadBlocklist() failed for blocklist: %s (%s), error: 0x%08x", blacklistName, uri, atfError);
        return atfError;
    }

    LOG_DEBUG("downloadBlocklist() downloaded blocklist: %s (%s) size: %d", blacklistName, uri, rawDownloadBuffer.size());

    atfError = parseBufIntoList(rawDownloadBuffer, blacklist);
    if (atfError) {
        LOG_ERROR("parseBufIntoList() failed for blocklist: %s (%s), error: 0x%08x", blacklistName, uri, atfError);
        return atfError;
    }

    return atfError;
}

ATF_ERROR IpBlacklistItem::parseBufIntoList(
    const std::vector<char> &buf, 
    std::vector<struct in_addr> &ipOut
)
{
    ATF_ERROR atfError = ATF_ERROR_OK;
    
    return atfError;
}


ATF_ERROR IpBlacklistItem::downloadBlocklist(
    const std::string &uri, 
    std::vector<char> &rawBufOut) const
{
    ATF_ERROR atfError = ATF_ERROR_OK;

    if (!globalCurlInit) {
        curl_global_init(CURL_GLOBAL_ALL);
        globalCurlInit = true;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        return ATF_CURL_INIT;
    }

    curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rawBufOut);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects

    CURLcode curlRes = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (curlRes != CURLE_OK) {
        LOG_ERROR("curl failed: 0x%08x", curlRes);
        atfError = ATF_CURL_DOWNLOAD;
    }

    return atfError;
}

//
// CURL callback function
//
size_t IpBlacklistItem::curlWriteCallback(
    const char *buf,
    size_t bufSize,
    size_t nmemb,
    std::vector<char> *out
)
{
    const size_t totalBytes = bufSize * nmemb;
   
    for (size_t i = 0; i < totalBytes; i++) {
        
        out->push_back(buf[i]);
    }    

    return totalBytes;
}

//
// FilterConfig class
//

//
// Parse the ini file
//
ATF_ERROR FilterConfig::ParseIniFile(void)
{
    static const std::string unknownVal = "UNKNOWN";
    static const char standardDelimiter = ',';

    // Parse all values
    enableLayerIpv4TcpInbound = iniReader.GetBoolean("wfp_layer", "enable_layer_inbound_tcp_v4", false);
    enableLayerIpv4TcpOutbound = iniReader.GetBoolean("wfp_layer", "enable_layer_inbound_tcp_v6", false);
    enableLayerIpv6TcpInbound = iniReader.GetBoolean("wfp_layer", "enable_layer_outbound_tcp_v4", false);
    enableLayerIpv6TcpOutbound = iniReader.GetBoolean("wfp_layer", "enable_layer_outbound_tcp_v6", false);
    enableLayerIcmpv4 = iniReader.GetBoolean("wfp_layer", "enableLayerIcmpv4", false);

    const std::string ipv4Blacklist = iniReader.Get("blacklist_ipv4", "ipv4_list", unknownVal);
    const std::string ipv6Blacklist = iniReader.Get("blacklist_ipv6", "ipv6_list", unknownVal);

    if (ipv4Blacklist != unknownVal) {
        const std::vector<std::string> out = shared::SplitStringByDelimiter(ipv4Blacklist, standardDelimiter);

        if (out.size() > MAX_IPV4_ADDRESSES_BLACKLIST) {
            return ATF_DEFAULT_CONFIG_TOO_LARGE;
        }

        for (std::vector<std::string>::const_iterator i = out.begin(); i != out.end(); i++) {
            uint32_t out = 0;

            if (!shared::ParseStringToIpv4(*i, out)) {
                continue;
            }

            struct in_addr ipAddr;
            ipAddr.S_un.S_addr = out;

            blocklistIpv4.push_back(ipAddr);
        }
    }

    //
    // Parse the online IP blacklists
    //
    ATF_ERROR atfError = parseOnlineIpBlacklists();
    if (atfError) {
        LOG_ERROR("Failed to parse online IP blacklist: 0x%08x", atfError);
    }

    genIoctlStruct();

    lastIniSum = shared::Crc32SumFile(iniFilePath);
    LOG_INFO("Ini CRC32 sum: 0x%08x", lastIniSum);

    return ATF_ERROR_OK;
}

const USER_DRIVER_FILTER_TRANSPORT_DATA &FilterConfig::GetRawFilterData(void) const
{
    return rawTransportData;
}

ATF_ERROR FilterConfig::getIniValuesBySection(
    const std::string &sectionName, 
    std::vector<std::string> &keyList) const
{
    if (!iniReader.HasSection(sectionName)) {
        return ATF_BAD_INI_CONFIG;
    }

    return ATF_ERROR_OK;
}

ATF_ERROR FilterConfig::parseOnlineIpBlacklists(void)
{
    static const std::string unknownVal = "UNKNOWN";
    static const char standardDelimiter = ',';

    ATF_ERROR atfError = ATF_ERROR_OK;

    const std::string ipUriUnparsed = iniReader.GetString("ipv4_blacklist_urls_simple", "online_ip_blocklists", unknownVal);
    if (ipUriUnparsed == unknownVal) {
        // Can be optionally removed
        return ATF_ERROR_OK;
    }

    const std::vector<std::string> splitUris = shared::SplitStringByDelimiter(ipUriUnparsed, standardDelimiter);
    if (!splitUris.size()) {
        return ATF_ERROR_OK;    
    }

    std::map<std::string, std::string> nameUriMap;
    for (std::vector<std::string>::const_iterator i = splitUris.begin(); i != splitUris.end(); i++) {
        nameUriMap[shared::IsolateDomainFromUri(*i)] = *i;
    }

    // Initialize the blacklist objects
    for (std::map<std::string, std::string>::const_iterator i = nameUriMap.begin(); i != nameUriMap.end(); i++) {
        if (i->first != "") {
            onlineIpBlacklists.push_back({i->first, i->second});
        }
    }

    // Download and parse each IP blocklist
    bool anyBlacklistAvail = false;
    for (std::vector<IpBlacklistItem>::iterator currBlacklist = onlineIpBlacklists.begin(); 
        currBlacklist != onlineIpBlacklists.end(); currBlacklist++)
    {
        atfError = currBlacklist->DownloadAndParseBlacklist();
        if (!atfError) {            
            anyBlacklistAvail = true;
        }
    }

    if (anyBlacklistAvail) {
        return ATF_ERROR_OK;
    }

    return ATF_NO_BLACKLISTS_AVAIL;
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
    rawTransportData.enableLayerIcmpv4 = enableLayerIcmpv4;

    rawTransportData.numOfIpv4Addresses = (UINT16)blocklistIpv4.size();
    for (std::vector<struct in_addr>::const_iterator i = blocklistIpv4.begin(); i != blocklistIpv4.end(); i++) {
        rawTransportData.ipv4BlackList[i - blocklistIpv4.begin()] = *i;
    }
}

std::vector<std::byte> FilterConfig::SerializeConfigBuffer(void) const
{
    std::vector<std::byte> out(sizeof(USER_DRIVER_FILTER_TRANSPORT_DATA));
    std::memcpy(out.data(), &this->rawTransportData, sizeof(USER_DRIVER_FILTER_TRANSPORT_DATA));
    return out;
}

bool FilterConfig::IsIniDataInitialized(void) const
{
    return (rawTransportData.magic == FILTER_TRANSPORT_MAGIC && 
        rawTransportData.size == sizeof(USER_DRIVER_FILTER_TRANSPORT_DATA));
}

