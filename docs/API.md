# API Reference — SelfOpt Scheduler

## Overview
SelfOptScheduler provides a compact API to register periodic tasks and let the scheduler
measure runtimes and adapt the task periods.

### Key concepts
- **Task**: registered callback with a target period.
- **EWMA**: fixed-point exponentially-weighted moving average (Q10 scaling).
- **Overrun**: when task runtime approaches or exceeds its scheduled period.

## Main classes / functions

### SelfOptScheduler(uint8_t maxTasks = 8)
Create scheduler instance with statically allocated slot count.

### int8_t addTask(void (*cb)(void*), void* ctx, uint32_t period_ms, uint32_t min_period_ms = 1, uint32_t max_period_ms = 60000, uint16_t alpha_q = 256)
Register a task. Returns task id or -1 on failure.

### bool enableTask(int8_t id), disableTask(int8_t id)
Enable/disable task.

### void run()
Call repeatedly from `loop()` to run scheduler.

### void printStats(Stream &out)
Print human-readable telemetry.

### uint32_t getEstimatedRuntimeUs(int8_t id)
Return estimated runtime in microseconds.

### Notes on fixed-point
EWMA values are stored scaled by Q = 1024 (2^10). alpha_q ranges 0..1024.

