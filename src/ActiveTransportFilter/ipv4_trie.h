#pragma once

#include <ntddk.h>

#include <inaddr.h>

#include "../common/errors.h"

#include "mem.h"

#define IPV4_TRIE_SIZE (_UI8_MAX * sizeof(VOID *))

//
// Sorted "patricia trie" for IPv4 addresses
// 
//  First variant: each ipv4 octet will be represented by an array of pointers, the complexity time 
//   will be O(m) for each octet. 
// 
//

typedef UINT8 IPV4_OCTET;

//
// The last octet pointer array is marked with this value, indicating that the IP is indeed in the trie
//
static VOID *ipEndMarker = (VOID *)-1;

//
// Trie instance context
//
typedef struct _ipv4_trie_ctx {
    // Total physical size of the trie, in bytes
    size_t          totalTrieSize;


    // Total number of stored IP addresses
    size_t          totalNumOfIps;

    // root
    VOID            *root;
} IPV4_TRIE_CTX, *PIPV4_TRIE_CTX;

//
// Initialize the context
//
ATF_ERROR AtfIpv4TrieAllocCtx(IPV4_TRIE_CTX **ctxOut);

//
// Insert ipv4 pool into trie
//  root is the first array uint8_t, which points to the second array
//
ATF_ERROR AtfIpv4TrieInsertPool(IPV4_TRIE_CTX *ctx, const struct in_addr *pool, size_t numOfIps);

//
// Print trie context info
//
VOID AtfIpv4TriePrintCtx(const IPV4_TRIE_CTX *ctx);

//
// Free the entire trie and context
// 
VOID AtfIpv4TrieFree(IPV4_TRIE_CTX **ctx);

//EOF