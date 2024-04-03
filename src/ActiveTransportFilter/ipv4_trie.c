#include <ntddk.h>

#include <inaddr.h>
#include <limits.h>

#include "ipv4_trie.h"

#include "mem.h"
#include "trace.h"

ATF_ERROR AtfIpv4TrieAllocCtx(IPV4_TRIE_CTX **ctxOut)
{
    IPV4_TRIE_CTX *ctx = (IPV4_TRIE_CTX *)ATF_MALLOC(sizeof(IPV4_TRIE_CTX));
    if (!ctx) {
        return ATF_NO_MEMORY_AVAILABLE;
    }

    ctx->root = ATF_MALLOC(IPV4_TRIE_NODE_SIZE);
    if (!ctx) {
        ATF_FREE(ctx);
        return ATF_NO_MEMORY_AVAILABLE;
    }

    *ctxOut = ctx;

    return ATF_ERROR_OK;
}

//
// Insert a IPv4 pool into the trie
//
ATF_ERROR AtfIpv4TrieInsertPool(IPV4_TRIE_CTX *ctx, const struct in_addr *pool, size_t numOfIps)
{
    DbgBreakPoint();

    if (!ctx || !pool || !numOfIps) {
        return ATF_BAD_PARAMETERS;
    }    

    // Iterate through the input pool
    for (size_t currIp = 0; currIp < numOfIps; currIp++) {
        struct in_addr ip;
        ip.S_un.S_addr = pool[currIp].S_un.S_addr;

        VOID **currTrieNode = ctx->root;

        // Iterate through each octet and insert trie at the corresponding index (octet)        
        for (UINT8 octetCount = 0; octetCount < sizeof(struct in_addr); octetCount++) {
            const IPV4_OCTET currOctet = (ip.S_un.S_addr >> (octetCount * 8)) & 0xff;

            // If we've reached the last octet, add a -1 (ipEndMarker)
            if (octetCount == 3) {
                currTrieNode[currOctet] = ipEndMarker;
                break;
            }

            // Add the new trie, if necessary
            if (currTrieNode[currOctet] == 0) {
                currTrieNode[currOctet] = (IPV4_OCTET *)ATF_MALLOC(IPV4_TRIE_NODE_SIZE);
                if (currTrieNode[currOctet] == NULL) {
                    return ATF_NO_MEMORY_AVAILABLE;
                }

                ctx->totalTrieSize += IPV4_TRIE_NODE_SIZE;
            }

            // Iterate into the next trie
            currTrieNode = currTrieNode[currOctet];
        }
    }

    ctx->totalNumOfIps += numOfIps;

    return ATF_ERROR_OK;
}

BOOLEAN AtfIpv4TrieSearch(IPV4_TRIE_CTX *ctx, struct in_addr ip)
{
    if (!ctx || !ctx->totalNumOfIps) {
        return FALSE;
    }

    VOID **currTrieNode = ctx->root;

    for (UINT8 octetCount = 0; octetCount < sizeof(struct in_addr); octetCount++) {
        const IPV4_OCTET octet = (ip.S_un.S_addr >> (octetCount * 8)) & 0xff;

        if (currTrieNode[octet] == NULL) {
            return FALSE;
        }

        // Just to be "safe", as the ipEndMarker pointer is invalid
        if (currTrieNode[octet] != ipEndMarker) {
            currTrieNode = currTrieNode[octet];
        }            
    }

    // All nodes
    return TRUE;
}

// Free prototype
static VOID AtfIpv4TrieFreeRecursive(VOID **root);

VOID AtfIpv4TrieFree(IPV4_TRIE_CTX **ctx)
{
    if (*ctx) {
        return;
    }

    IPV4_TRIE_CTX *c = *ctx;

    if (c->root) {
        AtfIpv4TrieFreeRecursive(c->root);
        ATF_FREE(c->root);
    }
    
    RtlZeroMemory(c, sizeof(IPV4_TRIE_CTX));
    ATF_FREE(c);
    *ctx = NULL;
}

static VOID AtfIpv4TrieFreeRecursive(VOID **root)
{
    if (!root) {
        return;
    }

    for (UINT8 i = 0; i < _UI8_MAX; i++) {
        if (root[i] == ipEndMarker) {
            // We've reached the last octet node, so return early
            return;
        } else if (root[i] != NULL) {
            AtfIpv4TrieFreeRecursive(root[i]);
            ATF_FREE(root[i]);
            root[i] = NULL;
        }
    }
}

VOID AtfIpv4TriePrintCtx(const IPV4_TRIE_CTX *ctx)
{
    if (!ctx) {
        return;
    }

    const size_t numOfNodes = ctx->totalTrieSize / IPV4_TRIE_NODE_SIZE;

    ATF_DEBUGA("[atftrace] IPv4 Trie Stats: Num of nodes: %d, Total Trie size: %d, Num of IPs: %d",
        numOfNodes, ctx->totalTrieSize, ctx->totalNumOfIps);
}