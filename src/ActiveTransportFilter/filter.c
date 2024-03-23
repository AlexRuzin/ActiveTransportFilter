#include <ntddk.h>

#include "filter.h"

#include "../common/errors.h"
#include "trace.h"

#include "config.h"

//
// Current config context structure (may be modified by config.cpp)
//
static CONFIG_CTX* gConfigCtx = NULL;

//
// Filter engine configuration state change lock
//
KMUTEX filterEngineLock;

VOID AtfFilterInit(VOID)
{
    KeInitializeMutex(&filterEngineLock, 0);

    gConfigCtx = NULL;
}

//
// If an existing config exists, then
VOID AtfFilterStoreDefaultConfig(const CONFIG_CTX *configCtx)
{
    if (gConfigCtx != NULL) {
        AtfFilterFlushConfig();
    }

    KeWaitForSingleObject(&filterEngineLock, Executive, KernelMode, FALSE, NULL);
    {
        gConfigCtx = (CONFIG_CTX *)configCtx;
    }
    KeReleaseMutex(&filterEngineLock, FALSE);

    ATF_DEBUG(AtfFilterStoreDefaultConfig, "Successfully loaded filter config");
}

VOID AtfFilterFlushConfig(VOID)
{
    KeWaitForSingleObject(&filterEngineLock, Executive, KernelMode, FALSE, NULL);
    {
        if (gConfigCtx) {
            AtfFreeConfig(gConfigCtx);
            gConfigCtx = NULL;
        }
    }
    KeReleaseMutex(&filterEngineLock, FALSE);
}

//
// Cleanup filter engine on driver unload
//
VOID AtfFilterCleanup(VOID)
{
    AtfFilterFlushConfig();

    ATF_DEBUG(AtfFilterCleanup, "Successfully unloaded the filter engine");
}
    
