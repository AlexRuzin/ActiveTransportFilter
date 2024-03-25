#include <Windows.h>

#include "../common/ioctl_codes.h"
#include "../common/user_logging.h"
#include "../common/errors.h"
#include "driver_command.h"

#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <memory>

ATF_ERROR DriverCommand::InitializeDriverComms(void)
{
    ATF_ERROR atfError = ATF_ERROR_OK;

    // We need to append "\\.\" to the device path for CreateFileA() to work
    const std::string fullDeviceName = "\\\\.\\" + deviceFilepath;

    // Check if driver device exists
    atfError = IoctlComm::tryOpenDevicePath(fullDeviceName);
    if (atfError) {
        return atfError;
    }

    ioctlComm = std::make_unique<IoctlComm>(fullDeviceName);
    atfError = ioctlComm->ConnectToDriver();
    if (atfError) {
        return atfError;
    }

    return atfError;
}

ATF_ERROR DriverCommand::CmdStartWfp(void)
{
    ATF_ERROR atfError = ATF_ERROR_OK;

    if (wfpRunning) {
        return ATF_WFP_ALREADY_RUNNING;
    }

    atfError = ioctlComm->SendIoctlNoData(IOCTL_ATF_WFP_SERVICE_START);
    if (atfError) {
        return atfError;
    }

    wfpRunning = true;

    return atfError;
}

ATF_ERROR DriverCommand::CmdStopWfp(void)
{
    ATF_ERROR atfError = ATF_ERROR_OK;

    if (!wfpRunning) {
        return ATF_WFP_NOT_RUNNING;
    }

    atfError = ioctlComm->SendIoctlNoData(IOCTL_ATF_WFP_SERVICE_STOP);
    if (atfError) {
        return atfError;
    }

    wfpRunning = false;

    return atfError;
}

ATF_ERROR DriverCommand::CmdFlushConfig(void) const
{
    ATF_ERROR atfError = ATF_ERROR_OK;

    if (wfpRunning) {
        return ATF_WFP_ALREADY_RUNNING;
    }

    return ioctlComm->SendIoctlNoData(IOCTL_ATF_FLUSH_CONFIG);
}

ATF_ERROR DriverCommand::CmdSendIniConfiguration(const FilterConfig &filterConfig) const
{
    ATF_ERROR atfError = ATF_ERROR_OK;

    // WFP engine cannot be running while sending commands to filter.c
    if (wfpRunning) {
        return ATF_WFP_ALREADY_RUNNING;
    }

    if (!filterConfig.IsIniDataInitialized()) {
        return ATF_NO_INI_CONFIG;
    }

    std::vector<std::byte> configSerialized = filterConfig.SerializeConfigBuffer();
    if (configSerialized.size() == 0) {
        return ATF_NO_INI_CONFIG;
    }

    return ioctlComm->SendRawBufferIoctl(IOCTL_ATF_SEND_WFP_CONFIG, configSerialized);
}

ATF_ERROR DriverCommand::CmdAppendIpv4Blacklist(const std::vector<struct in_addr> &blacklist) const
{
    if (!blacklist.size()) {
        return ATF_NO_DATA_AVAILABLE;
    }

    // WFP engine cannot be running while sending commands to filter.c
    if (wfpRunning) {
        return ATF_WFP_ALREADY_RUNNING;
    }

    const size_t totalSize = blacklist.size() * sizeof(struct in_addr);
    std::vector<std::byte> rawBuf(totalSize);
    CopyMemory(rawBuf.data(), blacklist.data(), blacklist.size() * sizeof(struct in_addr));

    return ioctlComm->SendRawBufferIoctl(IOCTL_ATF_APPEND_IPV4_BLACKLIST, rawBuf);
}
