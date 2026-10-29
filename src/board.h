#ifndef ORBITER_BOARD_H
#define ORBITER_BOARD_H

#include "app.h"
#include "recovery.h"
#include "config.h"
#include <avr/io.h>
/* Inline tiny I/O operations keep the fast ISR free of call-clobbered register
 * saves. SBI/CBI affect FS only; slow FTU writes use independent SET/CLR aliases. */
static inline bool board_filament_pin_high(void) { return (VPORTA.IN & PIN3_bm) != 0; }
static inline void board_write_filament_signal(bool present) {
    if (present) {
        VPORTA.OUT |= PIN1_bm;
    } else {
        VPORTA.OUT &= (uint8_t) ~PIN1_bm;
    }
}

void board_init(void);
bool board_clock_init(void);
void board_adc_init(void);
bool board_adc_read(uint8_t* value);
void board_settle(void);
void board_apply_outputs(const app_t* application, const recovery_t* recovery);
void board_show(rgb_t color);
void board_start_tick(void);
bool board_adc_poll(uint8_t* value);
void board_start_fast_tick(void);

#endif
