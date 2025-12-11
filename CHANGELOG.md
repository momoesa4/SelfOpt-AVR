#[1.0.0] - Initial Professional Release
Added
Core Scheduler Engine: A lightweight, self-optimizing adaptive task scheduler.

Fixed-Point EWMA Algorithm: Core optimization logic using integer math for efficiency on AVR.

Static Memory Model: No dynamic allocation; all memory is pre-allocated for reliability.

Multi-Platform Support: Portable layer for AVR (ATmega328P), ESP32, and STM32.

Overrun Detection & Stabilization: Hysteresis mechanism to prevent task period oscillation.

Comprehensive Documentation: Full Doxygen-style API reference in docs/API.md.

Practical Examples: Example sketches for Arduino IDE and PlatformIO.

CI Pipeline: Automated compilation checks for Arduino Uno and ESP32.

Notes
This is the first stable, production-ready release of the SelfOpt scheduler.

Designed specifically for deterministic performance on resource-constrained embedded systems.
