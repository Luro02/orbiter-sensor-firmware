#include <avr/interrupt.h>
#include <util/atomic.h>
#include "board.h"
#include "recovery.h"
#include "config.h"
#include "filament.h"
#include "leds.h"

static app_t application;
static recovery_t recovery;
static filament_t filament;
static startup_t startup;
static volatile uint8_t slow_ticks;
static volatile bool filament_signal_enabled;

ISR(RTC_PIT_vect) {
    RTC.PITINTFLAGS = RTC_PI_bm;
    ++slow_ticks;
}

ISR(TCB0_INT_vect) {
    TCB0.INTFLAGS = TCB_CAPT_bm;
    bool present = filament_tick(&filament, !board_filament_pin_high());
    board_write_filament_signal(filament_signal_enabled && present);
}

/* Keep boot's ADC sample array off the runtime/interrupt stack. */
static void boot(void) __attribute__((noinline));
static void boot(void) {
    uint8_t samples[BOOT_ADC_SAMPLES] = {0};
    uint8_t discarded_reading;
    board_init();
    bool startup_valid = board_clock_init();
    board_adc_init();
    board_settle();
    for (uint8_t sample_index = 0; sample_index < 2; ++sample_index) {
        if (!board_adc_read(&discarded_reading)) {
            startup_valid = false;
        }
    }
    for (uint8_t sample_index = 0; sample_index < BOOT_ADC_SAMPLES; ++sample_index) {
        if (!board_adc_read(&samples[sample_index])) {
            startup_valid = false;
        }
    }
    recovery_decide(&recovery, startup_valid, samples);
    application.unload_tangle_output_enabled = true; /* Other fields were cleared by the CRT. */
    filament_signal_enabled = !recovery.boot_recovery;
    board_apply_outputs(&application, &recovery);
    board_show(application.color);
    board_start_tick();
    board_start_fast_tick();
}

int main(void) {
    uint8_t consumed_ticks = 0;
    cli();
    boot();
    sei();
    for (;;) {
        /* ISR count is an atomic byte; subtraction is modulo 256. Foreground
         * must finish a slow slot in <1 PIT period (checked in the audit).
         * No slow work, ADC polling or LED transmission runs inside the ISR. */
        if (slow_ticks == consumed_ticks) {
            continue;
        }
        ++consumed_ticks;
        uint8_t button_reading = 255;
        if (!recovery.boot_recovery && application.tick_phase == APP_SAMPLE_BUTTONS
            && !board_adc_poll(&button_reading)) {
            recovery.boot_recovery = true;
            filament_signal_enabled = false;
            board_write_filament_signal(false);
        }
        app_tick(&application, filament.present, button_reading, !recovery.boot_complete || recovery.boot_recovery);
        board_apply_outputs(&application, &recovery);

        rgb_t display = application.color;
        startup_color(&startup, &display, recovery.boot_recovery || app_cancels_startup(&application));
        /* Every slow slot transmits (including startup frames), rather than
         * maintaining a second color cache. 7.8125-ms gaps provide LED latch
         * time. Only the wire frame masks IRQs, not the foreground logic. */
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { board_show(display); }
    }
}
