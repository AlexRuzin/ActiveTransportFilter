#include <Windows.h>

#include "main.h"
#include "config_service.h"
#include "driver_comm.h"
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
// Create a connection object to the driver
//
static ATF_ERROR connectToDriver(std::unique_ptr<IoctlComm> &ioctlComm);

//
// Returns an ini file, either it's in debug mode or it's in packed mode
//
static ATF_ERROR getIniFile(std::string &outPath);

//
// Use Win32 API to check for the device path. shared::IsFileExists() does not work.
//
static ATF_ERROR tryOpenDevicePath(const std::string &in);


//
// Global that stores the checksum for the last ini file read (TODO)
//
static shared::CRC32SUM lastIniSum = -1;

int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
    LOG_INIT(CONTROL_SERVICE_NAME, LOG_SOURCE_WINDOWS_DEBUG);
    LOG_INFO("Starting configuration service %s", CONTROL_SERVICE_NAME);

    ATF_ERROR atfError = ATF_ERROR_OK;

    Sleep(500);

    // Parse the ini configuration
    std::unique_ptr<FilterConfig> filterConfig;
    atfError = parseIni(filterConfig);
    if (atfError) {
        LOG_ERROR("Failed to parse INI object: 0x%08x", atfError);
        return atfError;
    }

    // Connect to the device driver
    std::unique_ptr<IoctlComm> ioctlComm;
    atfError = connectToDriver(ioctlComm);
    if (atfError) {
        LOG_ERROR("Failed to parse connect to driver: 0x%08x", atfError);
        return atfError;
    }

    LOG_INFO("Sucessfully connected to driver device: " + ioctlComm->GetLogicalDeviceFileName());

    // Transport configuration
    std::vector<std::byte> rawBuf = filterConfig->SerializeConfigBuffer();
    atfError = ioctlComm->SendRawBufferIoctl(rawBuf);
    if (atfError) {
        LOG_ERROR("Failed to transport configuration buffer: 0x%08x", atfError);
        return atfError;
    }

    Sleep(INFINITE);
    return 0;
}

static ATF_ERROR connectToDriver(std::unique_ptr<IoctlComm> &ioctlComm)
{
    ATF_ERROR atfError = ATF_ERROR_OK;

    // We need to append "\\.\" to the device path for CreateFileA() to work
    std::string fullDeviceName(ATF_DRIVER_NAME);
    fullDeviceName.insert(0, "\\\\.\\");

    // Check if driver device exists
    atfError = IoctlComm::tryOpenDevicePath(fullDeviceName);
    if (atfError) {
        return atfError;
    }

    ioctlComm = std::make_unique<IoctlComm>(fullDeviceName);
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
    } else if (shared::IsFileExists(GLOBAL_IP_FILTER_INI_RUNTIME)) {
        outPath = GLOBAL_IP_FILTER_INI_RUNTIME;
    } else {
        return ATF_ERROR_FILE_NOT_FOUND;
    }

    return ATF_ERROR_OK;
}