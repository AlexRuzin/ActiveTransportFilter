#pragma once

//
// Main configuration file for the WFP subsystem
//  Contains constants for initializing WFP, these will eventually become user-configurable,
//   in the meantime, store the default configuration for WFP here
//

#include <wchar.h>

#if !defined(INITGUID)
#define INITGUID
#endif //INITGUID

#include <initguid.h>

EXTERN_C_START

// 6f752cae-aae6-4f05-a9f8-402d149a1a31
DEFINE_GUID(
    ATF_FWPM_PROVIDER_KEY, 
    0x6f752cae, 
    0xaae6, 
    0x4f05, 
    0xa9, 0xf8,
    0x40, 0x2d, 0x14, 0x9a, 0x1a, 0x31
);

EXTERN_C_END

