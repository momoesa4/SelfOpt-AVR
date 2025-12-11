#include "SelfOptScheduler.h"

// AVR free memory helpers (works on many Arduino cores)
#if defined(ARDUINO_ARCH_AVR)
extern char __heap_start;
extern char *__brkval;
#endif

SelfOptScheduler::SelfOptScheduler(uint8_t maxTasks_)
    : maxTasks(maxTasks_),
      taskCount(0),
      adaptiveEnabled(true),
      memProtectEnabled(false),
      statusIntervalMs(2000),
      lastStatusMs(0),
      totalTaskTimeWindowUs(0),
      lastWindowMs(0),
      cpuLoadEWMA(0.0f),
      eventCallback(nullptr)
{
    tasks = (Task*)malloc(sizeof(Task) * maxTasks);
    memset(tasks, 0, sizeof(Task) * maxTasks);
}

void SelfOptScheduler::begin() {
    ensureInit();
    lastWindowMs = millis();
    lastStatusMs = millis();
    // initial event
    postEvent("SYSTEM", "Scheduler started");
}

void SelfOptScheduler::ensureInit() {
    if (!tasks) {
        tasks = (Task*)malloc(sizeof(Task) * maxTasks);
        memset(tasks, 0, sizeof(Task) * maxTasks);
    }
}

int8_t SelfOptScheduler::registerTask(const char* name, task_fn_t fn, uint32_t period_ms, uint8_t priority) {
    ensureInit();
    if (taskCount >= maxTasks) {
        postEvent("ERROR", "Max tasks reached");
        return -1;
    }
    Task &t = tasks[taskCount];
    memset(&t, 0, sizeof(Task));
    strncpy(t.name, name, SOS_NAME_LEN - 1);
    t.fn = fn;
    t.period_ms = period_ms;
    t.base_period_ms = period_ms;
    t.last_run_ms = 0;
    t.ewma_runtime_us = 0;
    t.runs = 0;
    t.enabled = true;
    t.priority = priority;
    taskCount++;
    char buf[64];
    snprintf(buf, sizeof(buf), "Registered task '%s' id=%d period=%lu", t.name, taskCount - 1, (unsigned long)period_ms);
    postEvent("TASK_REGISTER", buf);
    return taskCount - 1;
}

void SelfOptScheduler::addTask(task_fn_t fn, uint32_t period_ms, uint8_t priority) {
    // deprecated compatibility wrapper: generate a default name
    char tmp[12];
    snprintf(tmp, sizeof(tmp), "task%d", taskCount);
    registerTask(tmp, fn, period_ms, priority);
}

void SelfOptScheduler::enableTask(uint8_t id) {
    if (id >= taskCount) return;
    tasks[id].enabled = true;
    postEvent("TASK_ENABLE", tasks[id].name);
}

void SelfOptScheduler::disableTask(uint8_t id) {
    if (id >= taskCount) return;
    tasks[id].enabled = false;
    postEvent("TASK_DISABLE", tasks[id].name);
}

void SelfOptScheduler::toggleTask(uint8_t id) {
    if (id >= taskCount) return;
    tasks[id].enabled = !tasks[id].enabled;
    postEvent("TASK_TOGGLE", tasks[id].name);
}

void SelfOptScheduler::resetAdaptiveSystem() {
    for (uint8_t i = 0; i < taskCount; ++i) {
        tasks[i].period_ms = tasks[i].base_period_ms;
        tasks[i].ewma_runtime_us = 0;
        tasks[i].runs = 0;
    }
    totalTaskTimeWindowUs = 0;
    cpuLoadEWMA = 0.0f;
    postEvent("ADAPTIVE_RESET", "Reset tuning and stats");
}

void SelfOptScheduler::enableAdaptiveTuning(bool en) {
    adaptiveEnabled = en;
    postEvent("ADAPTIVE", en ? "enabled" : "disabled");
}

void SelfOptScheduler::enableMemoryProtection(bool en) {
    memProtectEnabled = en;
    postEvent("MEM_PROTECT", en ? "enabled" : "disabled");
}

void SelfOptScheduler::setStatusInterval(uint32_t ms) {
    statusIntervalMs = ms;
}

void SelfOptScheduler::setEventCallback(event_cb_t cb) {
    eventCallback = cb;
}

uint32_t SelfOptScheduler::measureRuntimeUs(task_fn_t fn) {
    uint32_t t0 = micros();
    fn();
    uint32_t t1 = micros();
    // handle micros() overflow: micro difference unsigned wrap works
    return (uint32_t)(t1 - t0);
}

void SelfOptScheduler::updateTaskStats(Task& t, uint32_t runtimeUs) {
    // EWMA: new = alpha*sample + (1-alpha)*old. alpha = 0.2 (approx)
    const float alpha = 0.2f;
    if (t.runs == 0) {
        t.ewma_runtime_us = runtimeUs;
    } else {
        t.ewma_runtime_us = (uint32_t)((alpha * runtimeUs) + ((1.0f - alpha) * (float)t.ewma_runtime_us));
    }
    t.runs++;
}

void SelfOptScheduler::adaptiveTune(Task &t) {
    if (!adaptiveEnabled) return;
    // Simple tuning policy:
    // target utilization per task = base_time / period
    // Conservative adjust: if runtime > 70% of period -> increase period by 25%
    // if runtime < 10% of period -> try decrease period by 10% (but not below base)
    uint32_t avgRunUs = t.ewma_runtime_us ? t.ewma_runtime_us : 0;
    uint32_t avgRunMs = avgRunUs / 1000;
    if (avgRunMs == 0) return;

    float runFrac = (float)avgRunMs / (float) t.period_ms; // fraction of period spent running (very rough)
    if (runFrac > 0.7f) {
        // increase period (back off)
        uint32_t newp = (uint32_t)((float)t.period_ms * 1.25f) + 1;
        if (newp > t.period_ms) {
            t.period_ms = newp;
            char buf[64];
            snprintf(buf, sizeof(buf), "Tuned UP '%s' -> %lu ms", t.name, (unsigned long)t.period_ms);
            postEvent("TUNE_UP", buf);
        }
    } else if (runFrac < 0.10f) {
        // try to make it more responsive, but don't go below base_period_ms/2
        uint32_t newp = (uint32_t)((float)t.period_ms * 0.9f);
        uint32_t minp = t.base_period_ms > 2 ? t.base_period_ms / 2 : 1;
        if (newp >= minp && newp < t.period_ms) {
            t.period_ms = newp;
            char buf[64];
            snprintf(buf, sizeof(buf), "Tuned DOWN '%s' -> %lu ms", t.name, (unsigned long)t.period_ms);
            postEvent("TUNE_DOWN", buf);
        }
    }
}

void SelfOptScheduler::maybeProtectMemory() {
    if (!memProtectEnabled) return;
    int free = getFreeMemory();
    // simple protection: if free < 300 bytes -> disable lower-priority tasks until free recovers
    if (free < 300) {
        for (uint8_t i = 0; i < taskCount; ++i) {
            if (tasks[i].priority > 1 && tasks[i].enabled) {
                tasks[i].enabled = false;
                char buf[64];
                snprintf(buf, sizeof(buf), "Disabled low-priority '%s' (mem=%d)", tasks[i].name, free);
                postEvent("MEM_ACTION", buf);
            }
        }
    }
}

void SelfOptScheduler::postEvent(const char* ev, const char* details) {
    if (eventCallback) eventCallback(ev, details);
    // also print to Serial for convenience
    if (Serial) {
        Serial.print("[EVENT] ");
        Serial.print(ev);
        Serial.print(": ");
        Serial.println(details);
    }
}

void SelfOptScheduler::run() {
    ensureInit();
    uint32_t now = millis();

    // iterate tasks; simple round-robin respecting enabled flag and scheduling period
    for (uint8_t i = 0; i < taskCount; ++i) {
        Task &t = tasks[i];
        if (!t.enabled || t.fn == nullptr) continue;
        uint32_t elapsed = (now - t.last_run_ms);
        if (t.last_run_ms == 0 || elapsed >= t.period_ms) {
            // measure runtime
            uint32_t ru = measureRuntimeUs(t.fn);
            updateTaskStats(t, ru);
            totalTaskTimeWindowUs += ru;
            t.last_run_ms = now;
            // adaptive tuning occasionally
            if (t.runs % 8 == 0) adaptiveTune(t);
        }
    }

    // update cpu load EWMA every window (e.g., 1000 ms)
    uint32_t wnow = millis();
    uint32_t windowElapsed = (wnow - lastWindowMs);
    if (windowElapsed >= 1000) {
        // cpuLoad = totalTaskTimeWindowUs / (windowElapsed * 1000)
        float rawLoad = 0.0f;
        if (windowElapsed > 0) {
            rawLoad = (float)totalTaskTimeWindowUs / ((float)windowElapsed * 1000.0f) * 100.0f;
            if (rawLoad > 100.0f) rawLoad = 100.0f;
        }
        // smooth it
        cpuLoadEWMA = (cpuEWMA_ALPHA * rawLoad) + ((1.0f - cpuEWMA_ALPHA) * cpuLoadEWMA);
        // reset counters
        totalTaskTimeWindowUs = 0;
        lastWindowMs = wnow;
    }

    // status printing
    if ((uint32_t)(now - lastStatusMs) >= statusIntervalMs) {
        printStatus(Serial);
        lastStatusMs = now;
    }

    // memory protection check
    maybeProtectMemory();
}

float SelfOptScheduler::getCpuLoad() {
    return cpuLoadEWMA;
}

int SelfOptScheduler::freeMemoryInternal() {
#if defined(ARDUINO_ARCH_AVR)
    char top;
    if ((intptr_t)__brkval == 0) {
        return (int)(&top - &__heap_start);
    } else {
        return (int)(&top - __brkval);
    }
#else
    // fallback: small approx using available RAM area
    int v;
    return (int)((char*)&v - (char*)0);
#endif
}

int SelfOptScheduler::getFreeMemory() {
    return freeMemoryInternal();
}

void SelfOptScheduler::printStatus(Stream &out) {
    out.print(F("[STATUS] "));
    out.print(millis());
    out.print(F(" ms | FreeMem: "));
    out.print(getFreeMemory());
    out.print(F(" bytes | CPU: "));
    out.print(getCpuLoad(), 1);
    out.println(F("%"));
}

void SelfOptScheduler::printDetails(Stream &out) {
    out.println(F("=== Scheduler Details ==="));
    out.print(F("Tasks: "));
    out.println(taskCount);
    for (uint8_t i = 0; i < taskCount; ++i) {
        Task &t = tasks[i];
        out.print(i);
        out.print(F(": "));
        out.print(t.name);
        out.print(F(" | en="));
        out.print(t.enabled ? "1" : "0");
        out.print(F(" | p="));
        out.print(t.priority);
        out.print(F(" | period="));
        out.print(t.period_ms);
        out.print(F("ms | runs="));
        out.print(t.runs);
        out.print(F(" | avg_us="));
        out.println(t.ewma_runtime_us);
    }
}

void SelfOptScheduler::handleSerialCommands() {
    while (Serial && Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;
        char cmd = line.charAt(0);
        switch (cmd) {
            case 's': cmdStatus(); break;
            case 'd': cmdDetails(); break;
            case 'l': cmdList(); break;
            case 'm': cmdMemory(); break;
            case 'c': cmdCpu(); break;
            case 'r': cmdReset(); break;
            case 't': {
                // t <id>  OR t <name>
                if (line.length() > 2) {
                    cmdToggle(line.substring(2).c_str());
                }
                break;
            }
            default:
                Serial.print(F("Unknown cmd: "));
                Serial.println(line);
        }
    }
}

void SelfOptScheduler::cmdStatus() {
    printStatus(Serial);
}

void SelfOptScheduler::cmdDetails() {
    printDetails(Serial);
}

void SelfOptScheduler::cmdList() {
    Serial.println(F("Tasks:"));
    for (uint8_t i = 0; i < taskCount; ++i) {
        Serial.print(i);
        Serial.print(F(": "));
        Serial.print(tasks[i].name);
        Serial.print(F(" en="));
        Serial.println(tasks[i].enabled ? "1" : "0");
    }
}

void SelfOptScheduler::cmdMemory() {
    Serial.print(F("Free memory: "));
    Serial.println(getFreeMemory());
}

void SelfOptScheduler::cmdCpu() {
    Serial.print(F("CPU load: "));
    Serial.print(getCpuLoad(), 1);
    Serial.println(F("%"));
}

void SelfOptScheduler::cmdToggle(const char* arg) {
    // try parse numeric id
    int id = -1;
    if (arg == nullptr) return;
    // trim leading spaces
    while (*arg == ' ') ++arg;
    if (*arg == '\0') return;
    bool isNum = true;
    const char* p = arg;
    while (*p) { if (!isDigit(*p)) { isNum = false; break; } p++; }
    if (isNum) id = atoi(arg);
    if (isNum && id >= 0 && id < taskCount) {
        toggleTask((uint8_t)id);
        Serial.print(F("Toggled task id "));
        Serial.println(id);
        return;
    }
    // else try to find by name
    for (uint8_t i = 0; i < taskCount; ++i) {
        if (strncmp(tasks[i].name, arg, SOS_NAME_LEN) == 0) {
            toggleTask(i);
            Serial.print(F("Toggled task "));
            Serial.println(tasks[i].name);
            return;
        }
    }
    Serial.print(F("Task not found: "));
    Serial.println(arg);
}

void SelfOptScheduler::cmdReset() {
    resetAdaptiveSystem();
    Serial.println(F("Adaptive system reset"));
}
