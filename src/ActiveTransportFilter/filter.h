#if _MSC_VER > 1000
#pragma once
#endif //_MSC_VER > 1000

#include <ntddk.h>

#include "../common/errors.h"

#include "config.h"

//
// Initialize the filter engine
//
VOID AtfFilterInit(VOID);

//
// Destroy the filter engine and cleanup
//
VOID AtfFilterCleanup(VOID);

//
// Flush the current config entirely
//
VOID AtfFilterFlushConfig(VOID);

//
// Store the current default ini configuration into the filter engine
//
VOID AtfFilterStoreDefaultConfig(const CONFIG_CTX *confgCtx);




