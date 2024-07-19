#include <Windows.h>

#include <INIReader.h>

#define CURL_STATICLIB 
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
#include <sstream>
#include <numeric>

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
        LOG_ERROR("downloadBlocklist() failed for blocklist: %s , error: 0x%08x", blacklistName.c_str(), atfError);
        return atfError;
    }

    LOG_DEBUG("downloadBlocklist() downloaded blocklist: %s size: %d bytes", blacklistName.c_str(), rawDownloadBuffer.size());

    atfError = parseBufIntoList(rawDownloadBuffer, blacklist);
    if (atfError) {
        LOG_ERROR("parseBufIntoList() failed for blocklist: %s, error: 0x%08x", blacklistName.c_str(), atfError);
        return atfError;
    }

    LOG_DEBUG("parseBufIntoList() parsed blocklist: %s numOfIps: %d", blacklistName.c_str(), blacklist.size());

    return atfError;
}

ATF_ERROR IpBlacklistItem::parseBufIntoList(
    const std::vector<char> &buf, 
    std::vector<struct in_addr> &ipOut
)
{
    ATF_ERROR atfError = ATF_ERROR_OK;

    const std::vector<std::string> ipList = shared::SplitStringByLine(rawDownloadBuffer);
    if (!ipList.size()) {
        return ATF_CURL_BAD_DATA;
    }

    for (std::vector<std::string>::const_iterator i = ipList.begin(); i != ipList.end(); i++) {
        uint32_t ip;
        if (shared::ParseStringToIpv4(*i, ip)) {
            struct in_addr ipAddr;
            ipAddr.S_un.S_addr = ip;
            blacklist.push_back(ipAddr);
        }
    }
    
    return atfError;
}

const std::vector<struct in_addr> IpBlacklistItem::GetIps(void) const
{
    return blacklist;
}

const std::string IpBlacklistItem::GetName(void) const
{
    return blacklistName;
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

    static const std::string userAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36";
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());


    CURLcode curlRes = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (curlRes != CURLE_OK) {
        LOG_ERROR("curl failed: 0x%08x", curlRes);
        atfError = ATF_CURL_DOWNLOAD;
    }

    LOG_DEBUG("CURL returned buffer size: %d bytes", rawDownloadBuffer.size());

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

    // Parse all layer switches
    enableLayerIpv4TcpInbound = iniReader.GetBoolean("wfp_layer", "enable_layer_inbound_tcp_v4", false);
    enableLayerIpv4TcpOutbound = iniReader.GetBoolean("wfp_layer", "enable_layer_inbound_tcp_v6", false);
    enableLayerIpv6TcpInbound = iniReader.GetBoolean("wfp_layer", "enable_layer_outbound_tcp_v4", false);
    enableLayerIpv6TcpOutbound = iniReader.GetBoolean("wfp_layer", "enable_layer_outbound_tcp_v6", false);
    enableLayerIpv4Datagram = iniReader.GetBoolean("wfp_layer", "enable_layer_datagram_v4", false);
    enableLayerIcmpv4 = iniReader.GetBoolean("wfp_layer", "enableLayerIcmpv4", false);

    // Parse action switches
    parseActionType("ipv4_blocklist_action", ipv4BlocklistAction);
    parseActionType("ipv6_blocklist_action", ipv6BlocklistAction);
    parseActionType("dns_blocklist_action", dnsBlocklistAction);

    // Parse direction switches
    alertInbound = iniReader.GetBoolean("alert_config", "alert_inbound", false);
    alertOutbound = iniReader.GetBoolean("alert_config", "alert_outbound", false);

    // Parse hardcoded blacklist strings
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

    //
    // Parse DNS blacklist
    //
    atfError = parseDnsSinkholeConfig();
    if (atfError) {
        LOG_ERROR("Failed to parse DNS blacklist: 0x%08x", atfError);
    }

    genIoctlStruct();

    lastIniSum = shared::Crc32SumFile(iniFilePath);
    LOG_INFO("Ini CRC32 sum: 0x%08x", lastIniSum);

    return ATF_ERROR_OK;
}

void FilterConfig::parseActionType(std::string typeStr, ACTION_OPTS &opt)
{
    static const std::string noneAction = "PASS";
    static const std::string alertAction = "ALERT";
    static const std::string blockAction = "BLOCK";

    static const std::map<std::string, ACTION_OPTS> actionVals = {
        {
            noneAction, ACTION_PASS
        },

        {  
            alertAction, ACTION_ALERT
        },

        {
            blockAction, ACTION_BLOCK
        }
    };

    const std::string actionString = iniReader.GetString("alert_config", typeStr, noneAction);
    if (actionVals.find(actionString) == actionVals.end()) {
        opt = ACTION_PASS;
        return;
    }

    opt = actionVals.at(actionString);
}

const ATF_CONFIG_HDR &FilterConfig::GetRawFilterData(void) const
{
    return rawTransportDataHdr;
}

const std::vector<struct in_addr> &FilterConfig::GetIpv4BlacklistOnline(void) const
{
    return blocklistIpv4Online;
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

        const std::vector<struct in_addr> &ipList = currBlacklist->GetIps();
        if (ipList.size()) {
            LOG_DEBUG("Downloaded blacklist IPs (ipv4) from %s (numOfIps: %d)", currBlacklist->GetName().c_str(), ipList.size());
            blocklistIpv4Online.insert(blocklistIpv4Online.end(), ipList.begin(), ipList.end());
        }
    }

    if (anyBlacklistAvail) {
        return ATF_ERROR_OK;
    }

    return ATF_NO_BLACKLISTS_AVAIL;
}

ATF_ERROR FilterConfig::parseDnsSinkholeConfig(void)
{    
    static const std::string invalidVal = "<invalid>";
    static const std::string dnsHeader = "dns_null_route";

    if (!iniReader.GetBoolean(dnsHeader, "null_route_enable", false)) {
        return ATF_ERROR_OK;
    }

    const std::string dnsRaw = iniReader.GetString(dnsHeader, "null_route_manual", invalidVal);
    if (dnsRaw == invalidVal) {
        return ATF_BAD_INI_CONFIG;
    }

    static const char commaDelim = ',';
    dnsItemList = shared::SplitStringByDelimiter(dnsRaw, commaDelim);
    if (dnsItemList.size() == 0) {
        return ATF_BAD_INI_CONFIG;
    }

    dnsProcessing = true;
    return ATF_ERROR_OK;
}

void FilterConfig::genIoctlStruct(void)
{
    ZeroMemory(&rawTransportDataHdr, sizeof(ATF_CONFIG_HDR));

    rawTransportDataHdr.magic = FILTER_TRANSPORT_MAGIC;
    rawTransportDataHdr.structHeaderSize = sizeof(ATF_CONFIG_HDR);

    rawTransportDataHdr.enableLayerIpv4TcpInbound = enableLayerIpv4TcpInbound;
    rawTransportDataHdr.enableLayerIpv4TcpOutbound = enableLayerIpv4TcpOutbound;
    rawTransportDataHdr.enableLayerIpv6TcpInbound = enableLayerIpv6TcpInbound;
    rawTransportDataHdr.enableLayerIpv6TcpOutbound = enableLayerIpv6TcpOutbound;
    rawTransportDataHdr.enableLayerIpv4Datagram = enableLayerIpv4Datagram;

    rawTransportDataHdr.enableLayerIcmpv4 = enableLayerIcmpv4;

    rawTransportDataHdr.dnsBlocklistAction = dnsBlocklistAction;
    rawTransportDataHdr.ipv4BlocklistAction = ipv4BlocklistAction;
    rawTransportDataHdr.ipv6BlocklistAction = ipv6BlocklistAction;

    rawTransportDataHdr.alertInbound = alertInbound;
    rawTransportDataHdr.alertOutbound = alertOutbound;

    rawTransportDataHdr.numOfIpv4Addresses = (UINT16)blocklistIpv4.size();
    for (std::vector<struct in_addr>::const_iterator i = blocklistIpv4.begin(); i != blocklistIpv4.end(); i++) {
        rawTransportDataHdr.ipv4BlackList[i - blocklistIpv4.begin()] = *i;
    }

    //
    // DNS
    //
    rawTransportDataHdr.enableDnsBlackhole = dnsProcessing;
    rawTransportDataHdr.dnsBufferSize = 0;

    for (const auto &t : dnsItemList) rawTransportDataHdr.dnsBufferSize += t.size() + sizeof('\0');
    
    // Create the raw buffer, consists of the header and buffer
    rawTransportBuffer = std::make_shared<std::vector<std::byte>>(sizeof(ATF_CONFIG_HDR) + rawTransportDataHdr.dnsBufferSize);
    std::fill(rawTransportBuffer->begin(), rawTransportBuffer->end(), std::byte{ 0 }); //zero

    // Copy over header
    std::memcpy(rawTransportBuffer->data(), (const void *)&rawTransportDataHdr, sizeof(ATF_CONFIG_HDR));

    // Copy over the DNS strings, delimited by \0
    uint16_t offset = sizeof(ATF_CONFIG_HDR);
    for (const auto &s : dnsItemList) {
        std::memcpy(rawTransportBuffer->data() + offset, s.c_str(), s.size() + sizeof('\0'));
        offset += s.size() + sizeof('\0');
    }
}

const std::shared_ptr<std::vector<std::byte>> FilterConfig::GetSerializeConfigBuffer(void) const
{
    return rawTransportBuffer;
}

bool FilterConfig::IsIniDataInitialized(void) const
{
    return (rawTransportDataHdr.magic == FILTER_TRANSPORT_MAGIC && 
        rawTransportDataHdr.structHeaderSize == sizeof(ATF_CONFIG_HDR));
}

const std::string &FilterConfig::GetIniFilepath(void) const
{
    return iniFilePath;
}

size_t FilterConfig::GetNumOfIpv4BlacklistIps(void) const
{
    return onlineIpBlacklists.size();
}
