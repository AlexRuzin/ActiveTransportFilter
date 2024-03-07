
#include <ntddk.h>
#include <wdf.h>
#include <fwpmk.h>
//#include <fwpsk.h>

#include "wfp.h"
#include "trace.h"
#include "../common/common.h"

static HANDLE kmfeHandle; //Kernel Mode Filter Engine (KMFE)
static DEVICE_OBJECT *atfDevice = NULL;

NTSTATUS InitializeWfp(
    _In_ DEVICE_OBJECT *deviceObj 
)
{
    ATF_ASSERT(deviceObj);

    ATF_DEBUG(InitializeWfp, "Entering...");
    NTSTATUS ntStatus = -1;

    FWPM_PROVIDER fwpmProvider = { 0 };

    ntStatus = FwpmEngineOpen(
        NULL,
        RPC_C_AUTHN_DEFAULT,
        NULL,
        NULL,
        &kmfeHandle
    );
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(FwpmEngineOpen, ntStatus);
        return ntStatus;
    }

    ntStatus = FwpmTransactionBegin(
        kmfeHandle, 
        0
    );
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(FwpmTransactionBegin, ntStatus);
        FwpmTransactionAbort(kmfeHandle);
        kmfeHandle = NULL;
        return ntStatus;
    }

    fwpmProvider.serviceName                = (wchar_t *)DRIVER_FWPM_SERVICENAME;
    fwpmProvider.displayData.name           = (wchar_t *)DRIVER_FWPM_DISPLAYNAME;
    fwpmProvider.displayData.description    = (wchar_t *)DRIVER_FWPM_DESC;
    fwpmProvider.providerKey                = ATF_FWPM_PROVIDER_KEY;

    ntStatus = FwpmProviderAdd(
        kmfeHandle,
        &fwpmProvider,
        NULL
    );
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(FwpmProviderAdd, ntStatus);
        FwpmTransactionAbort(kmfeHandle);
        kmfeHandle = NULL;
        return ntStatus;
    }

    ntStatus = FwpmTransactionCommit(kmfeHandle);
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(FwpmProviderAdd, ntStatus);
        FwpmTransactionAbort(kmfeHandle);
        kmfeHandle = NULL;
        return ntStatus;
    }

    atfDevice = deviceObj;

    ATF_DEBUG(InitializeWfp, "Success...");
    return ntStatus;
}

NTSTATUS DestroyWfp(
    VOID
)
{
    FwpmTransactionCommit(kmfeHandle);
    FwpmProviderDeleteByKey(kmfeHandle, &ATF_FWPM_PROVIDER_KEY);
    FwpmEngineClose(kmfeHandle);
}