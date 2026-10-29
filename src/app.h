#ifndef ORBITER_APP_H
#define ORBITER_APP_H

#include <stdbool.h>
#include <stdint.h>
#include "leds.h"

enum app_phase {
    APP_SAMPLE_BUTTONS,
    APP_UPDATE_OUTPUTS
};

enum app_state {
    APP_EMPTY,
    APP_PRESENT,
    APP_UNLOAD,
    APP_TANGLE,
    APP_PROGRAMMING
};

typedef struct {
    uint8_t count;
    bool qualified;
} button_filter_t;

typedef struct {
    button_filter_t unload;
    button_filter_t tangle;
    button_filter_t programming;
    uint8_t tick_phase;
    uint8_t state;
    uint8_t blink_ticks;
    bool filament_present;
    bool unload_tangle_asserted;
    bool unload_tangle_output_enabled;
    rgb_t color;
} app_t;

static inline bool app_cancels_startup(const app_t* application) {
    return application->state == APP_UNLOAD || application->state == APP_TANGLE
        || application->state == APP_PROGRAMMING;
}

/* One 128-Hz RTC slot. Filament is already qualified by the fast ISR.
 * Recovery ignores button/state transitions and flashes white with FTU disabled. */
void app_tick(app_t* application, bool filament_present, uint8_t button_reading, bool recovering);

#endif
