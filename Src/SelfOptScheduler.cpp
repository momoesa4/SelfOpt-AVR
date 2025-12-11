#include "SelfOptScheduler.h"

// ------------------- Constructor -------------------
SelfOptScheduler::SelfOptScheduler(uint8_t maxTasks)
    : maxTasks(maxTasks),
      taskCount(0),
      adaptiveEnabled(false),
      memoryProtect(false),
      eventCb(nullptr),
      lastStatusPrint(0),
      statusInterval(2000)
{
    tasks = new Task[maxTasks];
}

// ------------------- Register Task -------------------
bool SelfOptScheduler::registerTask(const char* name, task_fn_t fn, uint32_t period)
{
    if (taskCount >= maxTasks) return false;

    tasks[taskCount] = {
        name,
        fn,
        period,
        millis(),
        0,
        0,
        true
    };

    taskCount++;
    return true;
}

// ------------------- Runtime Measurement -------------------
uint32_t SelfOptScheduler::measureRuntime(task_fn_t fn)
{
    uint32_t start = micros();
    fn();
    return micros() - start;
}

// ------------------- Adaptive Tuning -------------------
void SelfOptScheduler::adaptTask(Task& t)
{
    if (!adaptiveEnabled) return;

    // EWMA filter
    if (t.ewma_runtime_us == 0)
        t.ewma_runtime_us = t.runs;
    else
        t.ewma_runtime_us = (t.ewma_runtime_us * 7 + t.runs) / 8;

    // Example: if the task becomes heavy -> increase period
    if (t.ewma_runtime_us > 800)
        t.period_ms += 5;

    // Too light -> decrease period
    if (t.ewma_runtime_us < 150 && t.period_ms > 10)
        t.period_ms -= 1;
}

// ------------------- Scheduler Core -------------------
void SelfOptScheduler::run()
{
    uint32_t now = millis();

    for (uint8_t i = 0; i < taskCount; i++)
    {
        Task& t = tasks[i];

        if (!t.enabled) continue;

        if (now - t.last_run >= t.period_ms)
        {
            t.last_run = now;

            uint32_t runtime = measureRuntime(t.fn);
            t.runs = runtime;

            adaptTask(t);

            if (memoryProtect && getFreeMemory() < 200)
                t.enabled = false;
        }
    }

    if (millis() - lastStatusPrint >= statusInterval)
    {
        printStatus();
        lastStatusPrint = millis();
    }
}

// ------------------- CPU Load -------------------
uint16_t SelfOptScheduler::getCpuLoad() const
{
    uint32_t total = 0;
    for (uint8_t i = 0; i < taskCount; i++)
        total += tasks[i].ewma_runtime_us;

    return total / 10;
}

// ------------------- Free Memory -------------------
extern char *__brkval;
extern char __bss_end;

int SelfOptScheduler::getFreeMemory() const
{
    char top;
    return &top - (__brkval ? __brkval : &__bss_end);
}

// ------------------- Features -------------------
void SelfOptScheduler::enableAdaptiveTuning(bool e) { adaptiveEnabled = e; }
void SelfOptScheduler::enableMemoryProtection(bool e) { memoryProtect = e; }
void SelfOptScheduler::setEventCallback(event_callback_t cb) { eventCb = cb; }
void SelfOptScheduler::setStatusInterval(uint32_t ms) { statusInterval = ms; }

void SelfOptScheduler::sendEvent(const char* evt, const char* details)
{
    if (eventCb) eventCb(evt, details);
}

// ------------------- Serial Commands -------------------
void SelfOptScheduler::handleSerialCommands()
{
    if (!Serial.available()) return;

    char c = Serial.read();

    switch (c)
    {
        case 's': printStatus(); break;
        case 'd': printDetails(); break;
        case 'l': listTasks(); break;
        case 'r': sendEvent("system", "reset called"); break;
        case 'c':
            Serial.print("CPU Load: ");
            Serial.print(getCpuLoad());
            Serial.println("%");
            break;

        case 'm':
            Serial.print("Free Memory: ");
            Serial.println(getFreeMemory());
            break;
    }
}

// ------------------- Status Print -------------------
void SelfOptScheduler::printStatus()
{
    Serial.print("[STATUS] CPU=");
    Serial.print(getCpuLoad());
    Serial.print("% | Free=");
    Serial.print(getFreeMemory());
    Serial.println(" bytes");
}

// ------------------- Detailed Status -------------------
void SelfOptScheduler::printDetails()
{
    Serial.println("=== Scheduler Details ===");
    for (uint8_t i = 0; i < taskCount; i++)
    {
        Task& t = tasks[i];
        Serial.print("Task ");
        Serial.print(t.name);
        Serial.print(" | period=");
        Serial.print(t.period_ms);
        Serial.print(" | runtime=");
        Serial.print(t.ewma_runtime_us);
        Serial.print(" | enabled=");
        Serial.println(t.enabled ? "yes" : "no");
    }
}

// ------------------- Task List -------------------
void SelfOptScheduler::listTasks()
{
    Serial.println("Registered tasks:");
    for (uint8_t i = 0; i < taskCount; i++)
    {
        Serial.print("- ");
        Serial.println(tasks[i].name);
    }
}
