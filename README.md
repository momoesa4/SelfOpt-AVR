# SelfOptScheduler

Self-Optimizing Scheduler for Arduino  
Dynamic EWMA-based runtime tuning, adaptive period control, and energy-aware scheduling.

## Features
- Auto-adjusting task periods  
- Runtime measurement  
- EWMA smoothing  
- Priority weighting  
- Lightweight (no RTOS required)  
- Works on AVR, ESP32, STM32  

## Example

```cpp
scheduler.addTask(blink, 100);
scheduler.addTask(printStatus, 1000);
