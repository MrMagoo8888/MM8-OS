#ifndef TIME_H
#define TIME_H

#include <stdint.h>

// Initializes time-related components (e.g., RTC, if present).
// For now, the system timer is initialized in main.c.
void time_initialize();

// Returns the system uptime in seconds.
uint32_t get_uptime_seconds();

// Pauses execution for the specified number of milliseconds (busy-waits).
void sleep_ms(uint32_t milliseconds);

// Pauses execution for the specified number of seconds (busy-waits).
void sleep_seconds(uint32_t seconds);

#endif // TIME_H