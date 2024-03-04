#include <ntddk.h>
#include <wdf.h>
//#include <initguid.h>

#include "trace.h"
#include "../common/common.h"

// Structure for initializing NT entry
typedef struct _atf_config {
    WDF_DRIVER_CONFIG               wdfDriverConfig;
    WDF_OBJECT_ATTRIBUTES           wdfObjectAttributes;

    UNICODE_STRING                  deviceName;
    UNICODE_STRING                  dosDeviceName;
} ATF_CONFIG, *PATF_CONFIG;

//
// Stores shared configs for WDF
//
ATF_CONFIG atfConfig = { 0 };


//
// WDF Callbacks
//
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD AtfDriverDeviceAdd;
//EVT_WDF_DRIVER_UNLOAD UnloadDriver;


static void AtfInitConfig(
    _Inout_ ATF_CONFIG *config
);

_Use_decl_annotations_
NTSTATUS DriverEntry(
    _In_ DRIVER_OBJECT *driverObj,
    _In_ UNICODE_STRING *registryPath
)
{
    NTSTATUS ntStatus = -1;
    ATF_DEBUG(DriverEntry, "Entering ActiveTransportFilter");


    ATF_ASSERT(driverObj);
    ATF_ASSERT(registryPath);    

    //WPP_INIT_TRACING(driverObj, NULL);

    //
    // Initialize config ONLY, i.e. strings, callbacks and other constants for the device
    //  Keeping scope local to ntentry
    //
    AtfInitConfig(&atfConfig);

    //
    // Create the driver object
    //
    ntStatus = WdfDriverCreate(
        driverObj,
        registryPath,
        &atfConfig.wdfObjectAttributes,
        &atfConfig.wdfDriverConfig,
        WDF_NO_HANDLE
    );
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(WdfDriverCreate, ntStatus);
        return ntStatus;
    }

    ATF_DEBUG(WdfDriverCreate, "Successfully created driver object");

    return ntStatus;
}

NTSTATUS AtfDriverDeviceAdd(
    _In_ WDFDRIVER wdfDriver,
    _Inout_ PWDFDEVICE_INIT deviceInit)
{
    ATF_DEBUG(AtfDriverDeviceAdd, "Entering AtfDriverDeviceAdd");

    NTSTATUS ntStatus = -1;

    UNICODE_STRING sddlString = { 0 };
    RtlInitUnicodeString(&sddlString, TEXT(SDDL_STRING)); //TODO

    deviceInit = WdfControlDeviceInitAllocate(
        wdfDriver, 
        &sddlString
    );
    if (!deviceInit) {
        ATF_ERROR(WdfControlDeviceInitAllocate, STATUS_INSUFFICIENT_RESOURCES);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ntStatus = WdfDeviceInitAssignName(deviceInit, &atfConfig.deviceName);
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(WdfDeviceInitAssignName, ntStatus);
        WdfDeviceInitFree(deviceInit);
        return ntStatus;
    }

    WDFDEVICE wdfDevice = NULL;
    ntStatus = WdfDeviceCreate(&deviceInit, &atfConfig.wdfObjectAttributes, &wdfDevice);
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(WdfDeviceCreate, ntStatus);
        WdfDeviceInitFree(deviceInit);
        return ntStatus;
    }
    ATF_DEBUG(WdfDeviceCreate, "Successfully created device object");

    ntStatus = IoCreateSymbolicLink(&atfConfig.dosDeviceName, &atfConfig.deviceName);
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(IoCreateSymbolicLink, ntStatus);
        WdfDeviceInitFree(deviceInit);
        return ntStatus;
    }

    DEVICE_OBJECT *deviceObj = WdfDeviceWdmGetDeviceObject(wdfDevice);
    UNREFERENCED_PARAMETER(deviceObj);

    ATF_DEBUG(AtfInitDevice, "Successfully created device");
    return STATUS_SUCCESS;
}

NTSTATUS AtfDriverUnload(
    _In_ WDFDRIVER driverObj
)
{
    ATF_DEBUG(AtfDriverUnload, "Entering AtfDriverUnload");

    UNREFERENCED_PARAMETER(driverObj);

    //WPP_CLEANUP(driverObj);

    return STATUS_SUCCESS;
}

static void AtfInitConfig(
    _Inout_ ATF_CONFIG *config
)
{
    ATF_DEBUG(AtfInitConfig, "Entering AtfInitConfig");

    ATF_ASSERT_NORETURN(config);
    RtlZeroBytes(config, sizeof(ATF_CONFIG));

    WDF_OBJECT_ATTRIBUTES_INIT(&config->wdfObjectAttributes);
    config->wdfObjectAttributes.EvtCleanupCallback = NULL;
    config->wdfObjectAttributes.EvtDestroyCallback = NULL; //todo?

    WDF_DRIVER_CONFIG_INIT(&config->wdfDriverConfig, AtfDriverDeviceAdd);
    config->wdfDriverConfig.EvtDriverUnload = (PFN_WDF_DRIVER_UNLOAD)AtfDriverUnload;
    //config->wdfDriverConfig.EvtDriverDeviceAdd = (PFN_WDF_DRIVER_DEVICE_ADD)AtfDriverDeviceAdd;
    //config->wdfDriverConfig.DriverInitFlags |= WdfDriverInitNonPnpDriver;

    RtlInitUnicodeString(&config->deviceName, TEXT(ATF_DEVICE_NAME));
    RtlInitUnicodeString(&config->dosDeviceName, TEXT(ATF_DOS_DEVICE_NAME));
}

