#pragma once

#include <Windows.h>

#include <string>

//
// Retrive the SCM handle
//
SC_HANDLE InitializeScm(void);

void CloseScmHandle(HANDLE scmHandle);

typedef struct _service_params {
    const std::string                           serviceName;
    const std::string                           nameToDisplay;
    const DWORD                                 desiredAccess;
    const DWORD                                 serviceType;
    const DWORD                                 startType;
    const DWORD                                 errorControlType;
    const std::string                           pathToBin;

    _service_params(
        const char                              *serviceName,
        const char                              *nameToDisplay,
        const DWORD                             desiredAccess,
        const DWORD                             serviceType,
        const DWORD                             startType,
        const DWORD                             errorControlType,
        const char                              *pathToBin
    ) :
        serviceName(serviceName),
        nameToDisplay(nameToDisplay),
        desiredAccess(desiredAccess),
        serviceType(serviceType),
        startType(startType),
        errorControlType(errorControlType),
        pathToBin(pathToBin)
    {
    
    }

} SERVICE_PARAMS, *PSERVICE_PARAMS;

SC_HANDLE CreateScmService(SC_HANDLE scmHandle, const SERVICE_PARAMS &params);