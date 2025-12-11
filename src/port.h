#ifndef SELFOPT_PORT_H
#define SELFOPT_PORT_H

// Portability layer: adapt macros for different platforms.
// Default uses Arduino's micros() and yield().

#include <Arduino.h>

// Provide SELFOPT_YIELD() and SELFOPT_NOW_US()
#ifndef SELFOPT_NOW_US
#define SELFOPT_NOW_US() (micros())
#endif

#ifndef SELFOPT_YIELD
#define SELFOPT_YIELD() (yield())
#endif

#endif
