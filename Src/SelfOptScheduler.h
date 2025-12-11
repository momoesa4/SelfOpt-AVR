#ifndef SELF_OPT_SCHEDULER_H
#define SELF_OPT_SCHEDULER_H

#include <Arduino.h>

#define MAX_TASKS 6

typedef void (*task_fn_t)(void);

struct Task {
    task_fn_t fn;
    uint32_t period_ms;
    uint32_t last_run;
    uint32_t ewma_runtime_us;
    uint32_t runs;
    bool enabled;
    uint8_t priority;
};

class SelfOptScheduler {
public:
    SelfOptScheduler();

    void addTask(task_fn_t fn, uint32_t period_ms, uint8_t priority = 1);
    void run();
    void enableTask(uint8_t id);
    void disableTask(uint8_t id);
    void resetAdaptiveSystem();

private:
    Task tasks[MAX_TASKS];
    uint8_t taskCount;

    void updatePeriod(Task& t, uint32_t runtime);
    uint32_t measureRuntime(task_fn_t fn);
};

#endif
