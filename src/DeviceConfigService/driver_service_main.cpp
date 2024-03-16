#include <Windows.h>

#include <memory>
#include <string>

#include "driver_service_main.h"
#include "driver_comm.h"
#include "../common/user_logging.h"
#include "../common/common.h"
#include "../common/errors.h"
#include "../common/shared.h"
#include "ini_reader.h"

//
// Parse the ini and create the FilterConfig object
//
static ATF_ERROR parseIni(std::unique_ptr<FilterConfig> &filterConfig);

//
// Create a connection object to the driver
//
static ATF_ERROR connectToDriver(std::unique_ptr<IoctlComm> &ioctlComm);

//
// Returns an ini file, either it's in debug mode or it's in packed mode
//
static ATF_ERROR getIniFile(std::string &outPath);

static shared::CRC32SUM lastIniSum = -1;

int main(void)
{
    Sleep(CONTROL_SERVICE_SLEEP_MS);

    printf("test");
    Sleep(500);

    LOG_INIT(CONTROL_SERVICE_NAME, LOG_SOURCE_WINDOWS_DEBUG);
    LOG_INFO("Starting configuration service %s", CONTROL_SERVICE_NAME);

    ATF_ERROR atfError = ATF_ERROR_OK;

    std::unique_ptr<FilterConfig> filterConfig;
    atfError = parseIni(filterConfig);
    if (atfError) {
        LOG_ERROR("Failed to parse INI object: 0x%08x", atfError);
        return atfError;
    }

    std::unique_ptr<IoctlComm> ioctlComm;
    atfError = connectToDriver(ioctlComm);
    if (atfError) {
        LOG_ERROR("Failed to parse connect to driver: 0x%08x", atfError);
        return atfError;
    }

    LOG_INFO("Sucessfully connected to driver device: " + ioctlComm->GetLogicalDeviceFileName());

    Sleep(INFINITE);

    return 0;
}

static ATF_ERROR connectToDriver(std::unique_ptr<IoctlComm> &ioctlComm)
{
    ATF_ERROR atfError = ATF_ERROR_OK;

    // Check if driver device exists
    if (!shared::IsFileExists(ATF_DEVICE_NAME)) {
        return ATF_ERROR_FILE_NOT_FOUND;
    }

    ioctlComm = std::make_unique<IoctlComm>(ATF_DEVICE_NAME);
    atfError = ioctlComm->ConnectToDriver();
    if (atfError) {
        return atfError;
    }

    return ATF_ERROR_OK;
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

    lastIniSum = shared::Crc32SumFile(targetIniFile);
    LOG_INFO("Ini CRC32 sum: 0x%08x", lastIniSum);

    return ATF_ERROR_OK;
}

static ATF_ERROR getIniFile(std::string &outPath)
{
    if (shared::IsFileExists(GLOBAL_IP_FILTER_INI)) {
        outPath = GLOBAL_IP_FILTER_INI;
    } else if (shared::IsFileExists(GLOBAL_IP_FILTER_INI_DEBUG)) {
        outPath = GLOBAL_IP_FILTER_INI_DEBUG;
    } else {
        return ATF_ERROR_FILE_NOT_FOUND;
    }

    return ATF_ERROR_OK;
}
