#ifndef SELF_OPT_SCHEDULER_H
#define SELF_OPT_SCHEDULER_H

#include <Arduino.h>

#define SOS_MAX_TASKS 8
#define SOS_NAME_LEN 16

typedef void (*task_fn_t)(void);
typedef void (*event_cb_t)(const char* event, const char* details);

struct Task {
    char name[SOS_NAME_LEN];
    task_fn_t fn;
    uint32_t period_ms;           // desired period (may be tuned)
    uint32_t base_period_ms;      // original requested period
    uint32_t last_run_ms;         // millis() timestamp of last run
    uint32_t ewma_runtime_us;     // EWMA of runtime in microseconds
    uint32_t runs;                // number of times run (for stats)
    bool enabled;
    uint8_t priority;
};

class SelfOptScheduler {
public:
    // constructors
    SelfOptScheduler(uint8_t maxTasks = SOS_MAX_TASKS);

    // lifecycle
    void begin();
    void run();

    // task management
    int8_t registerTask(const char* name, task_fn_t fn, uint32_t period_ms, uint8_t priority = 1);
    void addTask(task_fn_t fn, uint32_t period_ms, uint8_t priority = 1) __attribute__((deprecated));
    void enableTask(uint8_t id);
    void disableTask(uint8_t id);
    void toggleTask(uint8_t id);
    void resetAdaptiveSystem();

    // features toggles
    void enableAdaptiveTuning(bool en);
    void enableMemoryProtection(bool en);
    void setStatusInterval(uint32_t ms);
    void setEventCallback(event_cb_t cb);

    // info
    float getCpuLoad();         // percent 0..100
    int getFreeMemory();        // bytes free (approx)
    void printStatus(Stream &out = Serial);
    void printDetails(Stream &out = Serial);

    // serial commands (should be called regularly from loop)
    void handleSerialCommands();

private:
    Task *tasks;
    uint8_t maxTasks;
    uint8_t taskCount;

    // adaptive/control params
    bool adaptiveEnabled;
    bool memProtectEnabled;
    uint32_t statusIntervalMs;
    uint32_t lastStatusMs;

    // CPU / timing measurement
    uint64_t totalTaskTimeWindowUs; // sum of task runtimes in window
    uint32_t lastWindowMs;
    float cpuLoadEWMA;              // smoothed cpu load %
    const float cpuEWMA_ALPHA = 0.1f;

    // events
    event_cb_t eventCallback;

    // internal helpers
    uint32_t measureRuntimeUs(task_fn_t fn);
    void updateTaskStats(Task &t, uint32_t runtimeUs);
    void adaptiveTune(Task &t);
    void maybeProtectMemory();
    void postEvent(const char* ev, const char* details);
    void ensureInit();

    // serial command helpers
    void cmdStatus();
    void cmdDetails();
    void cmdList();
    void cmdMemory();
    void cmdCpu();
    void cmdToggle(const char* arg);
    void cmdReset();

    // utilities
    int freeMemoryInternal();
};

#endif
