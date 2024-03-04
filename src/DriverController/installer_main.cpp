#include <Windows.h>

#include <fstream>
#include <stdint.h>
#include <map>

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
// Cleanup of the installer directory (MAIN_INSTALL_PATH) (see common.h)
//
int32_t cleanInstallDirectory(const std::string &path);


//
// Entry point
//
int32_t main(int32_t argc, char argv[])
{
    LOG_INIT(DRIVER_CTL_NAME, LOG_SOURCE_WINDOWS_DEBUG);

    LOG_INFO("Starting process(1): %s", DRIVER_CTL_NAME);

    int32_t res = -1;

    //
    // Unpack the driver and service
    //
    res = doWriteExecutables();
    if (res) {
        LOG_ERROR("Failed to write executables: 0x%08x", res);
        return res;
    }

    //
    // Startup the driver and service
    //
    res = doServiceStartup();
    if (res) {
        LOG_ERROR("Failed to startup services: 0x%08x", res);
        return res;
    }

    LOG_INFO("%s completed operations, closing", DRIVER_CTL_NAME);

    return 0;
}

int32_t cleanInstallDirectory(const std::string &path)
{
    LOG_INFO("Deleting temp directory: " + path);
    RemoveDirectoryA(path.c_str());

    LOG_INFO("Creating temp directory: " + path);
    CreateDirectoryA(path.c_str(), NULL);

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

    // Cleanup the temp directory (install directory)
    cleanInstallDirectory(tempPath);   

    // Contains resources and their paths
    static const std::map<int, std::string> resPaths = {
        { ID_DRIVER_BIN_SYS, FILENAME_ATF_DRIVER },
        { ID_CONFIG_SERVICE_BIN, FILENAME_DEVICE_CONFIG_SERVICE },
        { ID_USER_INTERFACE_CONSOLE_BIN, FILENAME_INTERFACE_CONSOLE }
    };

    for (std::map<int, std::string>::const_iterator i = resPaths.begin(); i != resPaths.end(); i++) {
        const std::string path = tempPath + "\\" + i->second;

        LOG_INFO("Extracting file: %s (id: 0x%08x)", i->second.c_str(), i->first);

        res = ExtractResourceToPath(i->first, path);
        if (res) {
            LOG_ERROR("Failed to extract id: %d, path: %s", i->first, i->second);
            return res;
        }
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

    std::string tempPath;
    GetTemporaryFilePath(tempPath);  

    const std::string driverFullPath = tempPath + DRIVER_BIN_PATH;

    static const SERVICE_PARAMS driverService(
        DRIVER_SERVICE_NAME,
        DRIVER_SERVICE_DISPLAY_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        driverFullPath.c_str()
    );

    SC_HANDLE driverServiceHandle = CreateScmService(scmHandle, driverService);
    if (driverServiceHandle == NULL) {
        CloseScmHandle(scmHandle);
        return -1;
    }

    LOG_INFO("Successfully opened service: %s (handle: 0x%08x, err: 0x%08x)", driverService.nameToDisplay.c_str(), driverServiceHandle, GetLastError());


    //TODO cleanup
    CloseScmHandle(driverServiceHandle);
    CloseScmHandle(scmHandle);

    return 0;
}