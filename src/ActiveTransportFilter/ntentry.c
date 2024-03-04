#include <ntddk.h>
#include <wdf.h>
//#include <initguid.h>

#include "trace.h"
#include "../common/common.h"

// Structure for initializing NT entry
typedef struct atf_nt_config {
    WDF_DRIVER_CONFIG               wdfDriverConfig;
    WDF_OBJECT_ATTRIBUTES           wdfObjectAttributes;

    UNICODE_STRING                  deviceName;
    UNICODE_STRING                  dosDeviceName;
} ATF_NT_CONFIG, *PATF_NT_CONFIG;

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD AtfDriverDeviceAdd;
EVT_WDF_DRIVER_UNLOAD UnloadDriver;

NTSTATUS DriverEntry(
    _In_ DRIVER_OBJECT *driverObj,
    _In_ UNICODE_STRING *registryPath
);

NTSTATUS AtfDriverUnload(
    _In_ DRIVER_OBJECT *driverObj
);


static NTSTATUS AtfInitDevice(
    _In_ ATF_NT_CONFIG *config,
    _In_ DRIVER_OBJECT *driverObj,
    _In_ UNICODE_STRING *registryPath,
    _Out_ DEVICE_OBJECT **deviceObj
);

static void AtfInitConfig(
    _Inout_ ATF_NT_CONFIG *config
);

static NTSTATUS AtfDriverSetCallbacks(
    _In_ DRIVER_OBJECT *driverObj
);


_Use_decl_annotations_
NTSTATUS DriverEntry(
    _In_ DRIVER_OBJECT *driverObj,
    _In_ UNICODE_STRING *registryPath
)
{
    NTSTATUS ntStatus = -1;
    DbgPrint("Entering ATF");
    ATF_DEBUG(DriverEntry, "Entering ActiveTransportFilter");


    ATF_ASSERT(driverObj);
    ATF_ASSERT(registryPath);    

    // Initialize config ONLY, i.e. strings, callbacks and other constants for the device
    //  Keeping scope local to ntentry
    ATF_NT_CONFIG atfConfig = { 0 };
    AtfInitConfig(&atfConfig);

    // Initialize the device, using above config
    DEVICE_OBJECT *deviceObj = NULL;
    ntStatus = AtfInitDevice(&atfConfig, driverObj, registryPath, &deviceObj);
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(AtfInitDevice, ntStatus);
        return ntStatus;
    }

    ATF_DEBUG(DriverEntry, "Successful initialization");

    return STATUS_SUCCESS;
}

NTSTATUS AtfDriverDeviceAdd(
    _In_ WDFDRIVER wdfDriver,
    _Inout_ PWDFDEVICE_INIT deviceInit)
{
    UNREFERENCED_PARAMETER(wdfDriver);
    UNREFERENCED_PARAMETER(deviceInit);

    DbgPrint("Entering Device Add");

    return STATUS_SUCCESS;
}

static void AtfInitConfig(
    _Inout_ ATF_NT_CONFIG *config
)
{
    ATF_ASSERT_NORETURN(config);
    RtlZeroBytes(config, sizeof(ATF_NT_CONFIG));

    WDF_OBJECT_ATTRIBUTES_INIT(&config->wdfObjectAttributes);
    config->wdfObjectAttributes.EvtCleanupCallback = NULL;
    config->wdfObjectAttributes.EvtDestroyCallback = NULL; //todo?

    WDF_DRIVER_CONFIG_INIT(&config->wdfDriverConfig, NULL);
    config->wdfDriverConfig.EvtDriverUnload = (PFN_WDF_DRIVER_UNLOAD)AtfDriverUnload;

    RtlInitUnicodeString(&config->deviceName, TEXT(ATF_DEVICE_NAME));
    RtlInitUnicodeString(&config->dosDeviceName, TEXT(ATF_DOS_DEVICE_NAME));
}

static NTSTATUS AtfInitDevice(
    _In_ ATF_NT_CONFIG *config,
    _In_ DRIVER_OBJECT *driverObj,
    _In_ UNICODE_STRING *registryPath,
    _Out_ DEVICE_OBJECT **deviceObj
)
{
    ATF_ASSERT(config);
    ATF_ASSERT(driverObj);
    ATF_ASSERT(registryPath);
    ATF_ASSERT(deviceObj);

    *deviceObj = NULL;

    WDFDRIVER wdfDriver = NULL; // Declaration for a HANDLE-type

    NTSTATUS ntStatus = WdfDriverCreate(
        driverObj,
        registryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config->wdfDriverConfig,
        &wdfDriver
    );
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(WdfDriverCreate, ntStatus);
        return ntStatus;
    }

    UNICODE_STRING sddlString = { 0 };
    RtlInitUnicodeString(&sddlString, TEXT(SDDL_STRING)); //TODO

    PWDFDEVICE_INIT deviceInit = NULL; // This does not have to be free'd, unless error on device init
    deviceInit = WdfControlDeviceInitAllocate(
        wdfDriver, 
        &sddlString
    );
    if (!deviceInit) {
        ATF_ERROR(WdfControlDeviceInitAllocate, STATUS_INSUFFICIENT_RESOURCES);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ntStatus = WdfDeviceInitAssignName(deviceInit, &config->deviceName);
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(WdfDeviceInitAssignName, ntStatus);
        WdfDeviceInitFree(deviceInit);
        return ntStatus;
    }

    WDFDEVICE wdfDevice = NULL;
    ntStatus = WdfDeviceCreate(&deviceInit, &config->wdfObjectAttributes, &wdfDevice);
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(WdfDeviceCreate, ntStatus);
        WdfDeviceInitFree(deviceInit);
        return ntStatus;
    }

    ntStatus = IoCreateSymbolicLink(&config->dosDeviceName, &config->deviceName);
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(IoCreateSymbolicLink, ntStatus);
        WdfDeviceInitFree(deviceInit);
        return ntStatus;
    }

    *deviceObj = WdfDeviceWdmGetDeviceObject(wdfDevice);

    ATF_DEBUG(AtfInitDevice, "Successfully created device");
    return STATUS_SUCCESS;
}

static NTSTATUS AtfDriverSetCallbacks(
    _In_ DRIVER_OBJECT *driverObj
)
{
    UNREFERENCED_PARAMETER(driverObj);

    return STATUS_SUCCESS;
}

NTSTATUS AtfDriverUnload(
    _In_ DRIVER_OBJECT *driverObj
)
{
    UNREFERENCED_PARAMETER(driverObj);

    return STATUS_SUCCESS;
}