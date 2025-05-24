#include "MemoryManager.h"

// Memory pool implementation for efficient RF sample storage
class MemoryPool {
private:
    struct Block {
        uint8_t* data;
        size_t size;
        bool used;
        Block* next;
    };

    Block* head;
    size_t totalSize;
    size_t blockSize;
    uint8_t* memoryArea;

public:
    MemoryPool(size_t total, size_t block) : totalSize(total), blockSize(block) {
        memoryArea = (uint8_t*)heap_caps_malloc(totalSize, MALLOC_CAP_8BIT);
        if (!memoryArea) {
            log_e("Failed to allocate memory pool");
            return;
        }

        head = new Block();
        head->data = memoryArea;
        head->size = totalSize;
        head->used = false;
        head->next = nullptr;
    }

    ~MemoryPool() {
        if (memoryArea) {
            heap_caps_free(memoryArea);
        }
        while (head) {
            Block* next = head->next;
            delete head;
            head = next;
        }
    }

    void* allocate(size_t size) {
        Block* current = head;
        while (current) {
            if (!current->used && current->size >= size) {
                if (current->size > size + sizeof(Block)) {
                    // Split block
                    Block* newBlock = new Block();
                    newBlock->data = current->data + size;
                    newBlock->size = current->size - size;
                    newBlock->used = false;
                    newBlock->next = current->next;
                    current->size = size;
                    current->next = newBlock;
                }
                current->used = true;
                return current->data;
            }
            current = current->next;
        }
        return nullptr;
    }

    void free(void* ptr) {
        Block* current = head;
        while (current) {
            if (current->data == ptr) {
                current->used = false;
                // Merge with next block if free
                if (current->next && !current->next->used) {
                    Block* next = current->next;
                    current->size += next->size;
                    current->next = next->next;
                    delete next;
                }
                break;
            }
            current = current->next;
        }
    }
};

bool MemoryManager::init() {
    // Create memory pool for RF samples
    samplePool = new MemoryPool(32768, 2048); // 32KB pool with 2KB blocks
    
    // Start memory monitoring task
    xTaskCreatePinnedToCore(
        [](void* param) { 
            static_cast<MemoryManager*>(param)->monitorTask(param); 
        },
        "MemMonitor",
        4096,
        this,
        1,
        &monitorTaskHandle,
        1
    );

    return true;
}

void* MemoryManager::allocate(size_t size, uint32_t caps) {
    void* ptr = heap_caps_malloc(size, caps);
    if (!ptr) {
        // Try to free some memory
        defragment();
        ptr = heap_caps_malloc(size, caps);
    }
    return ptr;
}

void MemoryManager::free(void* ptr) {
    heap_caps_free(ptr);
}

size_t MemoryManager::getFreeHeap() {
    return heap_caps_get_free_size(MALLOC_CAP_8BIT);
}

size_t MemoryManager::getLargestFreeBlock() {
    return heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
}

uint8_t MemoryManager::getFragmentation() {
    size_t freeSize = getFreeHeap();
    size_t maxBlock = getLargestFreeBlock();
    return freeSize > 0 ? (100 - (maxBlock * 100) / freeSize) : 0;
}

MemoryPool* MemoryManager::getSamplePool() {
    return samplePool;
}

void MemoryManager::defragment() {
    // Basic defragmentation
    size_t beforeFrag = getFragmentation();
    
    // Force cleanup of any pending deallocations
    heap_caps_check_integrity_all(true);
    
    size_t afterFrag = getFragmentation();
    if (beforeFrag - afterFrag > 5) {
        log_i("Defrag: %u%% -> %u%%", beforeFrag, afterFrag);
    }
}

void MemoryManager::printStats() {
    log_i("Memory Stats:");
    log_i("Free: %u bytes", getFreeHeap());
    log_i("Largest Block: %u bytes", getLargestFreeBlock());
    log_i("Fragmentation: %u%%", getFragmentation());
}

void MemoryManager::monitorTask(void* parameter) {
    const size_t LOW_MEMORY_THRESHOLD = 10240; // 10KB
    
    while (true) {
        size_t freeHeap = getFreeHeap();
        uint8_t frag = getFragmentation();
        
        // Log memory stats periodically
        if (frag > 70 || freeHeap < LOW_MEMORY_THRESHOLD) {
            log_w("Memory Warning - Free: %u, Frag: %u%%", freeHeap, frag);
            if (frag > 80) {
                defragment();
            }
        }
        
        // Check heap integrity
        if (!heap_caps_check_integrity_all(true)) {
            log_e("Heap corruption detected!");
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000)); // Check every 5 seconds
    }
}
