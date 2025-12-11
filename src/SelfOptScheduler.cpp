#include "SelfOptScheduler.h"
#include <Arduino.h>

// Constructor
SelfOptScheduler::SelfOptScheduler(uint8_t maxTasks)
: capacity(maxTasks), count(0)
{
  // allocate statically sized array on heap once to avoid repeated allocations
  tasks = (Task*)malloc(sizeof(Task) * capacity);
  // initialize
  for (uint8_t i=0;i<capacity;i++){
    tasks[i].cb = nullptr;
    tasks[i].ctx = nullptr;
    tasks[i].last_run_at_us = 0;
    tasks[i].period_ms = 0;
    tasks[i].min_period_ms = 1;
    tasks[i].max_period_ms = 60000;
    tasks[i].ewma_runtime_q = 0;
    tasks[i].alpha_q = 256;
    tasks[i].active = false;
  }
}

// addTask
int8_t SelfOptScheduler::addTask(SelfOptTaskCb cb, void* ctx, uint32_t period_ms,
                 uint32_t min_period_ms, uint32_t max_period_ms, uint16_t alpha_q)
{
  if (!cb) return -1;
  if (count >= capacity) return -1;
  uint8_t idx = count++;
  tasks[idx].cb = cb;
  tasks[idx].ctx = ctx;
  tasks[idx].period_ms = max(min(period_ms, max_period_ms), min_period_ms);
  tasks[idx].min_period_ms = min_period_ms;
  tasks[idx].max_period_ms = max_period_ms;
  tasks[idx].last_run_at_us = now_us();
  tasks[idx].ewma_runtime_q = 0;
  tasks[idx].alpha_q = alpha_q > SCALE ? SCALE : alpha_q;
  tasks[idx].active = true;
  return (int8_t)idx;
}

bool SelfOptScheduler::enableTask(int8_t id){
  if (id < 0 || id >= count) return false;
  tasks[id].active = true;
  return true;
}
bool SelfOptScheduler::disableTask(int8_t id){
  if (id < 0 || id >= count) return false;
  tasks[id].active = false;
  return true;
}

uint32_t SelfOptScheduler::now_us(){
  return SELFOPT_NOW_US();
}

void SelfOptScheduler::updateEwma(uint8_t idx, uint32_t runtime_us){
  // cap runtime to prevent overflow
  if (runtime_us > MAX_EST_RUNTIME_US) runtime_us = MAX_EST_RUNTIME_US;
  uint32_t runtime_q = runtime_us * SCALE;
  uint32_t old_q = tasks[idx].ewma_runtime_q;
  uint32_t alpha_q = tasks[idx].alpha_q;
  uint32_t new_q = (alpha_q * runtime_q + (SCALE - alpha_q) * old_q) / SCALE;
  tasks[idx].ewma_runtime_q = new_q;
}

void SelfOptScheduler::adjustPeriodIfNeeded(uint8_t idx, uint32_t est_runtime_us){
  // convert period to microseconds
  uint32_t period_us = tasks[idx].period_ms * 1000UL;
  // hysteresis thresholds (percent)
  const uint8_t high_pct = 70; // if runtime > 70% => expand
  const uint8_t low_pct = 30;  // if runtime < 30% => shrink
  // compute condition safely
  if (est_runtime_us * 100 > period_us * high_pct) {
    // increase period by +20% with cap
    uint32_t add = max((uint32_t)1, tasks[idx].period_ms / 5);
    uint32_t np = tasks[idx].period_ms + add;
    if (np > tasks[idx].max_period_ms) np = tasks[idx].max_period_ms;
    tasks[idx].period_ms = np;
  } else if (est_runtime_us * 100 < period_us * low_pct) {
    // decrease period by 10% but not below min
    uint32_t dec = max((uint32_t)1, tasks[idx].period_ms / 10);
    uint32_t np = tasks[idx].period_ms > dec ? (tasks[idx].period_ms - dec) : tasks[idx].min_period_ms;
    if (np < tasks[idx].min_period_ms) np = tasks[idx].min_period_ms;
    tasks[idx].period_ms = np;
  }
}

void SelfOptScheduler::run(){
  uint32_t now = now_us();
  for (uint8_t i=0;i<count;i++){
    if (!tasks[i].active) continue;
    uint32_t elapsed_ms = (now - tasks[i].last_run_at_us) / 1000UL;
    if (elapsed_ms >= tasks[i].period_ms){
      uint32_t start = now_us();
      // call task
      tasks[i].cb(tasks[i].ctx);
      uint32_t end = now_us();
      tasks[i].last_run_at_us = now;
      uint32_t runtime = (end >= start) ? (end - start) : (UINT32_MAX - start + end);
      updateEwma(i, runtime);
      uint32_t est_runtime = tasks[i].ewma_runtime_q / SCALE;
      adjustPeriodIfNeeded(i, est_runtime);
      SELFOPT_YIELD();
    }
  }
}

void SelfOptScheduler::printStats(Stream &out){
  out.println(F("=== SelfOpt Scheduler Stats ==="));
  for (uint8_t i=0;i<count;i++){
    if (!tasks[i].active) continue;
    uint32_t est = tasks[i].ewma_runtime_q / SCALE;
    out.print(F("Task ")); out.print(i);
    out.print(F(" | period(ms): ")); out.print(tasks[i].period_ms);
    out.print(F(" | est(us): ")); out.print(est);
    out.print(F(" | min: ")); out.print(tasks[i].min_period_ms);
    out.print(F(" | max: ")); out.print(tasks[i].max_period_ms);
    out.println();
  }
}

SelfOptScheduler::Stats SelfOptScheduler::getStats(int8_t id){
  Stats s = {0,0,0,false};
  if (id < 0 || id >= count) return s;
  s.est_runtime_us = tasks[id].ewma_runtime_q / SCALE;
  s.last_run_at_us = tasks[id].last_run_at_us;
  s.period_ms = tasks[id].period_ms;
  s.active = tasks[id].active;
  return s;
}
