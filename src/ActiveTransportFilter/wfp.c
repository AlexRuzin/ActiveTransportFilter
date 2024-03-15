#if !defined(NT)
#define NT
#endif //NT

#if !defined(NDIS60)
#define NDIS60 1
#endif //NDIS60

#if !defined(NDIS_SUPPORT_NDIS6)
#define NDIS_SUPPORT_NDIS6 1
#endif //NDIS_SUPPORT_NDIS6

#include <ntddk.h>
#include <fwpsk.h>
#include <fwpmk.h>

#include "wfp.h"
#include "trace.h"
#include "filter.h"

#include "../common/common.h"
#include "../common/default_config.h"

//
// Main callout functions
//
void NTAPI AtfClassifyFunc(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ void *layerData,
    _In_opt_    const void *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flow_context,
    _Inout_     FWPS_CLASSIFY_OUT0 *classify_out
);


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

    return STATUS_SUCCESS;
}

void NTAPI AtfClassifyFunc(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ void *layerData,
    _In_opt_    const void *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flow_context,
    _Inout_     FWPS_CLASSIFY_OUT0 *classify_out
)
{
    UNREFERENCED_PARAMETER(fixedValues);
    UNREFERENCED_PARAMETER(metaValues);
    UNREFERENCED_PARAMETER(layerData);
    UNREFERENCED_PARAMETER(classifyContext);
    UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(fixedValues);
    UNREFERENCED_PARAMETER(flow_context);
    UNREFERENCED_PARAMETER(classify_out);

    return;
}