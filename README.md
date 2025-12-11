# SelfOpt — Self-Optimizing Adaptive Scheduler (AVR/ESP32/STM32)

SelfOpt is a lightweight, production-oriented adaptive scheduler for microcontrollers.
It measures task runtimes and adapts task periods using a fixed-point EWMA. Designed
for constrained devices (ATmega328P) but portable to other MCUs (ESP32, STM32) via a
small portability layer.

**Highlights**
- No dynamic allocation (static allocation only).
- Fixed-point EWMA (no floats on AVR).
- Overrun detection and hysteresis to avoid oscillation.
- Per-task configuration (alpha, min/max period, priority hint).
- API documented with Doxygen-style comments.
- Examples for Arduino IDE and PlatformIO.
- CI: compile checks for Arduino Uno (AVR) and ESP32.

See `docs/API.md` for full API and integration notes.
