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
// Main callout functions (TCP ipv4 inbound)
//
VOID NTAPI AtfClassifyFuncTcpV4Inbound(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ VOID *layerData,
    _In_opt_    const VOID *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flowContext,
    _Inout_     FWPS_CLASSIFY_OUT0 *classifyOut
);

//
// Main callout functions (TCP ipv4 outbound)
//
VOID NTAPI AtfClassifyFuncTcpV4Outbound(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ VOID *layerData,
    _In_opt_    const VOID *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flowContext,
    _Inout_     FWPS_CLASSIFY_OUT0 *classifyOut
);

//
// Main callout function datagram IPv4
//
VOID NTAPI AtfClassifyFuncDatagram(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ VOID *layerData,
    _In_opt_    const VOID *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flowContext,
    _Inout_     FWPS_CLASSIFY_OUT0 *classifyOut
);

//
// Main callout function (TCP v6)
//
VOID NTAPI AtfClassifyFuncTcpV6(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ VOID *layerData,
    _In_opt_    const VOID *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flowContext,
    _Inout_     FWPS_CLASSIFY_OUT0 *classifyOut
);

// 
// Main callout function (ICMP)
//
VOID NTAPI AtfClassifyFuncIcmp(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ VOID *layerData,
    _In_opt_    const VOID *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flowContext,
    _Inout_     FWPS_CLASSIFY_OUT0 *classifyOut
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
VOID NTAPI AtfFlowDeleteFunctionHandler(
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
    const CALLOUT_DESC *calloutDesc
);

//
// The callout descriptor layers. For each to be enabled, the user-mode ini must have each set to true.
//  wfp.c will then query filter.c for the aformentioned config, and will initialize the layers that way
// 
// wfp.c cannot initialize WFP if there is no config supplied by the user, the WFP startup will fail.
//
const CALLOUT_DESC descList[] = {
    // TCP Inbound v4
    {
        &FWPM_LAYER_INBOUND_TRANSPORT_V4,

        L"ATF Callout TCP V4 Inbound",
        L"ATF Callout Transport ipv4 Inbound",

        L"ATF Filter TCP V4 Inbound",
        L"ATF Filter Transport ipv4 Inbound",

        AtfClassifyFuncTcpV4Inbound
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

        AtfClassifyFuncTcpV4Outbound
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

    // Datagram v4 (bidirectional)
    {
        &FWPM_LAYER_DATAGRAM_DATA_V4,

        L"ATF Callout UDP V4 Bidirectional",
        L"ATF Callout Datagram ipv4 Bidirectional",

        L"ATF Filter UDP V4 Bidirectional",
        L"ATF Filter Datagram ipv4 Bidirectional",

        AtfClassifyFuncDatagram
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

//
// Structure that contains the callout, filter and layer object once instantiated
//  Used for maintaining and uninitializing the layers at shutdown (or modifying them)
//
#define CALLOUT_LAYER_DATA_MAGIC            0x45132341
#pragma pack(push, 1)
typedef struct _callout_layer_descriptor {
    // Indicates an initialized struct. Must be equal to CALLOUT_LAYER_DATA_MAGIC
    //  Note: these objects are stored in the global stack
    UINT32                                  magic;

    // Is the layer active/enabled from the usermode config?
    BOOL                                    isLayerActive;

    // Layer descriptor
    CALLOUT_DESC                            *desc; // Does not need to be freed -- is const struct descList

    // WFP filter and callout IDs
    UINT64                                  fwpmFilterId;
    UINT32                                  fwpsCalloutId;
    UINT32                                  fwpmCalloutId;
} CALLOUT_LAYER_DESCRIPTOR, *PCALLOUT_LAYER_DESCRIPTOR;
#pragma pack(pop)

//
// Layer descriptors (see fwpmk.h for layer descriptors)
//
static CALLOUT_LAYER_DESCRIPTOR calloutData[MAX_CALLOUT_LAYER_DATA];

//
// Handles to WFP
//
static HANDLE kmfeHandle = 0; //Kernel Mode Filter Engine (KMFE)
static DEVICE_OBJECT *atfDevice = NULL;

NTSTATUS InitializeWfp(
    _In_ DEVICE_OBJECT *deviceObj 
)
{
    ATF_ASSERT(deviceObj);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (kmfeHandle) {
        return STATUS_ALREADY_INITIALIZED;
    }

    // WFP initialization must be at PASSIVE_LEVEL
    if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
        return STATUS_INVALID_DEVICE_STATE;
    }

    ATF_DEBUG(InitializeWfp, "Starting WFP...");    

    RtlZeroMemory(&calloutData, MAX_CALLOUT_LAYER_DATA * sizeof(CALLOUT_LAYER_DESCRIPTOR));

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
        if (!AtfFilterIsLayerEnabled(descList[currLayer].guid)) {
            ATF_DEBUGA("WFP Filter Layer Disabled: %ws", descList[currLayer].calloutName);
            continue;
        }

        ntStatus = AtfAddCalloutLayer(&descList[currLayer]);
        if (!NT_SUCCESS(ntStatus)) {
            ATF_ERROR(AtfAddCalloutLayer, ntStatus);
            return ntStatus;
        }
        ATF_DEBUGA("WFP Filter Layer Enabled: %ws", descList[currLayer].calloutName);
    }

    ATF_DEBUG(InitializeWfp, "WFP Startup Successful!");
    return ntStatus;
}

static NTSTATUS AtfAddCalloutLayer(
    const CALLOUT_DESC *calloutDesc
)
{ 
    if (!atfDevice) {
        return STATUS_APP_INIT_FAILURE;
    }

    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = FwpmTransactionBegin(kmfeHandle, 0);
    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    //
    // Callout registration
    //
    FWPS_CALLOUT fwpsCallout = { 0 };
    fwpsCallout.classifyFn = calloutDesc->callback;
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

    //
    // Add a callout object
    //
    FWPM_CALLOUT fwpmCallout = { 0 };
    fwpmCallout.calloutKey = fwpsCallout.calloutKey;
    fwpmCallout.displayData.name = (wchar_t *)calloutDesc->calloutName;
    fwpmCallout.displayData.description = (wchar_t *)calloutDesc->calloutDesc;
    fwpmCallout.providerKey = (GUID*)&ATF_FWPM_PROVIDER_KEY;
    fwpmCallout.applicableLayer = *calloutDesc->guid;

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

    //
    // Add a filter key
    //
    FWPM_FILTER fwpmFilter = { 0 };
    fwpmFilter.displayData.name = (wchar_t *)calloutDesc->filterName;
    fwpmFilter.displayData.description = (wchar_t *)calloutDesc->filterDesc;
    fwpmFilter.layerKey = *calloutDesc->guid;
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

    ntStatus = FwpmTransactionCommit(kmfeHandle);
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(FwpmTransactionCommit, ntStatus);
        return ntStatus;
    }

    //
    // Commit layer IDs to calloutData
    //
    CALLOUT_LAYER_DESCRIPTOR *descPtr = calloutData;
    while (descPtr->magic == CALLOUT_LAYER_DATA_MAGIC) descPtr++;

    descPtr->magic = CALLOUT_LAYER_DATA_MAGIC;
    descPtr->isLayerActive = TRUE;
    descPtr->desc = (CALLOUT_DESC *)calloutDesc;
    descPtr->fwpsCalloutId = fwpsCalloutId;
    descPtr->fwpmCalloutId = fwpmCalloutId;
    descPtr->fwpmFilterId = fwpmFilterId;

    ATF_DEBUGD(FwpmTransactionCommit, fwpmCalloutId);

    //TODO implement proper cleanup: FwpmTransactionAbort
    return ntStatus;
}

NTSTATUS DestroyWfp(
    _In_ DEVICE_OBJECT *deviceObject
)
{
    UNREFERENCED_PARAMETER(deviceObject);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!atfDevice || !kmfeHandle) {
        return STATUS_UNSUCCESSFUL;
    }

    //DbgBreakPoint();

    // WFP initialization must be at PASSIVE_LEVEL
    if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
        return STATUS_INVALID_DEVICE_STATE;
    }

    //
    // Iterate through calloutData and destroy each layer
    //
    CALLOUT_LAYER_DESCRIPTOR *layerDesc = calloutData;
    while (layerDesc->magic != CALLOUT_LAYER_DATA_MAGIC) {
        ntStatus = FwpmFilterDeleteById(
            kmfeHandle,
            layerDesc->fwpmFilterId
        );
        if (!NT_SUCCESS(ntStatus)) {
            return ntStatus;
        }

        ntStatus = FwpmCalloutDeleteById(
            kmfeHandle,
            layerDesc->fwpmCalloutId
        );
        if (!NT_SUCCESS(ntStatus)) {
            return ntStatus;
        }

        ntStatus = FwpsCalloutUnregisterById(
            layerDesc->fwpsCalloutId
        );
        if (!NT_SUCCESS(ntStatus)) {
            return ntStatus;
        }

        ATF_DEBUG(FwpsCalloutUnregisterById, "Successfully deleted callout layer");

        layerDesc++;
    }

    ntStatus = FwpmTransactionCommit(kmfeHandle);
    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    FwpmProviderDeleteByKey(kmfeHandle, &ATF_FWPM_PROVIDER_KEY);
    FwpmEngineClose(kmfeHandle);
    kmfeHandle = 0; // Signal the IOCTLs that WFP is not running

    // Cleanup
    RtlZeroMemory(calloutData, MAX_CALLOUT_LAYER_DATA * sizeof(CALLOUT_LAYER_DESCRIPTOR));

    return STATUS_SUCCESS;
}

//
// Return a TRUE if WFP is initialized
//
BOOLEAN IsWfpRunning(VOID)
{
    if (kmfeHandle != 0) {
        return TRUE;
    }

    return FALSE;
}

//
// Calls directly into the filter engine (AtfFilterCallbackTcpIpv4Inbound/AtfFilterCallbackTcpIpv4Inbound)
//
VOID NTAPI AtfClassifyFuncTcpV4Inbound(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ VOID *layerData,
    _In_opt_    const VOID *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flowContext,
    _Inout_     FWPS_CLASSIFY_OUT0 *classifyOut
)
{
    UNREFERENCED_PARAMETER(fixedValues);
    UNREFERENCED_PARAMETER(metaValues);
    UNREFERENCED_PARAMETER(layerData);
    UNREFERENCED_PARAMETER(classifyContext);
    UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(flowContext);

    AtfFilterCallbackIpv4(
        fixedValues,
        classifyOut,
        _flow_direction_inbound
    );
}

VOID NTAPI AtfClassifyFuncTcpV4Outbound(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ VOID *layerData,
    _In_opt_    const VOID *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flowContext,
    _Inout_     FWPS_CLASSIFY_OUT0 *classifyOut
)
{
    UNREFERENCED_PARAMETER(fixedValues);
    UNREFERENCED_PARAMETER(metaValues);
    UNREFERENCED_PARAMETER(layerData);
    UNREFERENCED_PARAMETER(classifyContext);
    UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(flowContext);

    AtfFilterCallbackIpv4(
        fixedValues,
        classifyOut,
        _flow_direction_outbound
    );  
}

VOID NTAPI AtfClassifyFuncDatagram(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ VOID *layerData,
    _In_opt_    const VOID *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flowContext,
    _Inout_     FWPS_CLASSIFY_OUT0 *classifyOut
)
{
    UNREFERENCED_PARAMETER(fixedValues);
    UNREFERENCED_PARAMETER(metaValues);
    UNREFERENCED_PARAMETER(layerData);
    UNREFERENCED_PARAMETER(classifyContext);
    UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(flowContext);

    AtfFilterCallbackIpv4(
        fixedValues,
        classifyOut,
        _flow_direction_outbound
    );  
}

VOID NTAPI AtfClassifyFuncTcpV6(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ VOID *layerData,
    _In_opt_    const VOID *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flowContext,
    _Inout_     FWPS_CLASSIFY_OUT0 *classifyOut
)
{
    ATF_DEBUG(AtfClassifyFuncTcpV6, "Entered WFP callout: TCP ipv6");

    UNREFERENCED_PARAMETER(fixedValues);
    UNREFERENCED_PARAMETER(metaValues);
    UNREFERENCED_PARAMETER(layerData);
    UNREFERENCED_PARAMETER(classifyContext);
    UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(fixedValues);
    UNREFERENCED_PARAMETER(flowContext);
    UNREFERENCED_PARAMETER(classifyOut);

    return;
}

VOID NTAPI AtfClassifyFuncIcmp(
    _In_        const FWPS_INCOMING_VALUES0 *fixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0 *metaValues,
    _Inout_opt_ VOID *layerData,
    _In_opt_    const VOID *classifyContext,
    _In_        const FWPS_FILTER3 *filter,
    _In_        UINT64 flowContext,
    _Inout_     FWPS_CLASSIFY_OUT0 *classifyOut
)
{
    ATF_DEBUG(AtfClassifyFuncIcmp, "Entered WFP callout: ICMP");

    UNREFERENCED_PARAMETER(fixedValues);
    UNREFERENCED_PARAMETER(metaValues);
    UNREFERENCED_PARAMETER(layerData);
    UNREFERENCED_PARAMETER(classifyContext);
    UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(fixedValues);
    UNREFERENCED_PARAMETER(flowContext);
    UNREFERENCED_PARAMETER(classifyOut);

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
            ATF_DEBUGD(AtfNotifyFunctionHandler, notifyType);
            break;
        }
    case FWPS_CALLOUT_NOTIFY_DELETE_FILTER:
        {
            ATF_DEBUGD(AtfNotifyFunctionHandler, notifyType);
            break;
        }
    default:
        {
            ATF_DEBUG(AtfNotifyFunctionHandler, "Unknown type");
            break;
        }
    }

    return STATUS_SUCCESS;
}

VOID NTAPI AtfFlowDeleteFunctionHandler(
    _In_ UINT16 layerId,
    _In_ UINT32 calloutId,
    _In_ UINT64 flowContext
)
{
    UNREFERENCED_PARAMETER(layerId);
    UNREFERENCED_PARAMETER(calloutId);
    UNREFERENCED_PARAMETER(flowContext);

    //ATF_DEBUGAF("Received layerId: 0x%08x", layerId);

    return;
}