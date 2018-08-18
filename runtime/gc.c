#include <stdint.h>
#include <stddef.h>

#include "heap.h"
#include "lib/debug.h"
#include "gc.h"
#include "language.h"
#include "assert.h"
#include "objects.h"
#include "config.h"
#include "global.h"
#include "runtime.h"

static Heap* activeHeap;
static Heap* nextHeap;

static JsValue** rootsArray;
static uint64_t rootsArrayLength = 0;

void gcInit() {
    ConfigValue heapSize = configGet(HeapSizeConfig);
    activeHeap = heapCreate(heapSize.uintValue);
    nextHeap = heapCreate(heapSize.uintValue);
}

void _gcTestInit() {
    if(activeHeap != NULL) {
        heapFree(activeHeap);
        heapFree(nextHeap);
    }
    gcInit();
}

void* gcAllocate2(size_t bytes, int type) {
    GcObject* allocated = gcAllocate(bytes);
    allocated->type = type;
    return allocated;
}

void* gcAllocate(size_t bytes) {
    GcObject* allocated = heapAllocate(activeHeap, bytes);
    if(allocated == NULL) {
        RuntimeEnvironment* runtime = runtimeGet();
        _gcRun(runtime->gcRoots, runtime->gcRootsCount);
    }
    allocated->next = heapTop(activeHeap);
    assert(allocated->next != NULL);
    return allocated;
}

GcStats gcStats() {
    return (GcStats) {
        .used = heapBytesUsed(activeHeap),
        .remaining = heapBytesRemaining(activeHeap),
    };
}

static uint64_t gcObjectSize(GcObject* object) {
    assert(object->next != NULL);
    return (uint64_t)object->next - (uint64_t)object;
}

static GcObject* move(GcObject* item) {
    if(item->movedTo) {
        return item->movedTo;
    }

    uint64_t size = gcObjectSize(item);
    GcObject* newAddress = heapAllocate(nextHeap, size);
    memcpy(newAddress, item, size);
    item->movedTo = newAddress;
    newAddress->next = (void*)(((uint64_t)newAddress) + size);
    return newAddress;
}

static void traverse(GcObject* object) {
    switch(object->type) {
        case OBJECT_TYPE:
            objectGcTraverse((void*)object, (GcCallback*)move);
            break;
        // TODO - strings copy over string
        default:
            break;
    }
}

/**
 * GC uses something like the Cheny algorithm.
 *
 * Pseudo-code:
 *
 *     main([global, ...taskQueues])
 *
 *     main(roots)
 *       for each root
 *         move(root)
 *
 *       toProcess = nextHeap->bottom
 *       do 
 *         traverse(toProcess)
 *       while toProcess = toProcess->next
 *
 *     move(item)
 *       if not copied(item)
 *         copy item into new
 *         update movedTo of old
 *         clear movedTo in new
 *         update next pointer in new
 *
 *     traverse(item)
 *       for each obj reachable from item
 *          move(obj)
 *          updateReference(obj, item)
 *
 */

// roots: global environment
//        task queues pointing to ExecuableFunctions (which point to environments)
void _gcRun(GcObject** roots, uint64_t rootCount) {
    GcStats before = gcStats();

    // For item in roots
    for(uint64_t i = 0;
        i < rootCount;
        i++) {
        roots[i] = move(roots[i]);
    }
    log_info("GC moved %llu roots", rootCount);

    GcObject* toProcess = (void*)nextHeap->bottom;
    while((char*)toProcess != nextHeap->top) {
        traverse(toProcess);
        toProcess = toProcess->next;
    }

    Heap* oldHeap = activeHeap;
    activeHeap = nextHeap;
    heapFree(oldHeap);
    nextHeap = oldHeap;

    GcStats after = gcStats();
    // TODO - do this more safely?
    int64_t saved = (int64_t)(before.used - after.used);
    log_info("GC complete, %lli bytes collected", saved);
}

void* _gcMovedTo(GcObject* object) {
    return object->movedTo;
}

void gcSetRoots(JsValue** roots, uint64_t count) {
    rootsArray = roots;
    rootsArrayLength = count;
}
