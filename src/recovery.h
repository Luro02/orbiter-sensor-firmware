#ifndef ORBITER_RECOVERY_H
#define ORBITER_RECOVERY_H

#include <stdbool.h>
#include <stdint.h>

#define BOOT_ADC_SAMPLES 4
typedef struct {
    bool boot_complete;
    bool boot_recovery;
} recovery_t;

/* One-shot decision. Ambiguous/unsettled ADC readings latch recovery.
 * There is deliberately no operation to clear a latched recovery request. */
void recovery_decide(recovery_t* recovery, bool startup_valid, const uint8_t samples[BOOT_ADC_SAMPLES]);

/* Deny PA2 output before the boot decision and throughout latched recovery. */
static inline bool recovery_unload_tangle_output_enabled(const recovery_t* recovery, bool requested) {
    return recovery->boot_complete && !recovery->boot_recovery && requested;
}

#endif
