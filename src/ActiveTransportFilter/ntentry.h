#pragma once

#include <ntddk.h>
#include <wdf.h>

//
// Used in this driver to indicate that the function is a callback from the NT kernel
//
#define __NT_CALLBACK__

//
// DriverEntry:
//  Main driver entry point
//
__NT_CALLBACK__
NTSTATUS DriverEntry(
    _In_ DRIVER_OBJECT *driverObj,
    _In_ UNICODE_STRING *registryPath
);

//
// AtfDriverDeviceAdd
//  Callback for device add
//  Initialize device and config structures
//
__NT_CALLBACK__
NTSTATUS AtfDriverDeviceAdd(
    _In_ WDFDRIVER wdfDriver,
    _Inout_ PWDFDEVICE_INIT deviceInit
);

//
// AtfDriverUnload
//  Unloads the driver, free's WFP
//
__NT_CALLBACK__
NTSTATUS AtfDriverUnload(
    _In_ WDFDRIVER driverObj
);
