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
// Main callout functions (TCP ipv4)
//
void NTAPI AtfClassifyFuncTcpV4(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ void *layerData,
    _In_opt_    const void *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flow_context,
    _Inout_     FWPS_CLASSIFY_OUT0 *classify_out
);

//
// Main callout function (TCP v6)
//
void NTAPI AtfClassifyFuncTcpV6(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ void *layerData,
    _In_opt_    const void *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flow_context,
    _Inout_     FWPS_CLASSIFY_OUT0 *classify_out
);

// 
// Main callout function (ICMP)
//
void NTAPI AtfClassifyFuncIcmp(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ void *layerData,
    _In_opt_    const void *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flow_context,
    _Inout_     FWPS_CLASSIFY_OUT0 *classify_out
);

//
// Default notify function for adding or deleting layers
//
NTSTATUS NTAPI AtfNotifyFunctionHandler(
    _In_    FWPS_CALLOUT_NOTIFY_TYPE notifyType,
    _In_    const GUID* filterKey,
    _Inout_ FWPS_FILTER3* filter
);

// 
// Notify flow callback
//
void NTAPI AtfFlowDeleteFunctionHandler(
    _In_ UINT16 layerId,
    _In_ UINT32 calloutId,
    _In_ UINT64 flowContext
);

//
// Callout layer descriptions and definitions
//
typedef struct _callout_desc_layer {
    const GUID                      *guid; // GUID of the layer, as defined in fwpmk.h

    // Callout descriptor
    const wchar_t                   *calloutName;
    const wchar_t                   *calloutDesc;

    // Filter descriptor
    const wchar_t                   *filterName;
    const wchar_t                   *filterDesc;

    FWPS_CALLOUT_CLASSIFY_FN3       callback;
} CALLOUT_DESC, *PCALLOUT_DESC;

//
// Add a callout to the WFP engine
//
static NTSTATUS AtfAddCalloutLayer(
    const CALLOUT_DESC calloutDesc
);

const CALLOUT_DESC descList[] = {
    // TCP Inbound v4
    {
        &FWPM_LAYER_INBOUND_TRANSPORT_V4,

        L"ATF Callout TCP V4 Inbound",
        L"ATF Callout Transport ipv4 Inbound",

        L"ATF Filter TCP V4 Inbound",
        L"ATF Filter Transport ipv4 Inbound",

        AtfClassifyFuncTcpV4
    },

    // TCP Outbound v6
    {
        &FWPM_LAYER_INBOUND_TRANSPORT_V6,

        L"ATF Callout TCP V6 Inbound",
        L"ATF Callout Transport ipv6 Inbound",

        L"ATF Filter TCP V6 Inbound",
        L"ATF Filter Transport ipv6 Inbound",

        AtfClassifyFuncTcpV6
    },

    // TCP Outbound v4
    {
        &FWPM_LAYER_OUTBOUND_TRANSPORT_V4,

        L"ATF Callout TCP V4 Outbound",
        L"ATF Callout Transport ipv4 Outbound",

        L"ATF Filter TCP V4 Outbound",
        L"ATF Filter Transport ipv4 Outbound",

        AtfClassifyFuncTcpV4
    },

    // TCP Outbound v6
    {
        &FWPM_LAYER_OUTBOUND_TRANSPORT_V6,

        L"ATF Callout TCP V6 Outbound",
        L"ATF Callout Transport ipv6 Outbound",

        L"ATF Filter TCP V6 Outbound",
        L"ATF Filter Transport ipv6 Outbound",

        AtfClassifyFuncTcpV6
    },

    // ICMP Original Type
    {
        &FWPM_CONDITION_ORIGINAL_ICMP_TYPE,

        L"ATF Callout ICMP",
        L"ATF Callout ICMP Original Type",

        L"ATF Filter ICMP",
        L"ATF Filter ICMP Original Type",

        AtfClassifyFuncIcmp
    }
};


static HANDLE kmfeHandle = 0; //Kernel Mode Filter Engine (KMFE)
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

    fwpmProvider.serviceName                = (wchar_t *)TEXT(DRIVER_FWPM_SERVICENAME);
    fwpmProvider.displayData.name           = (wchar_t *)TEXT(DRIVER_FWPM_DISPLAYNAME);
    fwpmProvider.displayData.description    = (wchar_t *)TEXT(DRIVER_FWPM_DESC);
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

    for (UINT8 currLayer = 0; currLayer < ARRAYSIZE(descList); currLayer++) {
        ntStatus = AtfAddCalloutLayer(descList[currLayer]);
        if (!NT_SUCCESS(ntStatus)) {
            ATF_ERROR(AtfAddCalloutLayer, ntStatus);
            return ntStatus;
        }
    }

    ATF_DEBUG(InitializeWfp, "All WFP operations successful...");
    return ntStatus;
}

static NTSTATUS AtfAddCalloutLayer(
    const CALLOUT_DESC calloutDesc
)
{
    if (!atfDevice) {
        return STATUS_APP_INIT_FAILURE;
    }

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!kmfeHandle) {
        return STATUS_APP_INIT_FAILURE;
    }

    ntStatus = FwpmTransactionBegin(kmfeHandle, 0);
    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    ATF_DEBUGF("Attempting to register layer %ls", calloutDesc.calloutName);

    //
    // Callout registration
    //
    FWPS_CALLOUT fwpsCallout = { 0 };
    fwpsCallout.classifyFn = calloutDesc.callback;
    fwpsCallout.notifyFn = AtfNotifyFunctionHandler;
    fwpsCallout.flowDeleteFn = AtfFlowDeleteFunctionHandler;
    ExUuidCreate(&fwpsCallout.calloutKey);

    UINT32 fwpsCalloutId = 0;
    ntStatus = FwpsCalloutRegister(
        atfDevice,
        &fwpsCallout,
        &fwpsCalloutId
    );
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(FwpsCalloutRegister, ntStatus);
        return ntStatus;
    }
    ATF_DEBUGF("Registered callout layer %ls", calloutDesc.calloutName);

    //
    // Add a callout object
    //
    FWPM_CALLOUT fwpmCallout = { 0 };
    fwpmCallout.calloutKey = fwpsCallout.calloutKey;
    fwpmCallout.displayData.name = (wchar_t *)calloutDesc.calloutName;
    fwpmCallout.displayData.description = (wchar_t *)calloutDesc.callback;
    fwpmCallout.providerKey = (GUID*)&ATF_FWPM_PROVIDER_KEY;
    fwpmCallout.applicableLayer = *calloutDesc.guid;

    UINT32 fwpmCalloutId = 0;
    ntStatus = FwpmCalloutAdd(
        kmfeHandle,
        &fwpmCallout,
        NULL,
        &fwpmCalloutId
    );
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(FwpmCalloutAdd, ntStatus);
        return ntStatus;
    }
    ATF_DEBUGF("Added callout to layer %ls", calloutDesc.calloutName);

    //
    // Add a filter key
    //
    FWPM_FILTER fwpmFilter = { 0 };
    fwpmFilter.displayData.name = (wchar_t *)calloutDesc.filterName;
    fwpmFilter.displayData.description = (wchar_t *)calloutDesc.filterDesc;
    fwpmFilter.layerKey = *calloutDesc.guid;
    fwpmFilter.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
    fwpmFilter.action.calloutKey = fwpmCallout.calloutKey;

    UINT64 fwpmFilterId = 0;
    ntStatus = FwpmFilterAdd(
        kmfeHandle,
        &fwpmFilter,
        NULL,
        &fwpmFilterId
    );
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(FwpmFilterAdd, ntStatus);
        return ntStatus;
    }
    ATF_DEBUGF("Added filter to layer %ls", calloutDesc.filterName);

    ntStatus = FwpmTransactionCommit(kmfeHandle);
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(FwpmTransactionCommit, ntStatus);
        return ntStatus;
    }


    //TODO implement proper cleanup: FwpmTransactionAbort
    return ntStatus;
}

NTSTATUS DestroyWfp(
    _In_ DEVICE_OBJECT *deviceObject
)
{
    UNREFERENCED_PARAMETER(deviceObject);

    FwpmTransactionCommit(kmfeHandle);
    FwpmProviderDeleteByKey(kmfeHandle, &ATF_FWPM_PROVIDER_KEY);
    FwpmEngineClose(kmfeHandle);

    return STATUS_SUCCESS;
}

void NTAPI AtfClassifyFuncTcpV4(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ void *layerData,
    _In_opt_    const void *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flow_context,
    _Inout_     FWPS_CLASSIFY_OUT0 *classify_out
)
{
    ATF_DEBUG(AtfClassifyFuncTcpV4, "Entered Func");

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

void NTAPI AtfClassifyFuncTcpV6(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ void *layerData,
    _In_opt_    const void *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flow_context,
    _Inout_     FWPS_CLASSIFY_OUT0 *classify_out
)
{
    ATF_DEBUG(AtfClassifyFuncTcpV6, "Entered Func");

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

void NTAPI AtfClassifyFuncIcmp(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ void *layerData,
    _In_opt_    const void *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flow_context,
    _Inout_     FWPS_CLASSIFY_OUT0 *classify_out
)
{
    ATF_DEBUG(AtfClassifyFuncIcmp, "Entered Func");

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

NTSTATUS NTAPI AtfNotifyFunctionHandler(
    _In_    FWPS_CALLOUT_NOTIFY_TYPE notifyType,
    _In_    const GUID* filterKey,
    _Inout_ FWPS_FILTER3* filter
)
{
    UNREFERENCED_PARAMETER(filterKey);
    UNREFERENCED_PARAMETER(filter);

    switch (notifyType)
    {
    case FWPS_CALLOUT_NOTIFY_ADD_FILTER:
        {
            ATF_DEBUGF("Notify routine: FWPS_CALLOUT_NOTIFY_ADD_FILTER (0x%08x)", notifyType);
            break;
        }
    case FWPS_CALLOUT_NOTIFY_DELETE_FILTER:
        {
            ATF_DEBUGF("Notify routine: FWPS_CALLOUT_NOTIFY_DELETE_FILTER (0x%08x)", notifyType);
            break;
        }
    default:
        {
            ATF_ERRORF("Unknown notifyType: 0x%08x", notifyType);
            break;
        }
    }

    return STATUS_SUCCESS;
}

void NTAPI AtfFlowDeleteFunctionHandler(
    _In_ UINT16 layerId,
    _In_ UINT32 calloutId,
    _In_ UINT64 flowContext
)
{
    UNREFERENCED_PARAMETER(layerId);
    UNREFERENCED_PARAMETER(calloutId);
    UNREFERENCED_PARAMETER(flowContext);

    ATF_DEBUGF(AtfFlowDeleteFunctionHandler, "Received layerId: 0x%08x", layerId);

    return;
}