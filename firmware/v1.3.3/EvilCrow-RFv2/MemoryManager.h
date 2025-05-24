#pragma once

#include <Arduino.h>
#include "esp_heap_caps.h"
#include "esp_task_wdt.h"

class MemoryPool;

class MemoryManager {
public:
    static MemoryManager& getInstance() {
        static MemoryManager instance;
        return instance;
    }

    // Initialize memory management system
    bool init();

    // Allocate memory from the appropriate pool
    void* allocate(size_t size, uint32_t caps = MALLOC_CAP_8BIT);

    // Free allocated memory
    void free(void* ptr);

    // Get current free heap
    size_t getFreeHeap();

    // Get largest contiguous free block
    size_t getLargestFreeBlock();

    // Get heap fragmentation percentage
    uint8_t getFragmentation();

    // Memory pool for RF samples
    MemoryPool* getSamplePool();

    // Memory defragmentation
    void defragment();

    // Memory statistics
    void printStats();

private:
    MemoryManager() {}
    ~MemoryManager() {}
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    // Memory monitoring
    void monitorTask(void* parameter);
    
    MemoryPool* samplePool;
    TaskHandle_t monitorTaskHandle;
};

// Global access point
#define MEM_MGR MemoryManager::getInstance()
