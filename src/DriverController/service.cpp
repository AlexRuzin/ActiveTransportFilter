#include <Windows.h>

#include "service.h"
#include "../common/user_logging.h"

#include <string>
#include <stdio.h>

SC_HANDLE InitializeScm(void)
{
    SC_HANDLE scmHandle = OpenSCManagerA(
        NULL,
        NULL,
        0xF003F
    );

    if (scmHandle == NULL) {
        LOG_ERROR("OpenSCManagerA() failed: 0x%08x", GetLastError());
        return NULL;
    }
    
    return scmHandle;
}

void CloseScmHandle(HANDLE scmHandle)
{
    CloseHandle(scmHandle);
}

SC_HANDLE CreateScmService(SC_HANDLE scmHandle, const SERVICE_PARAMS &params)
{
    if (scmHandle == NULL) {
        return NULL;
    }

    LOG_INFO("Starting service: " + params.serviceName + " \"" + params.nameToDisplay + "\" " + params.pathToBin);

    Sleep(1000);
    SC_HANDLE serviceHandle = CreateServiceA(
        scmHandle,
        params.serviceName.c_str(),
        params.nameToDisplay.c_str(),
        params.desiredAccess,
        params.serviceType,
        params.startType,
        params.errorControlType,
        params.pathToBin.c_str(),
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    );
    if (serviceHandle == NULL) {
        LOG_ERROR("CreateServiceA() Failed: 0x%08x", GetLastError());
        return NULL;
    }

    Sleep(1000);
    SC_HANDLE openServiceHandle = OpenServiceA(
        scmHandle,
        params.serviceName.c_str(),
        params.desiredAccess
    );
    if (!openServiceHandle) {
        LOG_ERROR("OpenService failed: 0x%08x", GetLastError());
        return NULL;
    }

    Sleep(1000);
    if (!StartServiceA(openServiceHandle, 0, 0)) {
        LOG_ERROR("StartService failed: 0x%08x", GetLastError());
        return NULL;
    }
    LOG("Start Service Success");

    return serviceHandle;
}