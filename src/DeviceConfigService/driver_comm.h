#pragma once

#include <Windows.h>

#include <string>

#include "../common/common.h"
#include "../common/errors.h"

#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <memory>

//
// Global typedef for IOCTL control codes
//
typedef DWORD IOCTL_CODE, *PIOCTL_CODE;

class IoctlComm {
private:
    const std::string                               driverLogicalDevicePath;
    
    // Device handle to driver
    HANDLE                                          driverHandle;

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
    ATF_ERROR SendRawBufferIoctl(IOCTL_CODE ioctl, std::vector<std::byte> &rawBuffer) const;
    ATF_ERROR SendRawBufferIoctl(IOCTL_CODE ioctl, const void *ptr, size_t size) const;

    //
    // Send a raw IOCTL, without any input or output buffer
    //
    ATF_ERROR SendIoctlNoData(IOCTL_CODE ioctl) const;

    //
    // Check if a device symbolic link exists (win32-only), C++ <filesystem> fails here
    //
    static ATF_ERROR tryOpenDevicePath(const std::string &in);

    //
    // Is the object connected
    //
    bool GetIsConnected(void) const;
};
