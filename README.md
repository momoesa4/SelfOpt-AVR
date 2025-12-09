# SelfOpt-AVR
**Self-Optimizing Adaptive Firmware for ATmega328P (Arduino Uno)**

A tiny (~350 lines), zero-allocation cooperative scheduler that automatically:
- Profiles task runtime using EWMA
- Adapts task periods under load
- Disables non-critical tasks when RAM is low
- Prevents system hangs forever
## Features
- Less than 200 bytes RAM overhead
- No dynamic allocation
- Full Arduino IDE compatible
- Real self-healing behavior on 8-bit AVR

Just open in Arduino IDE → Upload → Open Serial Monitor (115200 baud) → type `s` to see magic!

Author: Mohamed Islam (17 y/o) - Cairo, Egypt
License: MIT
