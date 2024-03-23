#include <ntddk.h>
#include <wdf.h>

#include "ntentry.h"
#include "ioctl.h"
#include "trace.h"
#include "wfp.h"
#include "filter.h"
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
// Success on DriverEntry()
//
static BOOLEAN isWfpRunning = FALSE;

//
// Main DEVICE_OBJECT pointer -- for only one device
//
static DEVICE_OBJECT *gDeviceObj = NULL;

//
// WDF Callbacks
//
EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD AtfUnloadDriver;

EXTERN_C_END

// https://learn.microsoft.com/en-us/cpp/preprocessor/alloc-text?view=msvc-170
// remove?
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, AtfUnloadDriver)

//
// Initialize driver callbacks, strings, and constants
//
static VOID AtfInitConfig(
    _Inout_ ATF_CONFIG *config
);

//
// Create the physical device object (PDO)
//
static NTSTATUS AtfCreateDeviceObject(
    _In_    DRIVER_OBJECT *driverObj,
    _In_    UNICODE_STRING *registryPath,
    _Out_   DEVICE_OBJECT **deviceObjOut,
    _Out_   WDFDEVICE *wdfDevice
);

_Function_class_(DRIVER_INITIALIZE)
_IRQL_requires_same_
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
    // Initialize the base filter engines
    //
    AtfFilterInit();

    //
    // Create the driver/device object
    //
    WDFDEVICE wdfDevice = NULL;
    ntStatus = AtfCreateDeviceObject(
        driverObj,
        registryPath,
        &gDeviceObj,
        &wdfDevice
    );
    if (!NT_SUCCESS(ntStatus) || !gDeviceObj) {
        ATF_ERROR(AtfCreateDeviceObject, ntStatus);
        return ntStatus;
    }

    //
    // Initialize usermode IOCTL handler
    //
    ntStatus = AtfInitializeIoctlHandlers(
        wdfDevice
    );
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(AtfInitializeIoctlHandlers, ntStatus);
        return ntStatus;
    }

    ATF_DEBUG(AtfInitializeIoctlHandlers, "Successfully created IOCTL handlers");



    ATF_DEBUG(WdfDriverCreate, "Successfully created device driver object");
    isWfpRunning = TRUE;

    return ntStatus;
}

static NTSTATUS AtfCreateDeviceObject(
    _In_    DRIVER_OBJECT *driverObj,
    _In_    UNICODE_STRING *registryPath,
    _Out_   DEVICE_OBJECT **deviceObjOut,
    _Out_   WDFDEVICE *wdfDevice
)
{
    ATF_ASSERT(driverObj);
    ATF_ASSERT(registryPath);
    ATF_ASSERT(deviceObjOut);
    ATF_ASSERT(wdfDevice);

    *deviceObjOut = NULL;
    NTSTATUS ntStatus = -1;

    WDFDRIVER wdfDriver;
    ntStatus = WdfDriverCreate(
        driverObj,
        registryPath,
        &atfConfig.wdfObjectAttributes,
        &atfConfig.wdfDriverConfig,
        &wdfDriver
    );
    if (!NT_SUCCESS(ntStatus)) {
        ATF_ERROR(WdfDriverCreate, ntStatus);
        return ntStatus;
    }

    ATF_DEBUG(WdfDriverCreate, "Driver object created");

    UNICODE_STRING sddlString = { 0 };
    RtlInitUnicodeString(&sddlString, TEXT(SDDL_STRING)); //TODO

    PWDFDEVICE_INIT deviceInit = WdfControlDeviceInitAllocate(
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

    ntStatus = WdfDeviceCreate(&deviceInit, &atfConfig.wdfObjectAttributes, wdfDevice);
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

    *deviceObjOut = WdfDeviceWdmGetDeviceObject(*wdfDevice);

    ATF_DEBUG(AtfInitDevice, "Successfully created device object");
    return STATUS_SUCCESS;
}

VOID AtfUnloadDriver(
    _In_ WDFDRIVER driverObj
)
{
    ATF_DEBUG(AtfUnloadDriver, "Entering AtfUnloadDriver");

    UNREFERENCED_PARAMETER(driverObj);

    if (isWfpRunning) {
        DestroyWfp(gDeviceObj);
    }

    AtfFilterCleanup();
}

static VOID AtfInitConfig(
    _Inout_ ATF_CONFIG *config
)
{
    ATF_DEBUG(AtfInitConfig, "Entering AtfInitConfig");

    ATF_ASSERT_NORETURN(config);
    RtlZeroBytes(config, sizeof(ATF_CONFIG));

    WDF_OBJECT_ATTRIBUTES_INIT(&config->wdfObjectAttributes);
    config->wdfObjectAttributes.EvtCleanupCallback = NULL;
    config->wdfObjectAttributes.EvtDestroyCallback = NULL; //todo?

    //WDF_DRIVER_CONFIG_INIT(&config->wdfDriverConfig, AtfEvtWdfDriverDeviceAdd);
    WDF_DRIVER_CONFIG_INIT(&config->wdfDriverConfig, NULL);
    config->wdfDriverConfig.EvtDriverUnload = (PFN_WDF_DRIVER_UNLOAD)AtfUnloadDriver;

    //
    // Important note: since this is not a PnP driver (as it is WFP), it does not have a device that will
    //  call the deviceAdd callback when a device is added. Instead, set the NonPnpDriver flag, disable
    //  the EvtDriverDeviceAdd callback, and instantiate WFP in DriverEntry()
    // 
    // https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/using-kernel-mode-driver-framework-with-non-pnp-drivers
    //
    //config->wdfDriverConfig.EvtDriverDeviceAdd = (PFN_WDF_DRIVER_DEVICE_ADD)AtfEvtWdfDriverDeviceAdd;
    config->wdfDriverConfig.DriverInitFlags |= WdfDriverInitNonPnpDriver;

    RtlInitUnicodeString(&config->deviceName, L"\\Device\\" TEXT(ATF_DRIVER_NAME));
    RtlInitUnicodeString(&config->dosDeviceName, L"\\DosDevices\\" TEXT(ATF_DRIVER_NAME));
}

