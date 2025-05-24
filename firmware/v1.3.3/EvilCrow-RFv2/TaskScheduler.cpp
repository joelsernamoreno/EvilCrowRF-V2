#include "TaskScheduler.h"

void TaskScheduler::init()
{
    schedulerMutex = xSemaphoreCreateMutex();
    reservedMemory = 0;

    // Initialize core statistics
    for (int i = 0; i < 2; i++)
    {
        coreStats[i] = {0, 0, 0, 0.0f};
    }

    // Create task monitoring task
    xTaskCreatePinnedToCore(
        [](void *param)
        {
            static_cast<TaskScheduler *>(param)->monitorTasks();
        },
        "TaskMonitor",
        4096,
        this,
        configMAX_PRIORITIES - 1,
        nullptr,
        1);
}

void TaskScheduler::cleanup()
{
    if (schedulerMutex)
    {
        vSemaphoreTake(schedulerMutex, portMAX_DELAY);

        // Clean up all tasks
        for (auto task : tasks)
        {
            if (task->handle)
            {
                vTaskDelete(task->handle);
            }
            delete task;
        }
        tasks.clear();

        vSemaphoreGive(schedulerMutex);
        vSemaphoreDelete(schedulerMutex);
        schedulerMutex = nullptr;
    }
}

bool TaskScheduler::addTask(const std::string &name, std::function<void()> callback,
                            TaskPriority priority, TaskCore preferredCore, size_t stackSize)
{
    if (!callback || name.empty())
    {
        return false;
    }

    vSemaphoreTake(schedulerMutex, portMAX_DELAY);

    // Check if task already exists
    for (auto task : tasks)
    {
        if (task->name == name)
        {
            vSemaphoreGive(schedulerMutex);
            return false;
        }
    }

    // Create new task
    auto taskInfo = new TaskInfo(name, callback, priority, preferredCore, stackSize);

    // Check resource limits
    if (!reserveMemory(stackSize))
    {
        delete taskInfo;
        vSemaphoreGive(schedulerMutex);
        return false;
    }

    // Create the actual FreeRTOS task
    bool success = createTask(taskInfo);
    if (success)
    {
        tasks.push_back(taskInfo);
    }
    else
    {
        releaseMemory(stackSize);
        delete taskInfo;
    }

    vSemaphoreGive(schedulerMutex);
    return success;
}

bool TaskScheduler::createTask(TaskInfo *task)
{
    BaseType_t core = (task->preferredCore == TaskCore::ANY) ? tskNO_AFFINITY : static_cast<BaseType_t>(task->preferredCore);

    // Convert priority to FreeRTOS priority
    UBaseType_t priority = configMAX_PRIORITIES - 1 - static_cast<UBaseType_t>(task->priority);

    return xTaskCreatePinnedToCore(
               taskWrapper,
               task->name.c_str(),
               task->stackSize,
               task,
               priority,
               &task->handle,
               core) == pdPASS;
}

void TaskScheduler::taskWrapper(void *params)
{
    auto task = static_cast<TaskInfo *>(params);
    if (!task)
        return;

    while (true)
    {
        if (task->isRunning)
        {
            uint32_t start = millis();

            try
            {
                task->callback();
            }
            catch (...)
            {
                // Log error and continue
                log_e("Task %s threw an exception", task->name.c_str());
            }

            task->lastRunTime = millis() - start;
            task->totalRunTime += task->lastRunTime;
        }

        // Yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

bool TaskScheduler::removeTask(const std::string &name)
{
    vSemaphoreTake(schedulerMutex, portMAX_DELAY);

    for (auto it = tasks.begin(); it != tasks.end(); ++it)
    {
        if ((*it)->name == name)
        {
            TaskInfo *task = *it;
            if (task->handle)
            {
                vTaskDelete(task->handle);
            }
            releaseMemory(task->stackSize);
            tasks.erase(it);
            delete task;
            vSemaphoreGive(schedulerMutex);
            return true;
        }
    }

    vSemaphoreGive(schedulerMutex);
    return false;
}

bool TaskScheduler::pauseTask(const std::string &name)
{
    vSemaphoreTake(schedulerMutex, portMAX_DELAY);

    for (auto task : tasks)
    {
        if (task->name == name)
        {
            task->isRunning = false;
            vSemaphoreGive(schedulerMutex);
            return true;
        }
    }

    vSemaphoreGive(schedulerMutex);
    return false;
}

bool TaskScheduler::resumeTask(const std::string &name)
{
    vSemaphoreTake(schedulerMutex, portMAX_DELAY);

    for (auto task : tasks)
    {
        if (task->name == name)
        {
            task->isRunning = true;
            vSemaphoreGive(schedulerMutex);
            return true;
        }
    }

    vSemaphoreGive(schedulerMutex);
    return false;
}

void TaskScheduler::monitorTasks()
{
    TickType_t lastStatsUpdate = xTaskGetTickCount();

    while (true)
    {
        vSemaphoreTake(schedulerMutex, portMAX_DELAY);

        // Update core statistics
        for (int core = 0; core < 2; core++)
        {
            coreStats[core].activeTasks = 0;
            coreStats[core].freeMemory = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
            coreStats[core].totalMemory = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);

            for (auto task : tasks)
            {
                TaskCore taskCore = (task->preferredCore == TaskCore::ANY) ? (core == 0 ? TaskCore::CORE_0 : TaskCore::CORE_1) : task->preferredCore;

                if (taskCore == static_cast<TaskCore>(core) && task->isRunning)
                {
                    coreStats[core].activeTasks++;
                }
            }
        }

        // Check for resource limits and adjust if needed
        checkResourceLimits();

        // Log statistics periodically
        if ((xTaskGetTickCount() - lastStatsUpdate) >= pdMS_TO_TICKS(STATS_UPDATE_INTERVAL))
        {
            for (int core = 0; core < 2; core++)
            {
                log_i("Core %d Stats - Tasks: %u, Free Mem: %u, CPU: %.1f%%",
                      core, coreStats[core].activeTasks,
                      coreStats[core].freeMemory, coreStats[core].cpuUsage);
            }
            lastStatsUpdate = xTaskGetTickCount();
        }

        vSemaphoreGive(schedulerMutex);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Update every second
    }
}

void TaskScheduler::checkResourceLimits()
{
    size_t freeMemory = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

    // Check if we're running low on memory
    if (freeMemory < MIN_FREE_MEMORY)
    {
        // Find lowest priority running task
        TaskInfo *lowestPriorityTask = nullptr;
        for (auto task : tasks)
        {
            if (task->isRunning && (!lowestPriorityTask ||
                                    task->priority > lowestPriorityTask->priority))
            {
                lowestPriorityTask = task;
            }
        }

        // Pause lowest priority task if found
        if (lowestPriorityTask)
        {
            log_w("Low memory: Pausing task %s", lowestPriorityTask->name.c_str());
            lowestPriorityTask->isRunning = false;
        }
    }
}

bool TaskScheduler::reserveMemory(size_t bytes)
{
    size_t freeMemory = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    if (freeMemory - bytes < MIN_FREE_MEMORY)
    {
        return false;
    }
    reservedMemory += bytes;
    return true;
}

void TaskScheduler::releaseMemory(size_t bytes)
{
    if (reservedMemory >= bytes)
    {
        reservedMemory -= bytes;
    }
}

TaskScheduler::CoreStats TaskScheduler::getStats(TaskCore core)
{
    if (core == TaskCore::ANY)
    {
        return {}; // Return empty stats for ANY
    }
    return coreStats[static_cast<int>(core)];
}

std::vector<TaskInfo *> TaskScheduler::getRunningTasks()
{
    std::vector<TaskInfo *> runningTasks;
    vSemaphoreTake(schedulerMutex, portMAX_DELAY);

    for (auto task : tasks)
    {
        if (task->isRunning)
        {
            runningTasks.push_back(task);
        }
    }

    vSemaphoreGive(schedulerMutex);
    return runningTasks;
}

TaskInfo *TaskScheduler::getTaskInfo(const std::string &name)
{
    vSemaphoreTake(schedulerMutex, portMAX_DELAY);

    for (auto task : tasks)
    {
        if (task->name == name)
        {
            vSemaphoreGive(schedulerMutex);
            return task;
        }
    }

    vSemaphoreGive(schedulerMutex);
    return nullptr;
}
