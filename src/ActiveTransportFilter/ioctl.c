#include <ntddk.h>
#include <wdf.h>

#include "ioctl.h"
#include "trace.h"
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

static NTSTATUS AtfHandleSendWfpConfig(
    _In_ WDFREQUEST request, 
    _In_ size_t bufLen
);

NTSTATUS AtfInitializeIoctlHandlers(WDFDEVICE wdfDevice)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
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
    UNREFERENCED_PARAMETER(queue);

    NTSTATUS ntStatus = STATUS_SUCCESS;
    
    switch (ioControlCode)
    {
    case IOCTL_ATF_SEND_KEEPALIVE:
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

    WdfRequestComplete(request, ntStatus);
}

static NTSTATUS AtfHandleSendWfpConfig(
    _In_ WDFREQUEST request, 
    _In_ size_t bufLen
)
{
    UNREFERENCED_PARAMETER(request);
    UNREFERENCED_PARAMETER(bufLen);

    return STATUS_SUCCESS;
}