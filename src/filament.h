#ifndef ORBITER_FILAMENT_H
#define ORBITER_FILAMENT_H
#include <stdbool.h>
#include <stdint.h>
#include "config.h"

/* TCB at 1 kHz from the configured 16/20-MHz CPU oscillator.
 * First changed sample starts the interval; it never consumes its first tick.
 * Pulses entirely between samples cannot be observed by a sampled filter. */
typedef struct {
    uint8_t remaining_ticks;
    bool candidate;
    volatile bool present; /* ISR publishes one atomic byte to foreground. */
} filament_t;

static inline bool filament_tick(filament_t* filament, bool present) {
    uint8_t debounce_ticks = (uint8_t) (present ? INSERT_DEBOUNCE_MS : REMOVE_DEBOUNCE_MS);
    if (debounce_ticks == 0) {
        filament->present = filament->candidate = present;
        filament->remaining_ticks = 0;
    } else if (present != filament->candidate) {
        filament->candidate = present;
        filament->remaining_ticks = debounce_ticks;
    } else if (filament->remaining_ticks != 0) {
        --filament->remaining_ticks;
        if (filament->remaining_ticks == 0) {
            filament->present = present;
        }
    }
    return filament->present;
}
#endif
