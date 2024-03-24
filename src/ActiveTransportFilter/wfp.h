#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

#if !defined(INITGUID)
#define INITGUID
#endif //INITGUID

#include <initguid.h>

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
    _In_ DEVICE_OBJECT *deviceObject
);

//
// Return a TRUE if WFP is initialized
//
BOOLEAN IsWfpRunning(VOID);

// EOF