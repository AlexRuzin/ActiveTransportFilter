#pragma once

#include <Windows.h>

#include "../common/ioctl_codes.h"
#include "../common/errors.h"
#include "driver_comm.h"
#include "ini_reader.h"

#include <inaddr.h>

#include <memory>
#include <string>
#include <map>

#if defined(_DEBUG)
#undef OVERRIDE_CONN_REQ
#if defined(OVERRIDE_CONN_REQ)
#pragma message("WARNING: OVERRIDE_CONN_REQ is enabled!")
#endif //OVERRIDE_CONN_REQ
#endif _DEBUG

//
// IOCTL command wrapper (see ioctl_codes.h)
//
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
    //  IOCTL_ATF_WFP_SERVICE_START
    //
    ATF_ERROR CmdStartWfp(void);

    //
    // Command to signal WFP to stop
    //  IOCTL_ATF_WFP_SERVICE_STOP
    //
    ATF_ERROR CmdStopWfp(void);

    //
    // Command fto flush the configuation
    //  IOCTL_ATF_FLUSH_CONFIG
    //
    ATF_ERROR CmdFlushConfig(void) const;

    //
    // Default configuration load from the ini file (see ini_reader.h -- FilterConfig class)
    //  IOCTL_ATF_SEND_WFP_CONFIG
    //
    ATF_ERROR CmdSendIniConfiguration(const FilterConfig &filterConfig) const;

    //
    // Command to append an IPv4 blacklist to the driver
    //  IOCTL_ATF_APPEND_IPV4_BLACKLIST
    //
    ATF_ERROR CmdAppendIpv4Blacklist(const FilterConfig &filterConfig) const;

private:
    //
    // Returns the device driver connection state (ready)
    //  The state returns:
    //   1) The IoctlComm object exists
    //   2) The IoctlComm object is connected
    //
    bool isDeviceReady(void) const;

    //
    // Returns the state of the WFP engine
    //  Config update commands require the engine to be in an OFF state
    //
    bool isWfpReady(void) const;
};
