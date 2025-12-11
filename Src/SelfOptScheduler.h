#ifndef SELF_OPT_SCHEDULER_H
#define SELF_OPT_SCHEDULER_H

#include <Arduino.h>

#define MAX_TASKS 8

typedef void (*task_fn_t)(void);
typedef void (*event_callback_t)(const char*, const char*);

struct Task {
    const char* name;
    task_fn_t fn;
    uint32_t period_ms;
    uint32_t last_run;
    uint32_t ewma_runtime_us;
    uint32_t runs;
    bool enabled;
};

class SelfOptScheduler {
public:
    SelfOptScheduler(uint8_t maxTasks = MAX_TASKS);

    bool registerTask(const char* name, task_fn_t fn, uint32_t period);
    void run();

    // Diagnostics
    uint16_t getCpuLoad() const;
    int getFreeMemory() const;

    // Features
    void enableAdaptiveTuning(bool e);
    void enableMemoryProtection(bool e);
    void setEventCallback(event_callback_t cb);
    void setStatusInterval(uint32_t ms);

    // Serial commands
    void handleSerialCommands();

private:
    Task* tasks;
    uint8_t maxTasks;
    uint8_t taskCount;

    // System
    event_callback_t eventCb;
    bool adaptiveEnabled;
    bool memoryProtect;
    uint32_t lastStatusPrint;
    uint32_t statusInterval;

    // Internal
    uint32_t measureRuntime(task_fn_t fn);
    void adaptTask(Task& t);
    void sendEvent(const char* evt, const char* details);
    void printStatus();
    void printDetails();
    void listTasks();
};

#endif
