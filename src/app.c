#include "app.h"
#include "config.h"
#include "buttons.h"

enum button {
    BUTTON_NONE,
    BUTTON_UNLOAD,
    BUTTON_TANGLE,
    BUTTON_BOTH
};
enum {
    BLINK_PERIOD = 61,
    BLINK_ON = 30,
    PROGRAMMING_WHITE = 100,
    TANGLE_FILTER_LIMIT = 64,
    PROGRAMMING_HOLD_LIMIT = 250
};

/* Active samples increase the count; inactive samples gradually reduce it.
 * Brief interruptions retain accumulated evidence. Qualification clears at one. */
static void update_button_filter(button_filter_t* filter, bool active, uint8_t limit) {
    if (active) {
        ++filter->count;
        if (filter->count > limit) {
            filter->count = limit;
            filter->qualified = true;
        }
    } else if (filter->count != 0) {
        --filter->count;
    }
    if (filter->count == 1) {
        filter->qualified = false;
    }
}

/* The programming gesture requires consecutive active samples; a release resets the hold. */
static void update_programming_hold(button_filter_t* filter, bool active) {
    if (active) {
        ++filter->count;
        if (filter->count > PROGRAMMING_HOLD_LIMIT) {
            filter->count = PROGRAMMING_HOLD_LIMIT;
            filter->qualified = true;
        }
    } else {
        filter->count = 0;
    }
    if (filter->count == 0) {
        filter->qualified = false;
    }
}

static enum button classify_button(uint8_t button_reading) {
    if (button_reading >= BUTTON_TANGLE_MIN && button_reading < BUTTON_TANGLE_END) {
        return BUTTON_TANGLE;
    }
    if (button_reading >= BUTTON_UNLOAD_MIN && button_reading < BUTTON_UNLOAD_END) {
        return BUTTON_UNLOAD;
    }
    if (button_reading >= BUTTON_BOTH_MIN && button_reading < BUTTON_TANGLE_MIN) {
        return BUTTON_BOTH;
    }
    return BUTTON_NONE;
}

static void advance_blink(app_t* application) {
    if (++application->blink_ticks >= BLINK_PERIOD) {
        application->blink_ticks = 0;
    }
}

static void update_programming_blink(app_t* application) {
    advance_blink(application);
    uint8_t level = application->blink_ticks >= BLINK_ON ? PROGRAMMING_WHITE : 0;
    application->color = (rgb_t){level, level, level};
}

static enum app_state select_state(const app_t* application) {
    switch (application->state) {
        case APP_EMPTY:
            if (application->programming.qualified) {
                return APP_PROGRAMMING;
            }
            if (!application->filament_present) {
                return APP_EMPTY;
            }
            break;
        case APP_PRESENT: break;
        case APP_UNLOAD:
            /* Keep the unload indication until the button and filament clear.
             * Tangle does not interrupt this indication while unloading. */
            return application->unload.qualified || application->filament_present ? APP_UNLOAD : APP_EMPTY;
        case APP_TANGLE:
            if (application->tangle.qualified) {
                return APP_TANGLE;
            }
            if (application->unload.qualified) {
                return APP_UNLOAD;
            }
            return application->filament_present ? APP_PRESENT : APP_EMPTY;
        case APP_PROGRAMMING:
            if (!application->filament_present) {
                return APP_PROGRAMMING;
            }
            break;
        default: return APP_EMPTY;
    }

    /* Resolve insertion/programming-exit directly to the qualified warning.
     * Tangle outranks unload; neither needs an intermediate green update. */
    if (application->tangle.qualified) {
        return APP_TANGLE;
    }
    if (application->unload.qualified) {
        return APP_UNLOAD;
    }
    return application->filament_present ? APP_PRESENT : APP_EMPTY;
}

static void update_application(app_t* application) {
    application->state = select_state(application);
    application->unload_tangle_output_enabled = application->state != APP_PROGRAMMING;
    /* FTU is a qualified event, not an LED-state indication. Once asserted,
     * overlapping qualified requests keep it high across any state handover.
     * Both requests must release before a later assertion becomes a new press.
     * Empty sensors cannot originate an event without entering a warning. */
    application->unload_tangle_asserted = application->unload_tangle_output_enabled
        && (application->unload.qualified || application->tangle.qualified)
        && (application->unload_tangle_asserted || application->state == APP_UNLOAD
            || application->state == APP_TANGLE);

    application->color = (rgb_t){0, 0, 0};
    switch (application->state) {
        case APP_EMPTY:
            application->blink_ticks = 0;
            application->color.red = 255;
            break;
        case APP_PRESENT:
            application->blink_ticks = 0;
            application->color.green = 255;
            break;
        case APP_UNLOAD:
        case APP_TANGLE:
            advance_blink(application);
            if (application->blink_ticks >= BLINK_ON) {
                if (application->state == APP_UNLOAD) {
                    application->color.blue = 255;
                } else {
                    application->color.red = 255;
                    application->color.green = 100;
                }
            }
            break;
        case APP_PROGRAMMING: update_programming_blink(application); break;
        default: break; /* select_state already maps invalid states to APP_EMPTY. */
    }
}

void app_tick(app_t* application, bool filament_present, uint8_t button_reading, bool recovering) {
    application->filament_present = filament_present;
    if (recovering) {
        application->unload_tangle_asserted = application->unload_tangle_output_enabled = false;
        application->tick_phase ^= 1;
        if (application->tick_phase == APP_SAMPLE_BUTTONS) {
            update_programming_blink(application);
        }
        return;
    }

    if (application->tick_phase == APP_SAMPLE_BUTTONS) {
        enum button button = classify_button(button_reading);
        update_button_filter(&application->unload, button == BUTTON_UNLOAD, UNLOAD_LIMIT);
        update_button_filter(&application->tangle, button == BUTTON_TANGLE, TANGLE_FILTER_LIMIT);
        update_programming_hold(&application->programming, button == BUTTON_BOTH);
        application->tick_phase = APP_UPDATE_OUTPUTS;
    } else if (application->tick_phase == APP_UPDATE_OUTPUTS) {
        application->tick_phase = APP_SAMPLE_BUTTONS;
        update_application(application);
    } else {
        application->tick_phase = APP_SAMPLE_BUTTONS;
    }
}
