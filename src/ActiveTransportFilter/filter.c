#include <ntddk.h>

#include <initguid.h>
#include <guiddef.h>

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
// Returns TRUE if a config exists
//  A filter config must exist for the WFP callouts to be registered
//
BOOLEAN AtfFilterIsInitialized(VOID)
{
    if (gConfigCtx != NULL) {
        return TRUE;
    }

    return FALSE;
}

BOOLEAN AtfFilterIsLayerEnabled(const GUID *guid)
{
    // IOCTL locks are sufficient for this function, as WFP is in the process of initializing

    // Sanity checks just in case
    if (!guid) {
        return FALSE;
    }

    if (!AtfFilterIsInitialized()) {
        return FALSE;
    }

    if (gConfigCtx->numOfLayers == 0) {
        ATF_DEBUG(AtfFilterIsLayerEnabled, "Warning: 0 enabled WFP layers in config");
        return FALSE;
    }

    for (UINT8 i = 0; i < gConfigCtx->numOfLayers; i++) {
        if (IsEqualGUID(guid, gConfigCtx->enabledLayers[i].layerGuid)) {
            return gConfigCtx->enabledLayers[i].enabled;
        }
    }

    return FALSE;
}

//
// Filter callback for IPv4 (TCP) 
//
ATF_ERROR AtfFilterCallbackTcpIpv4Inbound(const ATF_FLT_DATA_IPV4 *data)
{
    if (data == NULL) {
        return ATF_BAD_PARAMETERS;
    }

    KeWaitForSingleObject(&filterEngineLock, Executive, KernelMode, FALSE, NULL);
    {

    }
    KeReleaseMutex(&filterEngineLock, FALSE);

    return ATF_ERROR_OK;
}

