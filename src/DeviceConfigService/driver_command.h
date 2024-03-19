#pragma once

#include <Windows.h>

#include "../common/ioctl_codes.h"
#include "../common/errors.h"
#include "driver_comm.h"
#include "ini_reader.h"

#include <memory>
#include <string>
#include <map>

class DriverCommand {
public:
    static inline const std::map<DWORD, std::string> commandDesc = {
        { IOCTL_ATF_SEND_KEEPALIVE, "KEEPALIVE" },
        { IOCTL_ATF_WFP_SERVICE_START, "START_WFP" },
        { IOCTL_ATF_WFP_SERVICE_STOP, "STOP_WFP" },
        { IOCTL_ATF_FLUSH_CONFIG, "FLUSH_CONFIG" },
        { IOCTL_ATF_SEND_WFP_CONFIG, "SET_INI_CONFIG" }
    };

private:
    const std::string                       deviceFilepath;

    std::unique_ptr<IoctlComm>              ioctlComm;

    bool                                    wfpRunning;

public:
    DriverCommand(const std::string deviceFilePathIn) :
        deviceFilepath(deviceFilePathIn),
        wfpRunning(false)
    {
    
    }

    ~DriverCommand(void)
    {

    }
    
    //
    // Initialize communication with the driver via IOCTL
    //
    ATF_ERROR InitializeDriverComms(void);

    //
    // Command to signal WFP to start
    //
    ATF_ERROR CmdStartWfp(void);

    //
    // Command to signal WFP to stop
    //
    ATF_ERROR CmdStopWfp(void);

    //
    // Command fto flush the configuation
    //
    ATF_ERROR CmdFlushConfig(void) const;

    //
    // Default configuration load from the ini file (see ini_reader.h -- FilterConfig class)
    //
    ATF_ERROR CmdSendIniConfiguration(const FilterConfig &filterConfig) const;
};
