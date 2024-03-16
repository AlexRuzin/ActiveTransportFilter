#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

//
// Initialize the usermode callback handler function through WDF
//
NTSTATUS AtfInitializeIoctlHandlers(WDFDEVICE wdfDevice);
