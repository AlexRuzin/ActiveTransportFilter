#include <ntddk.h>

#include "mem.h"

#include "../common/errors.h"
#include "trace.h"

//
// Non-paged pool allocator
//
VOID *AtfMallocNP(SIZE_T size)
{
    if (size == 0) {
        return NULL;
    }

    VOID *ptr = ExAllocatePoolWithTag(NonPagedPool, size, MEM_PAGE_NAME_NP);
    if (ptr == NULL) {
        ATF_ERROR(AtfMallocNP, -1);
        return NULL;
    }

    return ptr;
}

//
// Non-paged pool deallocator
//
VOID AtfFreeNP(VOID *p)
{
    if (!p) {
        return;
    }

    ExFreePoolWithTag(p, MEM_PAGE_NAME_NP);
}

//
// Paged pool allocator
//
VOID *AtfMallocPP(SIZE_T size)
{
    if (size == 0) {
        return NULL;
    }

    VOID *ptr = ExAllocatePoolWithTag(PagedPool, size, MEM_PAGE_NAME_PP);
    if (ptr == NULL) {
        ATF_ERROR(AtfMallocNP, -1);
        return NULL;
    }

    return ptr;
}

//
// Paged pool deallocator
//
VOID AtfFreePP(VOID *p)
{
    if (!p) {
        return;
    }

    ExFreePoolWithTag(p, MEM_PAGE_NAME_PP);
}

//EOF