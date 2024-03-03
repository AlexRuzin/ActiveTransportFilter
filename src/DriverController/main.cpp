#include <Windows.h>

#include <stdint.h>

#include "service.h"
#include "../common/user_logging.h"
#include "../common/common.h"

#pragma comment(lib, "Advapi32.lib")

int32_t main(int32_t argc, char argv[])
{
    LOG_INIT(DRIVER_CTL_NAME, LOG_SOURCE_WINDOWS_DEBUG);

    LOG_INFO("Starting process: %s", DRIVER_CTL_NAME);

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

    LOG("Successfully opened service: %s", driverService.nameToDisplay);


    LOG("%s completed operations, closing.", DRIVER_CTL_NAME);
    CloseScmHandle(driverServiceHandle);
    CloseScmHandle(scmHandle);

    return 0;
}