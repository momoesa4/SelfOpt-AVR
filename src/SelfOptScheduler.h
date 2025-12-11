#ifndef SELFOPT_SCHEDULER_H
#define SELFOPT_SCHEDULER_H

#include <Arduino.h>
#include "port.h"

/**
 * @file SelfOptScheduler.h
 * @brief Adaptive fixed-point scheduler with runtime EWMA tracking.
 *
 * Designed for resource-constrained MCUs: avoids floating point and dynamic allocation.
 */

/**
 * Type of task callback: receives opaque user context pointer.
 */
typedef void (*SelfOptTaskCb)(void* ctx);

class SelfOptScheduler {
public:
  struct Stats {
    uint32_t est_runtime_us;
    uint32_t last_run_at_us;
    uint32_t period_ms;
    bool active;
  };

  /**
   * Construct scheduler with statically allocated capacity.
   * @param maxTasks Maximum number of tasks supported.
   */
  SelfOptScheduler(uint8_t maxTasks = 8);

  /**
   * Register a periodic task.
   * @param cb Callback function pointer.
   * @param ctx Opaque user context pointer (may be NULL).
   * @param period_ms Initial target period in ms.
   * @param min_period_ms Minimum allowed period in ms (default 1).
   * @param max_period_ms Maximum allowed period in ms (default 60000).
   * @param alpha_q EWMA smoothing factor in Q10 (0..1024). Typical 256 (~0.25).
   * @return task id (0..maxTasks-1) or -1 if no slot available.
   */
  int8_t addTask(SelfOptTaskCb cb, void* ctx, uint32_t period_ms,
                 uint32_t min_period_ms = 1, uint32_t max_period_ms = 60000,
                 uint16_t alpha_q = 256);

  bool enableTask(int8_t id);
  bool disableTask(int8_t id);

  /**
   * Call repeatedly from main loop to dispatch tasks.
   */
  void run();

  /**
   * Print human-readable stats to a Stream (Serial).
   */
  void printStats(Stream &out);

  /**
   * Retrieve stats for a task.
   */
  Stats getStats(int8_t id);

private:
  struct Task {
    SelfOptTaskCb cb;
    void* ctx;
    uint32_t last_run_at_us;
    uint32_t period_ms;
    uint32_t min_period_ms;
    uint32_t max_period_ms;
    // EWMA in Q10 scaling (value = ewma_q / SCALE)
    uint32_t ewma_runtime_q;
    uint16_t alpha_q;
    bool active;
    uint8_t reserved;
  };

  Task* tasks;
  uint8_t capacity;
  uint8_t count;

  static const uint16_t SCALE = 1024; // Q10
  static const uint32_t MAX_EST_RUNTIME_US = 2000000UL; // 2s cap

  uint32_t now_us();
  void adjustPeriodIfNeeded(uint8_t idx, uint32_t est_runtime_us);
  void updateEwma(uint8_t idx, uint32_t runtime_us);
};

#endif
