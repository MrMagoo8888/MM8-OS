#include "time.h"
#include "stdio.h" // For printf, if needed for debugging

// External global tick counter from main.c
// This variable is incremented by the timer IRQ handler.
extern volatile uint32_t g_ticks;

// Assuming a timer frequency of 100 Hz (100 ticks per second)
// This value should match how your timer IRQ is configured.
#define TIMER_FREQUENCY_HZ 100

/**
 * @brief Initializes the time module.
 *
 * Currently, the timer IRQ is set up in main.c. This function can be
 * used for future Real-Time Clock (RTC) initialization or other
 * time-related setup.
 */
void time_initialize() {
    // For now, it just prints a message.
    printf("Time module initialized.\n");
}

/**
 * @brief Returns the system uptime in seconds.
 * @return The number of seconds since the system started.
 */
uint32_t get_uptime_seconds() {
    // Avoid division by zero if TIMER_FREQUENCY_HZ is unexpectedly 0.
    if (TIMER_FREQUENCY_HZ == 0) {
        return 0;
    }
    return g_ticks / TIMER_FREQUENCY_HZ;
}

/**
 * @brief Pauses execution for the specified number of milliseconds using busy-waiting.
 * @param milliseconds The number of milliseconds to sleep.
 *
 * This function busy-waits, consuming CPU cycles. In a multi-tasking OS,
 * a more efficient approach would involve yielding the CPU to other tasks.
 */
void sleep_ms(uint32_t milliseconds) {
    if (milliseconds == 0) {
        return;
    }

    // Calculate the target number of ticks. Use uint64_t for intermediate calculation
    // to prevent overflow for large `milliseconds` values before division.
    uint64_t target_ticks_increment = ((uint64_t)milliseconds * TIMER_FREQUENCY_HZ) / 1000;
    if (target_ticks_increment == 0 && milliseconds > 0) {
        target_ticks_increment = 1; // Ensure at least one tick for non-zero milliseconds
    }
    uint32_t target_ticks = g_ticks + (uint32_t)target_ticks_increment;

    while (g_ticks < target_ticks) {
        // Busy-wait: do nothing, just wait for the timer interrupt to increment g_ticks
    }
}

/**
 * @brief Pauses execution for the specified number of seconds using busy-waiting.
 * @param seconds The number of seconds to sleep.
 *
 * This function internally calls `sleep_ms`.
 */
void sleep_seconds(uint32_t seconds) {
    sleep_ms(seconds * 1000);
}