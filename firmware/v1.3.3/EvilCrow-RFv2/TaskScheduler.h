#pragma once

#include <Arduino.h>
#include <functional>
#include <vector>
#include <string>
#include <memory>
#include "esp_task_wdt.h"
#include "MemoryManager.h"

enum class TaskPriority
{
    CRITICAL = 0,      // Highest priority, for RF timing critical tasks
    PRIORITY_HIGH = 1, // High priority for signal processing
    NORMAL = 2,        // Normal priority for UI updates, file operations
    LOW = 3,           // Background tasks
    IDLE = 4           // Only when system is idle
};

enum class TaskCore
{
    CORE_0 = 0, // Protocol task core
    CORE_1 = 1, // Arduino core
    ANY = -1    // Any available core
};

class TaskInfo
{
public:
    std::string name;
    std::function<void()> callback;
    TaskPriority priority;
    TaskCore preferredCore;
    size_t stackSize;
    TaskHandle_t handle;
    uint32_t lastRunTime;
    uint32_t totalRunTime;
    bool isRunning;

    TaskInfo(const std::string &n, std::function<void()> cb,
             TaskPriority p, TaskCore core = TaskCore::ANY, size_t stack = 4096)
        : name(n), callback(cb), priority(p), preferredCore(core),
          stackSize(stack), handle(nullptr), lastRunTime(0),
          totalRunTime(0), isRunning(false) {}
};

class TaskScheduler
{
public:
    static TaskScheduler &getInstance()
    {
        static TaskScheduler instance;
        return instance;
    }

    // Task management
    bool addTask(const std::string &name, std::function<void()> callback,
                 TaskPriority priority, TaskCore preferredCore = TaskCore::ANY,
                 size_t stackSize = 4096);
    bool removeTask(const std::string &name);
    bool pauseTask(const std::string &name);
    bool resumeTask(const std::string &name);

    // Task monitoring
    struct CoreStats
    {
        size_t activeTasks;
        size_t totalMemory;
        size_t freeMemory;
        float cpuUsage;
    };

    CoreStats getStats(TaskCore core);
    std::vector<TaskInfo *> getRunningTasks();
    TaskInfo *getTaskInfo(const std::string &name);

    // System management
    void init();
    void cleanup();
    void monitorTasks();

    // Resource management
    bool reserveMemory(size_t bytes);
    void releaseMemory(size_t bytes);

private:
    TaskScheduler() {}
    ~TaskScheduler() { cleanup(); }
    TaskScheduler(const TaskScheduler &) = delete;
    TaskScheduler &operator=(const TaskScheduler &) = delete;

    static void taskWrapper(void *params);
    bool createTask(TaskInfo *task);
    void updateTaskStats(TaskInfo *task);
    void checkResourceLimits();

    std::vector<TaskInfo *> tasks;
    CoreStats coreStats[2];
    size_t reservedMemory;
    SemaphoreHandle_t schedulerMutex;

    // Constants
    static constexpr size_t MAX_TASKS_PER_CORE = 8;
    static constexpr size_t MIN_FREE_MEMORY = 32768;        // 32KB minimum free
    static constexpr uint32_t STATS_UPDATE_INTERVAL = 5000; // 5 seconds
};

#define SCHEDULER TaskScheduler::getInstance()
