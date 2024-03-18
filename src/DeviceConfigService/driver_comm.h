#pragma once

#include <Windows.h>

#include <string>

#include "../common/common.h"
#include "../common/errors.h"

#include <string>
#include <vector>
#include <cstring>
#include <cstdint>

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
        //
        // Close device driver handle
        //
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

    //
    // Send a raw buffer to the device driver
    //
    ATF_ERROR SendRawBufferIoctl(std::vector<std::byte> &rawBuffer);

    //
    // Check if a device symbolic link exists (win32-only), C++ <filesystem> fails here
    //
    static ATF_ERROR tryOpenDevicePath(const std::string &in);
};
