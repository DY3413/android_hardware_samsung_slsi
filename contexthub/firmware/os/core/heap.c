/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <trylock.h>
#include <atomic.h>
#include <stdint.h>
#include <stdio.h>
#include <heap.h>
#include <seos.h>
#include <cmsis.h>
#if defined(EXYNOS_CONTEXTHUB)
#include <csp_common.h>
#endif
#if defined(HEAP_DEBUG)
#include <heapDebug.h>
#endif
#define TIDX_HEAP_EXTRA 2 // must be >= 0; best if > 0, don't make it > 7, since it unnecessarily limits max heap size we can manage

#define TIDX_HEAP_BITS (TASK_IDX_BITS + TIDX_HEAP_EXTRA)

#define TIDX_MASK ((1 << TIDX_HEAP_BITS) - 1)
#define MAX_HEAP_ORDER (31 - TIDX_HEAP_BITS)

#if MAX_HEAP_ORDER < 16
# error Too little HEAP is available
#endif

struct HeapNode {

    struct HeapNode* prev;
    uint32_t size: MAX_HEAP_ORDER;
    uint32_t used: 1;
    uint32_t tidx: TIDX_HEAP_BITS; // TASK_IDX_BITS to uniquely identify task; + extra bits of redundant counter add extra protection
    uint8_t  data[];
};

#ifdef FORCE_HEAP_IN_DOT_DATA

    static uint8_t __attribute__ ((aligned (8))) gHeap[HEAP_SIZE];

    #define REAL_HEAP_SIZE     ((HEAP_SIZE) &~ 7)
    #define ALIGNED_HEAP_START (&gHeap)

#else

    extern uint8_t __heap_end[], __heap_start[];
    #define ALIGNED_HEAP_START  (uint8_t*)((((uintptr_t)&__heap_start) + 7) &~ 7)
    #define ALIGNED_HEAP_END    (uint8_t*)(((uintptr_t)&__heap_end) &~ 7)

    #define REAL_HEAP_SIZE      (ALIGNED_HEAP_END - ALIGNED_HEAP_START)


#endif

static inline void HEAP_ASSERT(int test)
{
    if (!(test)) {
        osLog(LOG_ERROR, "[assert]%s, %d\n", __func__, __LINE__);
        osLog(LOG_ERROR,  "%s\n", __FILE__);
      #if defined(EXYNOS_CONTEXTHUB)
        //cspNotify(ERR_ASSERT);
      #endif
    }
}

static struct HeapNode* gHeapHead;
static TRYLOCK_DECL_STATIC(gHeapLock) = TRYLOCK_INIT_STATIC();
static volatile uint8_t gNeedFreeMerge = false; /* cannot be bool since its size is ill defined */
static struct HeapNode *gHeapTail;

static inline struct HeapNode* heapPrvGetNext(struct HeapNode* node)
{
    return (gHeapTail == node) ? NULL : (struct HeapNode*)(node->data + node->size);
}

bool heapInit(void)
{
    uint32_t size = REAL_HEAP_SIZE;
    struct HeapNode* node;

    node = gHeapHead = (struct HeapNode*)ALIGNED_HEAP_START;

    if (size < sizeof(struct HeapNode))
    {
        HEAP_ASSERT(0);
        return false;
    }

    gHeapTail = node;

    node->used = 0;
    node->prev = NULL;
    node->size = size - sizeof(struct HeapNode);

#if defined(HEAP_DEBUG)
    heapHistoryInit();
#endif

    return true;
}

#if defined(HEAP_DEBUG)
void heapShowStatus(void)
{
    CSP_PRINTF_INFO("%s: base:0x%x size:%d\n", __func__, (u32)gHeapHead, REAL_HEAP_SIZE);
}
#endif

//called to merge free chunks in case free() was unable to last time it tried. only call with lock held please
static void heapMergeFreeChunks(void)
{
    while (atomicXchgByte(&gNeedFreeMerge, false)) {
        struct HeapNode *node = gHeapHead, *next;

        while (node) {
            next = heapPrvGetNext(node);

            if (!node->used && next && !next->used) { /* merged */
                node->size += sizeof(struct HeapNode) + next->size;

                next = heapPrvGetNext(node);
                if (next)
                    next->prev = node;
                else
                    gHeapTail = node;
            }
            else
                node = next;
        }
    }
}

#if defined(HEAP_DEBUG)
void* __heapAlloc(uint32_t sz, const char* fn, int line)
#else
void* heapAlloc(uint32_t sz)
#endif
{
    struct HeapNode *node, *best = NULL;
    void* ret = NULL;

    __disable_irq();

    if (!trylockTryTake(&gHeapLock))
    {
        HEAP_ASSERT(0);
        __enable_irq();
        return NULL;
    }

    /* merge free chunks to help better use space */
    heapMergeFreeChunks();

    sz = (sz + 3) &~ 3;
    node = gHeapHead;

    while (node) {
        if (!node->used && node->size >= sz && (!best || best->size > node->size)) {
            best = node;
            if (best->size == sz)
                break;
        }

        node = heapPrvGetNext(node);
    }

    if (!best) //alloc failed
        goto out;

    if (best->size - sz > sizeof(struct HeapNode)) {        //there is a point to split up the chunk

        node = (struct HeapNode*)(best->data + sz);

        node->used = 0;
        node->tidx = 0;
        node->size = best->size - sz - sizeof(struct HeapNode);
        node->prev = best;

        if (best != gHeapTail)
            heapPrvGetNext(node)->prev = node;
        else
            gHeapTail = node;

        best->size = sz;
    }

    best->used = 1;
    best->tidx = osGetCurrentTid();
    ret = best->data;

out:
    trylockRelease(&gHeapLock);
    __enable_irq();

    if(ret == NULL) {
#if defined(HEAP_DEBUG)
        heapHistoryPush(fn, line, sz, ret);
        heapHistoryPrint();
#endif
        HEAP_ASSERT(0);
    }
#if defined(HEAP_DEBUG)
    heapHistoryPush(fn, line, sz, ret);
#endif
    return ret;
}

void heapFree(void* ptr)
{
    struct HeapNode *node, *t;
    bool haveLock;

    if (ptr == NULL) {
        // NULL is a valid reply from heapAlloc, and thus it is not an error for
        // us to receive it here.  We just ignore it.
        HEAP_ASSERT(0);
        return;
    }

    __disable_irq();
    haveLock = trylockTryTake(&gHeapLock);

    node = ((struct HeapNode*)ptr) - 1;
    node->used = 0;
    node->tidx = 0;

    if (haveLock) {

        while (node->prev && !node->prev->used)
            node = node->prev;

        while ((t = heapPrvGetNext(node)) && !t->used) {
            node->size += sizeof(struct HeapNode) + t->size;
            if (gHeapTail == t)
                gHeapTail = node;
        }

        if ((t = heapPrvGetNext(node)))
            t->prev = node;

        trylockRelease(&gHeapLock);
    }
    else
        gNeedFreeMerge = true;

    __enable_irq();

#if defined(HEAP_DEBUG)
    heapHistoryPop(ptr);
#endif
}

int heapFreeAll(uint32_t tid)
{
    struct HeapNode *node;
    bool haveLock;
    int count = 0;

    if (!tid)
    {
        HEAP_ASSERT(0);
        return -1;
    }

    __disable_irq();
    // this can only fail if called from interrupt
    haveLock = trylockTryTake(&gHeapLock);
    if (!haveLock)
    {
        HEAP_ASSERT(0);
        __enable_irq();
        return -1;
    }

    tid &= TIDX_MASK;
    for (node = gHeapHead; node; node = heapPrvGetNext(node)) {
        if (node->tidx == tid) {
            node->used = 0;
            node->tidx = 0;
            count++;
#if defined(HEAP_DEBUG)
            heapHistoryPop(node->data);
#endif
        }
    }
    gNeedFreeMerge = count > 0;
    trylockRelease(&gHeapLock);
    __enable_irq();

    return count;
}

int heapGetFreeSize(int *numChunks, int *largestChunk)
{
    struct HeapNode *node;
    bool haveLock;
    int bytes = 0;
    *numChunks = *largestChunk = 0;

    __disable_irq();
    // this can only fail if called from interrupt
    haveLock = trylockTryTake(&gHeapLock);
    if (!haveLock) {
        __enable_irq();
        return -1;
    }

    for (node = gHeapHead; node; node = heapPrvGetNext(node)) {
        if (!node->used) {
            if (node->size > *largestChunk)
                *largestChunk = node->size;
            bytes += node->size + sizeof(struct HeapNode);
            (*numChunks)++;
        }
    }
    trylockRelease(&gHeapLock);
    __enable_irq();

    return bytes;
}

int heapGetTaskSize(uint32_t tid)
{
    struct HeapNode *node;
    bool haveLock;
    int bytes = 0;

    __disable_irq();
    // this can only fail if called from interrupt
    haveLock = trylockTryTake(&gHeapLock);
    if (!haveLock) {
        __enable_irq();
        return -1;
    }

    tid &= TIDX_MASK;
    for (node = gHeapHead; node; node = heapPrvGetNext(node)) {
        if (node->used && node->tidx == tid) {
            bytes += node->size + sizeof(struct HeapNode);
        }
    }
    trylockRelease(&gHeapLock);
    __enable_irq();

    return bytes;
}
