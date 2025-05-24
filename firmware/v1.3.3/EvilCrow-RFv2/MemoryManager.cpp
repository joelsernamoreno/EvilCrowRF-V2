#include "MemoryManager.h"

// Memory pool implementation for efficient RF sample storage
class MemoryPool
{
private:
    struct Block
    {
        uint8_t *data;
        size_t size;
        bool used;
        Block *next;
        Block *prev; // Added for bidirectional merging
    };

    Block *head;
    size_t totalSize;
    size_t blockSize;
    uint8_t *memoryArea;
    const size_t alignment = 32; // Alignment for optimal access

    // Align size to boundary
    size_t alignSize(size_t size)
    {
        return (size + alignment - 1) & ~(alignment - 1);
    }

    // Merge block with adjacent free blocks
    void mergeBlocks(Block *block)
    {
        // Merge with next block if free
        while (block->next && !block->next->used)
        {
            Block *next = block->next;
            block->size += next->size;
            block->next = next->next;
            if (next->next)
            {
                next->next->prev = block;
            }
            delete next;
        }

        // Merge with previous block if free
        while (block->prev && !block->prev->used)
        {
            Block *prev = block->prev;
            prev->size += block->size;
            prev->next = block->next;
            if (block->next)
            {
                block->next->prev = prev;
            }
            delete block;
            block = prev;
        }
    }

public:
    MemoryPool(size_t total, size_t block) : totalSize(total), blockSize(block)
    {
        memoryArea = (uint8_t *)heap_caps_aligned_alloc(alignment, totalSize, MALLOC_CAP_8BIT);
        if (!memoryArea)
        {
            log_e("Failed to allocate memory pool");
            return;
        }

        head = new Block();
        head->data = memoryArea;
        head->size = totalSize;
        head->used = false;
        head->next = nullptr;
        head->prev = nullptr;
    }

    ~MemoryPool()
    {
        if (memoryArea)
        {
            heap_caps_free(memoryArea);
        }
        while (head)
        {
            Block *next = head->next;
            delete head;
            head = next;
        }
    }

    void *allocate(size_t size)
    {
        size = alignSize(size);
        Block *current = head;
        Block *bestFit = nullptr;
        size_t minWaste = SIZE_MAX;

        // Find best fit block
        while (current)
        {
            if (!current->used && current->size >= size)
            {
                size_t waste = current->size - size;
                if (waste < minWaste)
                {
                    bestFit = current;
                    minWaste = waste;
                }
                if (waste == 0)
                    break; // Perfect fit
            }
            current = current->next;
        }

        if (!bestFit)
            return nullptr;

        // Split block if large enough
        if (bestFit->size > size + sizeof(Block) + alignment)
        {
            Block *newBlock = new Block();
            newBlock->data = bestFit->data + size;
            newBlock->size = bestFit->size - size;
            newBlock->used = false;
            newBlock->next = bestFit->next;
            newBlock->prev = bestFit;
            if (bestFit->next)
            {
                bestFit->next->prev = newBlock;
            }
            bestFit->size = size;
            bestFit->next = newBlock;
        }

        bestFit->used = true;
        return bestFit->data;
    }

    void free(void *ptr)
    {
        Block *current = head;
        while (current)
        {
            if (current->data == ptr)
            {
                current->used = false;
                mergeBlocks(current);
                break;
            }
            current = current->next;
        }
    }
};

bool MemoryManager::init()
{
    // Create memory pool for RF samples with improved size
    samplePool = new MemoryPool(32768, 2048); // 32KB pool with 2KB blocks

    // Start memory monitoring task
    xTaskCreatePinnedToCore(
        [](void *param)
        {
            static_cast<MemoryManager *>(param)->monitorTask(param);
        },
        "MemMonitor",
        4096,
        this,
        1,
        &monitorTaskHandle,
        1);

    // Initialize tracking
    memset(allocationHistory, 0, sizeof(allocationHistory));

    return true;
}

void *MemoryManager::allocate(size_t size, uint32_t caps)
{
    // Check if allocation might cause problems
    if (predictOOM(size))
    {
        // Try emergency recovery
        if (!recoverMemory(size))
        {
            log_e("Memory allocation of %u bytes would cause OOM", size);
            return nullptr;
        }
    }

    void *ptr = heap_caps_malloc(size, caps);
    if (!ptr)
    {
        // Initial allocation failed, try recovery
        if (recoverMemory(size))
        {
            ptr = heap_caps_malloc(size, caps);
        }
    }

    if (ptr)
    {
        recordAllocation(size);
    }

    return ptr;
}

void MemoryManager::free(void *ptr)
{
    if (!ptr)
        return;

    heap_caps_free(ptr);

    // Check fragmentation after free
    uint8_t frag = getFragmentation();
    if (frag > defragThreshold)
    {
        uint32_t now = millis();
        if (now - lastDefragTime > 5000)
        { // Don't defrag too often
            defragment();
            lastDefragTime = now;
        }
    }
}

bool MemoryManager::recoverMemory(size_t needed)
{
    size_t freeBeforeDefrag = getFreeHeap();
    uint8_t fragBeforeDefrag = getFragmentation();

    // Force aggressive defragmentation
    defragment();

    // Check if defrag helped
    size_t freeAfterDefrag = getFreeHeap();
    size_t recovered = freeAfterDefrag - freeBeforeDefrag;

    if (recovered >= needed)
    {
        log_i("Memory recovered through defrag: %u bytes", recovered);
        return true;
    }

    // TODO: Implement cache clearing and other recovery mechanisms

    return getFreeHeap() >= needed;
}

void MemoryManager::recordAllocation(size_t size)
{
    allocationHistory[historyIndex] = size;
    historyIndex = (historyIndex + 1) % HISTORY_SIZE;
    analyzeTrends();
}

bool MemoryManager::predictOOM(size_t size)
{
    size_t freeHeap = getFreeHeap();
    size_t largestBlock = getLargestFreeBlock();

    // Immediate OOM check
    if (size > largestBlock)
        return true;

    // Check if allocation would leave us with too little memory
    if (freeHeap - size < criticalMemoryThreshold)
        return true;

    // Check recent allocation patterns
    size_t recentTotal = 0;
    for (size_t i = 0; i < HISTORY_SIZE; i++)
    {
        recentTotal += allocationHistory[i];
    }

    // If recent allocations show high memory pressure, be conservative
    if (recentTotal > freeHeap / 2 && size > freeHeap / 4)
        return true;

    return false;
}

void MemoryManager::analyzeTrends()
{
    size_t recentTotal = 0;
    for (size_t i = 0; i < HISTORY_SIZE; i++)
    {
        recentTotal += allocationHistory[i];
    }

    float avgAllocation = recentTotal / static_cast<float>(HISTORY_SIZE);
    size_t freeHeap = getFreeHeap();

    if (avgAllocation > freeHeap / 10)
    {
        log_w("High memory pressure detected - avg allocation: %.2f bytes", avgAllocation);
    }
}

void MemoryManager::monitorTask(void *parameter)
{
    while (true)
    {
        size_t freeHeap = getFreeHeap();
        uint8_t frag = getFragmentation();

        // Log memory stats periodically
        if (frag > defragThreshold || freeHeap < lowMemoryThreshold)
        {
            log_w("Memory Warning - Free: %u, Frag: %u%%", freeHeap, frag);

            if (frag > emergencyDefragThreshold)
            {
                defragment();
            }
        }

        // Check heap integrity
        if (!heap_caps_check_integrity_all(true))
        {
            log_e("Heap corruption detected!");
            // TODO: Implement recovery strategy for heap corruption
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // Check every 5 seconds
    }
}

void MemoryManager::defragment()
{
    // Enhanced defragmentation
    size_t beforeFrag = getFragmentation();
    size_t beforeFree = getFreeHeap();

    // Force cleanup of any pending deallocations
    heap_caps_check_integrity_all(true);

    // Attempt to merge memory blocks
    if (samplePool)
    {
        // TODO: Implement custom defragmentation for sample pool
    }

    size_t afterFrag = getFragmentation();
    size_t afterFree = getFreeHeap();

    if (beforeFrag - afterFrag > 5 || afterFree > beforeFree)
    {
        log_i("Defrag: %u%% -> %u%%, Freed: %u bytes",
              beforeFrag, afterFrag, afterFree - beforeFree);
    }
}

void MemoryManager::printStats()
{
    log_i("Memory Stats:");
    log_i("Free: %u bytes", getFreeHeap());
    log_i("Largest Block: %u bytes", getLargestFreeBlock());
    log_i("Fragmentation: %u%%", getFragmentation());

    // Print prediction info
    size_t recentTotal = 0;
    for (size_t i = 0; i < HISTORY_SIZE; i++)
    {
        recentTotal += allocationHistory[i];
    }
    float avgAllocation = recentTotal / static_cast<float>(HISTORY_SIZE);

    log_i("Avg Recent Allocation: %.2f bytes", avgAllocation);
}
