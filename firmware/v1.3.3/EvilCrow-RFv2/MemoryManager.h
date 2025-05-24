#pragma once

#include <Arduino.h>
#include "esp_heap_caps.h"
#include "esp_task_wdt.h"

class MemoryPool;

class MemoryManager
{
public:
    static MemoryManager &getInstance()
    {
        static MemoryManager instance;
        return instance;
    }

    // Initialize memory management system
    bool init();

    // Enhanced memory allocation with fallback and recovery
    void *allocate(size_t size, uint32_t caps = MALLOC_CAP_8BIT);

    // Smart free with defragmentation trigger
    void free(void *ptr);

    // Get current free heap
    size_t getFreeHeap();

    // Get largest contiguous free block
    size_t getLargestFreeBlock();

    // Get heap fragmentation percentage
    uint8_t getFragmentation();

    // Memory pool for RF samples
    MemoryPool *getSamplePool();

    // Enhanced memory defragmentation
    void defragment();

    // Memory statistics with prediction
    void printStats();

    // Emergency memory recovery
    bool recoverMemory(size_t needed);

    // Check if allocation would succeed
    bool canAllocate(size_t size, uint32_t caps = MALLOC_CAP_8BIT);

private:
    MemoryManager() : lowMemoryThreshold(10240),
                      criticalMemoryThreshold(5120),
                      defragThreshold(70),
                      emergencyDefragThreshold(85) {}
    ~MemoryManager() {}
    MemoryManager(const MemoryManager &) = delete;
    MemoryManager &operator=(const MemoryManager &) = delete;

    // Memory monitoring task
    void monitorTask(void *parameter);

    // Analyze memory trends
    void analyzeTrends();

    // Track allocation history
    void recordAllocation(size_t size);

    // Memory prediction
    bool predictOOM(size_t size);

    MemoryPool *samplePool;
    TaskHandle_t monitorTaskHandle;

    // Configuration thresholds
    const size_t lowMemoryThreshold;        // 10KB
    const size_t criticalMemoryThreshold;   // 5KB
    const uint8_t defragThreshold;          // 70%
    const uint8_t emergencyDefragThreshold; // 85%

    // Memory tracking
    static const size_t HISTORY_SIZE = 10;
    size_t allocationHistory[HISTORY_SIZE] = {0};
    size_t historyIndex = 0;
    uint32_t lastDefragTime = 0;
};

// Global access point
#define MEM_MGR MemoryManager::getInstance()
