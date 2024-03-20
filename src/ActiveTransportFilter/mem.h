#pragma once

#include <ntddk.h>

//
// Force allocations to be non-paged (see mem.h)
//
#if defined(MEM_USE_NON_PAGED_POOL)
#undef MEM_USE_NON_PAGED_POOL
#endif //#define MEM_USE_NON_PAGED_POOL
#define MEM_USE_NON_PAGED_POOL

#include "../common/errors.h"

#define MEM_PAGE_NAME_NP                'SRnp'
#define MEM_PAGE_NAME_PP                'SRpp'

//
// Define default allocators
//
#if defined(MEM_USE_NON_PAGED_POOL)
#define ATF_MALLOC(x) AtfMallocNP(x)
#else //MEM_USE_NON_PAGED_POOL
#define ATF_MALLOC(x) AtfMallocPP(x)
#endif //MEM_USE_NON_PAGED_POOL

//
// Define default deallocators
//
#if defined(MEM_USE_NON_PAGED_POOL)
#define ATF_FREE(x) AtfFreeNP(x)
#else //MEM_USE_NON_PAGED_POOL
#define ATF_FREE(x) AtfFreePP(x)
#endif //MEM_USE_NON_PAGED_POOL


//
// Non-paged memory
//

// Allocate non-paged Memory (default)
VOID *AtfMallocNP(SIZE_T size);

// Free non-paged memory (default)
VOID AtfFreeNP(VOID *p);


//
// Paged memory
//

// Allocate paged memory 
VOID *AtfMallocPP(SIZE_T size);

// Free paged memory 
VOID AtfFreePP(VOID *p);


