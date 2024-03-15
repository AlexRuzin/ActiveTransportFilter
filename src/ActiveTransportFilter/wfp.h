#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

//
// Initialize WFP subsystem
//
NTSTATUS InitializeWfp(
    _In_ DEVICE_OBJECT *deviceObj
);

//
// Cleanup
//
NTSTATUS DestroyWfp(
    VOID
);
