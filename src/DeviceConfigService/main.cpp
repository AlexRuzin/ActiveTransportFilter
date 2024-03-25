#include <Windows.h>

#include "main.h"
#include "config_service.h"
#include "driver_command.h"
#include "../common/user_logging.h"
#include "../common/common.h"
#include "../common/errors.h"
#include "../common/shared.h"
#include "ini_reader.h"

#include <string>
#include <vector>
#include <cstring>
#include <cstdint>

//
// Parse the ini and create the FilterConfig object
//
static ATF_ERROR parseIni(std::unique_ptr<FilterConfig> &filterConfig);

//
// Returns an ini file, either it's in debug mode or it's in packed mode
//
static ATF_ERROR getIniFile(std::string &outPath);


int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd
)
{
    LOG_INIT(CONTROL_SERVICE_NAME, LOG_SOURCE_WINDOWS_DEBUG);
    LOG_INFO("Starting configuration service %s", CONTROL_SERVICE_NAME);

    ATF_ERROR atfError = ATF_ERROR_OK;

    Sleep(500);

    DriverCommand driverCommand(ATF_DRIVER_NAME);
    atfError = driverCommand.InitializeDriverComms();
    if (atfError) {
        LOG_ERROR("Failed to open connection to device: 0x%08x", atfError);
        //return atfError;
    }

    std::unique_ptr<FilterConfig> filterConfig;
    atfError = parseIni(filterConfig);
    if (atfError) {
        LOG_ERROR("Failed to parse INI object: 0x%08x", atfError);
        return atfError;
    }

    atfError = driverCommand.CmdSendIniConfiguration(*filterConfig);
    if (atfError) {
        LOG_ERROR("Failed to send ini command (0x%08x)", atfError);
        return atfError;
    }

    atfError = driverCommand.CmdStartWfp();
    if (atfError) {
        LOG_ERROR("Failed to start WFP 0x%08x", atfError);
        return atfError;
    }

    #if 0
    Sleep(10000);
    atfError = driverCommand.CmdStopWfp();
    if (atfError) {
        LOG_ERROR("Failed to start WFP 0x%08x", atfError);
        return atfError;
    }
    #endif

    #if 0
    ConfigRefreshService refreshService;

    atfError = refreshService.ConnectToDriver();
    if (atfError) {
        LOG_ERROR("Failed to parse connect to driver: 0x%08x", atfError);
        return atfError;
    }

    LOG_INFO("Sucessfully connected to driver device: " + refreshService.GetIoctlComm()->GetLogicalDeviceFileName());


    // Parse the ini configuration
    std::unique_ptr<FilterConfig> filterConfig;
    atfError = parseIni(filterConfig);
    if (atfError) {
        LOG_ERROR("Failed to parse INI object: 0x%08x", atfError);
        return atfError;
    }
    

    // Transport configuration
    std::vector<std::byte> rawBuf = filterConfig->SerializeConfigBuffer();
    atfError = ioctlComm->SendRawBufferIoctl(rawBuf);
    if (atfError) {
        LOG_ERROR("Failed to transport configuration buffer: 0x%08x", atfError);
        return atfError;
    }
    #endif

    Sleep(INFINITE);
    return 0;
}


static ATF_ERROR parseIni(std::unique_ptr<FilterConfig> &filterConfig)
{
    std::string targetIniFile;
    ATF_ERROR atfError = getIniFile(targetIniFile);
    if (atfError) {
        return atfError;
    }

    LOG_INFO("Found ini config path: " + targetIniFile);

    filterConfig = std::make_unique<FilterConfig>(targetIniFile);
    atfError = filterConfig->ParseIniFile();
    if (atfError) {
        return atfError;
    }

    return ATF_ERROR_OK;
}

static ATF_ERROR getIniFile(std::string &outPath)
{
    if (shared::IsFileExists(GLOBAL_IP_FILTER_INI)) {
        outPath = GLOBAL_IP_FILTER_INI;
    } else if (shared::IsFileExists(GLOBAL_IP_FILTER_INI_DEBUG)) {
        outPath = GLOBAL_IP_FILTER_INI_DEBUG;
    } else if (shared::IsFileExists(GLOBAL_IP_FILTER_INI_RUNTIME)) {
        outPath = GLOBAL_IP_FILTER_INI_RUNTIME;
    } else {
        return ATF_ERROR_FILE_NOT_FOUND;
    }

    return ATF_ERROR_OK;
}