#include <Windows.h>

#include <fstream>
#include <stdint.h>

#include "resource.h"
#include "service.h"
#include "unpacker.h"
#include "../common/user_logging.h"
#include "../common/common.h"

#pragma comment(lib, "Advapi32.lib")

//
// Write executable files to <currProcessLocation>\\MAIN_INSTALL_PATH (see common.h)
//
int32_t doWriteExecutables(void);

//
// Startup the driver service, and the primary config service (see common.h)
// 
int32_t doServiceStartup(void);


//
// Entry point
//
int32_t main(int32_t argc, char argv[])
{
    LOG_INIT(DRIVER_CTL_NAME, LOG_SOURCE_WINDOWS_DEBUG);

    LOG_INFO("Starting process: %s", DRIVER_CTL_NAME);

    int32_t res = -1;

    // Unpack the driver and service
    res = doWriteExecutables();
    if (res) {
        LOG_ERROR("Failed to write executables: 0x%08x", res);
        return res;
    }

    // Startup the driver and service
    res = doServiceStartup();
    if (res) {
        LOG_ERROR("Failed to startup services: 0x%08x", res);
        return res;
    }

    return 0;
}

int32_t doWriteExecutables(void)
{
    std::string tempPath;
    int32_t res = GetTemporaryFilePath(tempPath);
    if (res) {
        LOG_ERROR("Failed to get temp path: 0x%08x", res);
        return res;
    }

    LOG_INFO("Temporary path: " + tempPath);

    enumResource();

    HRSRC hResource = FindResourceA(
        GetModuleHandleA(NULL),
        RT_RCDATA,
        MAKEINTRESOURCEA(ID_DRIVER_BIN_SYS)
    );
    if (hResource == NULL) {
        LOG_ERROR("Error: 0x%08x", GetLastError());
    }

    return 0;
}

int32_t doServiceStartup(void)
{
    SC_HANDLE scmHandle = InitializeScm();
    if (scmHandle == NULL) {
        LOG_ERROR("Failed to Initialze SCM");
        return -1;
    }

    LOG("Opened SCM successfully");

    static const SERVICE_PARAMS driverService(
        DRIVER_SERVICE_NAME,
        DRIVER_SERVICE_DISPLAY_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        DRIVER_BIN_PATH
    );

    SC_HANDLE driverServiceHandle = CreateScmService(scmHandle, driverService);
    if (driverServiceHandle == NULL) {
        CloseScmHandle(scmHandle);
        return -1;
    }

    LOG_INFO("Successfully opened service: %s", driverService.nameToDisplay);


    LOG_INFO("%s completed operations, closing.", DRIVER_CTL_NAME);
    CloseScmHandle(driverServiceHandle);
    CloseScmHandle(scmHandle);

    return 0;
}