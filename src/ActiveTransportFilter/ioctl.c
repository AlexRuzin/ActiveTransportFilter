#include <ntddk.h>
#include <wdf.h>

#include "ioctl.h"
#include "trace.h"
#include "wfp.h"
#include "config.h"
#include "filter.h"
#include "../common/errors.h"
#include "../common/ioctl_codes.h"
#include "../common/user_driver_transport.h"

//
// DeviceIoControl handler
//
VOID AtfIoDeviceControl(
    _In_ WDFQUEUE queue,
    _In_ WDFREQUEST request,
    _In_ size_t outputBufferLength,
    _In_ size_t inputBufferLength,
    _In_ ULONG ioControlCode
);

//
// Function handles the start WFP command
//  IOCTL_ATF_WFP_SERVICE_START
//
static NTSTATUS AtfHandleStartWFP(
    _In_ DEVICE_OBJECT *deviceObj
);

//
// Function to handle the WFP stop command
//  IOCTL_ATF_WFP_SERVICE_STOP
//
static NTSTATUS AtfHandleStopWFP(
    _In_ DEVICE_OBJECT *deviceObj
);

//
// Function handles the ini configuration loading
//  IOCTL_ATF_SEND_WFP_CONFIG
//
static NTSTATUS AtfHandleSendWfpConfig(
    _In_ WDFREQUEST request, 
    _In_ size_t bufLen
);

//
// Handler for flush config
//  Note, for this call to succeed, WFP must first be shut down (i.e. call to ATFHandleStopWFP())
//
static NTSTATUS AtfHandleFlushConfig(
    _In_ DEVICE_OBJECT *deviceObj
);

//
// Handler to append blacklist IPs
//
static NTSTATUS AtfHandlerAppendIpv4Blacklist(
    _In_ WDFREQUEST request, 
    _In_ size_t bufLen
);

//
// Lock that handles synchronization between IOCTL calls
//
KMUTEX gIoctlLock;

NTSTATUS AtfInitializeIoctlHandlers(
    WDFDEVICE wdfDevice
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    KeInitializeMutex(&gIoctlLock, 0);

    WDF_IO_QUEUE_CONFIG ioQueueConfig;
    WDFQUEUE wdfQueue;

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);

    ioQueueConfig.EvtIoDeviceControl = AtfIoDeviceControl;

    ntStatus = WdfIoQueueCreate(
        wdfDevice,
        &ioQueueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &wdfQueue
    );
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(WdfIoQueueCreate, ntStatus);
        return ntStatus;
    }

    return STATUS_SUCCESS;
}

VOID AtfIoDeviceControl(
    _In_ WDFQUEUE queue,
    _In_ WDFREQUEST request,
    _In_ size_t outputBufferLength,
    _In_ size_t inputBufferLength,
    _In_ ULONG ioControlCode
)
{
    UNREFERENCED_PARAMETER(outputBufferLength);

    //
    // Dev note: using a KMUTEX since it does not change IRQL from PASSIVE_LEVEL
    //  Initially, I used a spinlock but this caused FwpmEngineOpen() to fail since spinlocks
    //  raise the IRQL to APC_LEVEL when acquired, and WFP initialization requries PASSIVE_LEVEL
    // 
    // The reason for this is that a spinlock forces the thread to be raised to APC_LEVEL so it
    //  is not interrupted by PASSIVE_LEVEL threads. The scheduler itself runs at DISPATCH_LEVEL
    //  so it cannot be interrupted by anything below that IRQL, but it does manage all APC_LEVEL
    //  and PASSIVE_LEVEL context switches.
    // 
    // The higher the IRQL of a thread the less likely a thread is to be interrupted. So the 
    //  higher the IRQL the higher the priority (for example hardware devices need high IRQL). 
    //
    KeWaitForSingleObject(&gIoctlLock, Executive, KernelMode, FALSE, NULL);

    //DbgBreakPoint();

    NTSTATUS ntStatus = STATUS_SUCCESS;

    WDFDEVICE wdfDevice = WdfIoQueueGetDevice(queue);
    DEVICE_OBJECT *deviceObject = WdfDeviceWdmGetDeviceObject(wdfDevice);

    ATF_DEBUGD(AtfIoDeviceControl, ioControlCode);
    
    switch (ioControlCode)
    {
    case IOCTL_ATF_SEND_KEEPALIVE:
        {
    
        }
        break;
    case IOCTL_ATF_WFP_SERVICE_START:
        {
            ntStatus = AtfHandleStartWFP(
                deviceObject
            );
        }
        break;

    case IOCTL_ATF_WFP_SERVICE_STOP:
        {
            ntStatus = AtfHandleStopWFP(
                deviceObject
            );
        }
        break;
    case IOCTL_ATF_FLUSH_CONFIG:
        {
            ntStatus = AtfHandleFlushConfig(
                deviceObject
            );
        }
        break;
    case IOCTL_ATF_SEND_WFP_CONFIG:
        {
            ntStatus = AtfHandleSendWfpConfig(
                request,
                inputBufferLength
            );
        }
        break;

    case IOCTL_ATF_APPEND_IPV4_BLACKLIST:
        {
            ntStatus = AtfHandlerAppendIpv4Blacklist(
                request,
                inputBufferLength
            );
        }
        break;
    default:
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(AtfIoDeviceControl, ntStatus);
    } else {
        ATF_DEBUG(AtfIoDeviceControl, "IOCTL Successfully processed");
    }

    KeReleaseMutex(&gIoctlLock, FALSE);

    WdfRequestComplete(request, ntStatus);
}

static NTSTATUS AtfHandleStartWFP(
    _In_ DEVICE_OBJECT *deviceObj
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // WFP cannot be already running
    //
    if (IsWfpRunning()) {
        ATF_ERROR(IsWfpRunning, STATUS_DEVICE_BUSY);
        return STATUS_DEVICE_BUSY;
    }

    //
    // Verify that the filter is ready (i.e. a default config was received)
    //
    if (!AtfFilterIsInitialized()) {
        ATF_ERROR(AtfFilterIsInitialized, STATUS_DEVICE_NOT_READY);
        return STATUS_DEVICE_NOT_READY;
    }

    //
    // Initialize the WFP subsystem, but do not populate callouts yet
    //
    ntStatus = InitializeWfp(deviceObj);
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(WdfDriverCreate, ntStatus);
        return ntStatus;
    }

    ATF_DEBUG(AtfHandleStartWFP, "Sucessfully started AFP filters");
    return ntStatus;
}

static NTSTATUS AtfHandleStopWFP(
    _In_ DEVICE_OBJECT *deviceObj
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!IsWfpRunning()) {
        ATF_ERROR(IsWfpRunning, STATUS_UNSUCCESSFUL);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Remove all callouts and filters
    //
    ntStatus = DestroyWfp(deviceObj);
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(WdfDriverCreate, ntStatus);
        return ntStatus;
    }

    ATF_DEBUG(AtfHandleStartWFP, "Sucessfully stopped AFP filters");
    return ntStatus;
}

static NTSTATUS AtfHandleFlushConfig(
    _In_ DEVICE_OBJECT *deviceObj
)
{
    UNREFERENCED_PARAMETER(deviceObj);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (IsWfpRunning()) {
        // WFP cannot be running for a filter config to be flushed. AtfHandleStopWFP() must be called first
        ATF_ERROR(AtfHandleFlushConfig, STATUS_DEVICE_BUSY);
        return STATUS_DEVICE_BUSY;
    }

    //
    // Verify that there is a config to flush
    //
    if (!AtfFilterIsInitialized()) {
        ATF_ERROR(AtfFilterIsInitialized, STATUS_DEVICE_NOT_READY);
        return STATUS_DEVICE_NOT_READY;
    }

    AtfFilterFlushConfig();
    
    ATF_DEBUG(AtfFilterFlushConfig, "Successfully flushed filter config");
    return ntStatus;
}

//
// Important note: the WFP subsystem cannot be running while appending blacklist IPs.
//  The service must be stopped IOCTL_ATF_WFP_SERVICE_STOP, then appended (IOCTL_ATF_APPEND_IPV4_BLACKLIST), 
//  then restarted by calling IOCTL_ATF_WFP_SERVICE_START
// 
// Note: The filter.c engine will automatically flush its config if a new config is received
//
static NTSTATUS AtfHandleSendWfpConfig(
    _In_ WDFREQUEST request, 
    _In_ size_t bufLen
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (IsWfpRunning()) {
        // WFP cannot be running when sending the config
        ATF_ERROR(IsWfpRunning, STATUS_DEVICE_BUSY);
        return STATUS_DEVICE_BUSY;
    }

    if (bufLen == 0) {
        return STATUS_NO_DATA_DETECTED;
    }

    if (bufLen != sizeof(USER_DRIVER_FILTER_TRANSPORT_DATA)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    USER_DRIVER_FILTER_TRANSPORT_DATA *data = NULL;

    ntStatus = WdfRequestRetrieveInputBuffer(
        request,
        sizeof(USER_DRIVER_FILTER_TRANSPORT_DATA),
        (PVOID *)&data,
        NULL
    );
    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    //
    // Parse the default ini config into a CONFIG_CTX object
    //  This function will also validate the user input
    //
    CONFIG_CTX *configCtx = NULL;
    ATF_ERROR atfError = AtfAllocDefaultConfig(data, &configCtx);
    if (atfError) {
        ATF_ERROR(AtfAllocDefaultConfig, atfError);
        return STATUS_BAD_DATA; 
    }

    // Store the default config to filter.c, which takes CONFIG_CTX and the WFP callback as 
    //  inputs. The filter does not need a filter state of its own.
    //
    // If a default config already exists, we can override it, so the filter engine
    //  will destroy the configCtx and load the new one as supplied by the user
    AtfFilterStoreDefaultConfig(configCtx);

    ATF_DEBUG(AtfHandleSendWfpConfig, "Sucessfully processed config ini!");
    return ntStatus;
}

static NTSTATUS AtfHandlerAppendIpv4Blacklist(
    _In_ WDFREQUEST request,  
    _In_ size_t bufLen
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (IsWfpRunning()) {
        // WFP cannot be running when sending the blacklist
        ATF_ERROR(IsWfpRunning, STATUS_DEVICE_BUSY);
        return STATUS_DEVICE_BUSY;
    }

    //
    // There must exist at least a default config (IOCTL_ATF_SEND_WFP_CONFIG) before blacklist IPs can be appended
    //
    if (!AtfFilterIsInitialized()) {
        ATF_ERROR(AtfFilterIsInitialized, STATUS_DEVICE_NOT_READY);
        return STATUS_DEVICE_NOT_READY;
    }

    if (bufLen == 0) {
        return STATUS_NO_DATA_DETECTED;
    }

    if (bufLen > BLACKLIST_IPV4_MAX_SIZE) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // config.c will reallocate this memory and store a pointer into the current filter.c's CONFIG_CTX object
    //
    CONFIG_CTX *configCtx = AtfFilterGetCurrentConfig();
    if (!configCtx) {
        ATF_ERROR(AtfFilterGetCurrentConfig, STATUS_DEVICE_NOT_READY);
        return STATUS_DEVICE_NOT_READY;
    }

    VOID *rawBuf = NULL;

    ntStatus = WdfRequestRetrieveInputBuffer(
        request,
        bufLen,
        (PVOID *)&rawBuf,
        NULL
    );
    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    ATF_ERROR atfError = AtfConfigAddIpv4Blacklist(configCtx, rawBuf, bufLen);
    if (atfError) {
        ATF_ERROR(AtfConfigAddIpv4Blacklist, atfError);
        return STATUS_DEVICE_NOT_READY;
    }

    return ntStatus;
}

//EOF