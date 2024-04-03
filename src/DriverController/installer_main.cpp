#include <Windows.h>

#include <fstream>
#include <stdint.h>
#include <map>

#include "resource.h"
#include "service.h"
#include "unpacker.h"
#include "../common/shared.h"
#include "../common/user_logging.h"
#include "../common/errors.h"
#include "../common/common.h"

#pragma comment(lib, "Advapi32.lib")

//
// Write executable files to <currProcessLocation>\\MAIN_INSTALL_PATH (see common.h)
//
ATF_ERROR doWriteExecutables(void);

//
// Startup the driver service (see common.h)
// 
ATF_ERROR doDriverServiceStartup(void);

//
// Startup the driver config service (see common.h)
//
ATF_ERROR doConfigServiceStartup(void);

//
// Cleanup of the installer directory (MAIN_INSTALL_PATH) (see common.h)
//
ATF_ERROR cleanInstallDirectory(const std::string &path);


//
// Entry point
//
int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
    LOG_INIT(DRIVER_CTL_NAME, LOG_SOURCE_WINDOWS_DEBUG);

    LOG_INFO("Starting process(1): %s", DRIVER_CTL_NAME);

    ATF_ERROR res = ATF_ERROR_FAIL;

    //
    // Unpack the driver and service
    //
    res = doWriteExecutables();
    if (res) {
        LOG_ERROR("Failed to write executables: 0x%08x", res);
        return res;
    }
    Sleep(100);

    //
    // Startup the driver
    //
    res = doDriverServiceStartup();
    if (res) {
        LOG_ERROR("Failed to startup services: 0x%08x", res);
        return res;
    }
    Sleep(100);

    //
    // Startup config service
    //
    res = doConfigServiceStartup();
    if (res) {
        LOG_ERROR("Failed to start config service: 0x%08x", res);
        return res;
    }

    LOG_INFO("%s completed operations, closing", DRIVER_CTL_NAME);

    return 0;
}

ATF_ERROR cleanInstallDirectory(const std::string &path)
{
    LOG_INFO("Deleting temp directory: " + path);
    RemoveDirectoryA(path.c_str());

    LOG_INFO("Creating temp directory: " + path);
    CreateDirectoryA(path.c_str(), NULL);

    return ATF_ERROR_OK;
}

ATF_ERROR doWriteExecutables(void)
{
    std::string tempPath;
    ATF_ERROR res = GetTemporaryFilePath(tempPath);
    if (res) {
        LOG_ERROR("Failed to get temp path: 0x%08x", res);
        return res;
    }

    LOG_INFO("Temporary path: " + tempPath);

    // Cleanup the temp directory (install directory)
#if 0
    // Removing this because I am manually pushing library DLLs to the machine. In release builds, this will be enabled
    cleanInstallDirectory(tempPath);   
#endif

    // Contains resources and their paths
    static const std::map<int, std::string> resPaths = {
        { ID_DRIVER_BIN_SYS, FILENAME_ATF_DRIVER },
        { ID_CONFIG_SERVICE_BIN, FILENAME_DEVICE_CONFIG_SERVICE },
        { ID_USER_INTERFACE_CONSOLE_BIN, FILENAME_INTERFACE_CONSOLE },
        { ID_WFP_CONFIG_FILE, FILENAME_CONFIG }
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

    return ATF_ERROR_OK;
}

ATF_ERROR doConfigServiceStartup(void)
{
    std::string tempPath;
    GetTemporaryFilePath(tempPath); 

    const std::string fullConfigServicePath = tempPath + FILENAME_DEVICE_CONFIG_SERVICE;
    if (!shared::IsFileExists(fullConfigServicePath)) {
        return ATF_ERROR_FILE_NOT_FOUND;
    }


    STARTUPINFOA startupInfo = { 0 };
    startupInfo.cb = sizeof(STARTUPINFOA);

    PROCESS_INFORMATION procInfo = { 0 };

    if (!CreateProcessA(
        fullConfigServicePath.c_str(),
        NULL,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &startupInfo,
        &procInfo
    )) 
    {
        LOG_ERROR("CreateProcessA failed: 0x%08x", GetLastError());
        return ATF_FAILED_PROC_CREATE;
    }

    Sleep(1000);

    DWORD exitCode = 0;
    if (!GetExitCodeProcess(procInfo.hProcess, &exitCode) || exitCode != STILL_ACTIVE) {
        LOG_ERROR("Failed to start service: %s (%d)", fullConfigServicePath.c_str(), procInfo.dwProcessId);
        CloseHandle(procInfo.hThread);
        CloseHandle(procInfo.hProcess);
        return ATF_FAILED_PROC_CREATE;
    } else if (exitCode == STILL_ACTIVE) {
        LOG_INFO("Created config service. PID: %d", procInfo.dwProcessId);
    } 

    CloseHandle(procInfo.hThread);
    CloseHandle(procInfo.hProcess);
    return ATF_ERROR_OK;
}

ATF_ERROR doDriverServiceStartup(void)
{
    SC_HANDLE scmHandle = InitializeScm();
    if (scmHandle == NULL) {
        LOG_ERROR("Failed to Initialze SCM");
        return ATF_INIT_SCM;
    }

    LOG("Opened SCM successfully");

    std::string tempPath;
    GetTemporaryFilePath(tempPath);  

    const std::string driverFullPath = tempPath + DRIVER_BIN_PATH;
    if (!shared::IsFileExists(driverFullPath)) {
        return ATF_ERROR_FILE_NOT_FOUND;
    }

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
        return ATF_CREATE_SERVICE;
    }

    LOG_INFO("Successfully opened service: %s (handle: 0x%08x, err: 0x%08x)", driverService.nameToDisplay.c_str(), driverServiceHandle, GetLastError());

    //TODO cleanup
    CloseScmHandle(driverServiceHandle);
    CloseScmHandle(scmHandle);

    return ATF_ERROR_OK;
}