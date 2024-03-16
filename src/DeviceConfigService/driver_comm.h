#pragma once

#include <Windows.h>

#include <string>

#include "../common/common.h"
#include "../common/errors.h"

#include <string>

class IoctlComm {
private:
    const std::string                       driverLogicalDevicePath;
    
    // Device handle to driver
    HANDLE                                  driverHandle;

public:
    IoctlComm(std::string driverLogicalPath) :
        driverLogicalDevicePath(driverLogicalPath),
        driverHandle(INVALID_HANDLE_VALUE)
    {
    
    }

    ~IoctlComm(void)
    {
        if (driverHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(driverHandle);
            driverHandle = INVALID_HANDLE_VALUE;
        }
    }

    //
    // Connect to the driver, and keep the connection open
    //
    ATF_ERROR ConnectToDriver(void);

    //
    // Returns the driver logical device filename
    //
    const std::string &GetLogicalDeviceFileName(void) const;
};
