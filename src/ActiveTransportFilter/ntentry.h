#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

//
// Used in this driver to indicate that the function is a callback from the NT kernel
//
#define __NT_CALLBACK__

//
// DriverEntry:
//  Main driver entry point
//
_Use_decl_annotations_ 
NTSTATUS DriverEntry(
    _In_ DRIVER_OBJECT *driverObj,
    _In_ UNICODE_STRING *registryPath
);

//
// AtfDriverUnload
//  Unloads the driver, free's WFP
//
VOID AtfUnloadDriver(
    _In_ WDFDRIVER driverObj
);
