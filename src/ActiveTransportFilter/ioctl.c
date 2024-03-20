#include <ntddk.h>
#include <wdf.h>

#include "ioctl.h"
#include "trace.h"
#include "wfp.h"
#include "config.h"
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

static NTSTATUS AtfHandleSendWfpConfig(
    _In_ WDFREQUEST request, 
    _In_ size_t bufLen
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

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

    ATF_DEBUG(AtfHandleStartWFP, "Sucessfully processed config ini!");
    return ntStatus;
}
