# Changelog

All notable changes to the SelfOpt-AVR (Self-Optimizing Adaptive Scheduler) project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.0.0] - Initial Professional Release

### Release Overview
This is the first stable and official release of the SelfOpt scheduler. It marks the transition of the core idea into a production-ready, lightweight, and adaptive task scheduler designed for resource-constrained embedded systems (AVR, ESP32, STM32). The library is now structured, fully documented, and ready for community use.

### ✨ Added
- **Core Scheduler Engine**: A lightweight, self-optimizing adaptive task scheduler that dynamically adjusts task periods at runtime.
- **Fixed-Point EWMA Algorithm**: The core optimization logic uses efficient integer-based calculations, eliminating the need for floating-point operations on microcontrollers like the ATmega328P.
- **Static & Safe Memory Model**: The library uses static allocation only. No dynamic memory allocation, ensuring predictable memory usage and enhanced reliability for embedded environments.
- **Multi-Platform Portability Layer**: A hardware abstraction layer enables support for **AVR (Arduino Uno)**, **ESP32**, and **STM32** architectures from a single codebase.
- **Runtime Stability Features**:
  - **Overrun Detection**: Identifies tasks that exceed their allotted execution time.
  - **Adaptive Stabilization (Hysteresis)**: Integrated hysteresis mechanism to prevent rapid oscillation of task periods and ensure stable system behavior.
- **Professional Documentation**:
  - Complete **Doxygen-style API reference** in `/docs/API.md`.
  - Overhauled and informative `README.md` with clear usage guidelines.
- **Expanded Examples**:
  - Practical example sketches for both **Arduino IDE** and **PlatformIO** environments, demonstrating basic and advanced use cases.
- **Development Infrastructure**:
  - **Continuous Integration (CI) Pipeline**: Automated compilation checks (via GitHub Actions or similar) for **Arduino Uno (AVR)** and **ESP32** targets to ensure build stability.
  - Established project structure with `/src`, `/examples`, `/docs`, and `/tests` directories.

### ⚠️ Notes for Users
- As the first major release, this version establishes the stable public API. Future changes will follow semantic versioning.
- This library is specifically designed for developers needing deterministic, efficient task scheduling on microcontrollers with limited resources.
- Users are encouraged to review the `docs/API.md` and the provided examples for integration guidance.

---

*This changelog starts with version 1.0.0, which represents the first packaged release of the project.*
