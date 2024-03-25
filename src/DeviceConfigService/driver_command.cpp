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

#include <inaddr.h>

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

    if (!isDeviceReady()) {
        return ATF_DEVICE_NOT_CONNECTED;
    }

    if (isWfpReady()) {
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

    if (!isDeviceReady()) {
        return ATF_DEVICE_NOT_CONNECTED;
    }

    if (!isWfpReady()) {
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

    if (!isDeviceReady()) {
        return ATF_DEVICE_NOT_CONNECTED;
    }

    if (isWfpReady()) {
        return ATF_WFP_ALREADY_RUNNING;
    }

    return ioctlComm->SendIoctlNoData(IOCTL_ATF_FLUSH_CONFIG);
}

ATF_ERROR DriverCommand::CmdSendIniConfiguration(const FilterConfig &filterConfig) const
{
    ATF_ERROR atfError = ATF_ERROR_OK;

    if (!isDeviceReady()) {
        return ATF_DEVICE_NOT_CONNECTED;
    }

    // WFP engine cannot be running while sending commands to filter.c
    if (isWfpReady()) {
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

ATF_ERROR DriverCommand::CmdAppendIpv4Blacklist(const FilterConfig &filterConfig) const
{
    if (!isDeviceReady()) {
        return ATF_DEVICE_NOT_CONNECTED;
    }

    // WFP engine cannot be running while sending commands to filter.c
    if (isWfpReady()) {
        return ATF_WFP_ALREADY_RUNNING;
    }

    const std::vector<struct in_addr> &list = filterConfig.GetIpv4BlacklistOnline();

    if (!list.size()) {
        return ATF_NO_DATA_AVAILABLE;
    }

    // Serialize the IP list into a raw buffer
    const size_t totalSize = list.size() * sizeof(struct in_addr);
    std::vector<std::byte> rawBuf(totalSize);
    CopyMemory(rawBuf.data(), list.data(), list.size() * sizeof(struct in_addr));

    return ioctlComm->SendRawBufferIoctl(IOCTL_ATF_APPEND_IPV4_BLACKLIST, rawBuf);
}

bool DriverCommand::isDeviceReady(void) const
{
#if defined(OVERRIDE_CONN_REQ) // Debug and testing only!
    return true;
#endif

    if (!ioctlComm || !ioctlComm->GetIsConnected()) {
        return false;
    }

    return true;
}

bool DriverCommand::isWfpReady(void) const
{
#if defined(OVERRIDE_CONN_REQ) // Debug and testing only!
    return false;
#endif

    return wfpRunning;
}

//EOF