#include "SelfOptScheduler.h"

#define EWMA_ALPHA_NUM 1
#define EWMA_ALPHA_DEN 4
#define TARGET_RUNTIME_US 3000
#define PERIOD_MIN_MS 20
#define PERIOD_MAX_MS 5000

SelfOptScheduler::SelfOptScheduler() {
    taskCount = 0;
}

void SelfOptScheduler::addTask(task_fn_t fn, uint32_t period_ms, uint8_t priority) {
    if (taskCount >= MAX_TASKS) return;

    tasks[taskCount] = { fn, period_ms, 0, 0, 0, true, priority };
    taskCount++;
}

void SelfOptScheduler::enableTask(uint8_t id) {
    if (id < taskCount) tasks[id].enabled = true;
}

void SelfOptScheduler::disableTask(uint8_t id) {
    if (id < taskCount) tasks[id].enabled = false;
}

uint32_t SelfOptScheduler::measureRuntime(task_fn_t fn) {
    uint32_t start = micros();
    fn();
    return micros() - start;
}

void SelfOptScheduler::updatePeriod(Task& t, uint32_t rt) {
    t.ewma_runtime_us = ((t.ewma_runtime_us * (EWMA_ALPHA_DEN - EWMA_ALPHA_NUM)) +
                         (rt * EWMA_ALPHA_NUM)) / EWMA_ALPHA_DEN;

    float loadRatio = (float)t.ewma_runtime_us / (float)TARGET_RUNTIME_US;

    if (loadRatio < 0.8f) {
        t.period_ms = min((uint32_t)(t.period_ms * 1.05f), PERIOD_MAX_MS);
    } 
    else if (loadRatio > 1.2f) {
        t.period_ms = max((uint32_t)(t.period_ms * 0.85f), PERIOD_MIN_MS);
    }
}

void SelfOptScheduler::resetAdaptiveSystem() {
    for (uint8_t i = 0; i < taskCount; i++) {
        tasks[i].ewma_runtime_us = 0;
        tasks[i].runs = 0;
    }
}

void SelfOptScheduler::run() {
    uint32_t now = millis();

    for (uint8_t i = 0; i < taskCount; i++) {
        Task& t = tasks[i];

        if (!t.enabled) continue;

        if (now - t.last_run >= t.period_ms) {
            uint32_t rt = measureRuntime(t.fn);

            t.last_run = now;
            t.runs++;

            updatePeriod(t, rt);
        }
    }
}
