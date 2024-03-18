#pragma once

//
// This class will periodically check the ini for changes, and send the change out to the driver via IOCTL
//

#include <Windows.h>

#include <string>
#include <vector>
#include <memory>

#include "driver_comm.h"
#include "../common/errors.h"

class ConfigRefreshService {
private:
    std::string                                 configIniFilename;

    //
    // Active connection to driver
    //
    std::shared_ptr<IoctlComm>                  ioctlComm;

public:
    ConfigRefreshService(void)
    {
    
    }

    //
    // Get ptr to driver instance
    //
    const std::shared_ptr<IoctlComm> &GetIoctlComm(void) const;
};