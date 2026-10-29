#include "recovery.h"
#include "buttons.h"

enum {
    BOOT_MAX_READING_SPREAD = 4
};

void recovery_decide(recovery_t* recovery, bool startup_valid, const uint8_t samples[BOOT_ADC_SAMPLES]) {
    uint8_t minimum_reading = 255;
    uint8_t maximum_reading = 0;
    if (recovery->boot_complete) {
        return;
    }

    for (uint8_t sample_index = 0; sample_index < BOOT_ADC_SAMPLES; ++sample_index) {
        uint8_t value = samples[sample_index];
        if (value < minimum_reading) {
            minimum_reading = value;
        }
        if (value > maximum_reading) {
            maximum_reading = value;
        }
    }

    /* Boot accepts only settled idle or tangle-only readings. Unload, both
     * buttons, gaps, >4-count spread or failed conversions prefer recovery.
     * This deliberately differs from ordinary runtime button classification. */
    if (!(minimum_reading >= BUTTON_IDLE_MIN
          || (minimum_reading >= BUTTON_TANGLE_MIN && maximum_reading < BUTTON_TANGLE_END))
        || (uint8_t) (maximum_reading - minimum_reading) > BOOT_MAX_READING_SPREAD) {
        startup_valid = false;
    }
    if (!startup_valid) {
        recovery->boot_recovery = true;
    }
    recovery->boot_complete = true;
}
