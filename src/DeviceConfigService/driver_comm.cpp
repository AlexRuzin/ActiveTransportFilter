#include <Windows.h>

#include "driver_comm.h"

#include "../common/shared.h"
#include "../common/common.h"
#include "../common/errors.h"
#include "../common/user_logging.h"

#include <string>

ATF_ERROR IoctlComm::ConnectToDriver(void)
{
    if (!shared::IsFileExists(driverLogicalDevicePath)) {
        return ATF_ERROR_FILE_NOT_FOUND;
    }

    driverHandle = CreateFileA(
        ATF_DEVICE_NAME,
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