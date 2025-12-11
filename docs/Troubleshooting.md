# Troubleshooting & Best Practices

- If a task uses delay() internally, it will block. Prefer non-blocking work or split into smaller steps.
- Avoid heavy allocations or long-running blocking operations in callbacks.
- Use conservative alpha (e.g., 128..512) on noisy workloads.
- If using on ESP32/FreeRTOS, ensure callbacks are safe for the chosen core and do not call blocking RTOS APIs unless desired.
