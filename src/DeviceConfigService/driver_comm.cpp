#include <Windows.h>

#include "driver_comm.h"

#include "../common/common.h"
#include "../common/errors.h"
#include "../common/ioctl_codes.h"
#include "../common/user_logging.h"

#include <string>

ATF_ERROR IoctlComm::ConnectToDriver(void)
{
    ATF_ERROR atfError = IoctlComm::tryOpenDevicePath(driverLogicalDevicePath);
    if (atfError) {
        return atfError;
    }

    driverHandle = CreateFileA(
        driverLogicalDevicePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (driverHandle == INVALID_HANDLE_VALUE) {
        LOG_ERROR("Failed to open device: %s (err: 0x%08x)", driverLogicalDevicePath.c_str(), GetLastError());
        return ATF_ERROR_OPEN_FILE;
    }


    return ATF_ERROR_OK;
}

const std::string &IoctlComm::GetLogicalDeviceFileName(void) const
{
    return driverLogicalDevicePath;
}

ATF_ERROR IoctlComm::SendRawBufferIoctl(IOCTL_CODE ioctl, const void *ptr, size_t size) const
{
    if (driverHandle == INVALID_HANDLE_VALUE) {
        return ATF_FAILED_HANDLE_NOT_OPENED;
    }

    if (!size) {
        return ATF_BAD_PARAMETERS;
    }

    DWORD bytesReturned = 0;

    if (!DeviceIoControl(
        driverHandle,
        ioctl,
        const_cast<void *>(ptr),
        (DWORD)size,
        NULL,
        0,
        &bytesReturned,
        NULL
    )) 
    {
        return ATF_DEVICEIOCONTROL;
    }

    return ATF_ERROR_OK;
}

ATF_ERROR IoctlComm::SendRawBufferIoctl(IOCTL_CODE ioctl, const std::vector<std::byte> &rawBuffer) const
{
    if (driverHandle == INVALID_HANDLE_VALUE) {
        return ATF_FAILED_HANDLE_NOT_OPENED;
    }

    if (rawBuffer.size() == 0) {
        return ATF_BAD_PARAMETERS;
    }    

    return SendRawBufferIoctl(ioctl, rawBuffer.data(), rawBuffer.size());
}

ATF_ERROR IoctlComm::SendIoctlNoData(IOCTL_CODE ioctl) const
{
    if (driverHandle == INVALID_HANDLE_VALUE) {
        return ATF_FAILED_HANDLE_NOT_OPENED;
    }

    DWORD bytesReturned = 0;

    if (!DeviceIoControl(
        driverHandle,
        ioctl,
        NULL,
        0,
        NULL,
        0,
        &bytesReturned,
        NULL
    )) 
    {
        return ATF_DEVICEIOCONTROL;
    }

    return ATF_ERROR_OK;
}

ATF_ERROR IoctlComm::tryOpenDevicePath(const std::string &in)
{
    HANDLE deviceHandle = CreateFileA(
        in.c_str(),
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (deviceHandle == INVALID_HANDLE_VALUE) {
        return ATF_ERROR_FILE_NOT_FOUND;
    }

    CloseHandle(deviceHandle);
    return ATF_ERROR_OK;
}

bool IoctlComm::GetIsConnected(void) const
{
    return !(driverHandle == INVALID_HANDLE_VALUE);
}