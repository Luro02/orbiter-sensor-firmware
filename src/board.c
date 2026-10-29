#include <avr/io.h>
#include <avr/cpufunc.h>
#include <util/delay_basic.h>
#include "board.h"
#include "config.h"

#if !defined(__AVR_ATtiny202__)
    #error "This firmware is for ATtiny202 only"
#endif
#if RAMSTART != 0x3F80 || RAMEND != 0x3FFF
    #error "Unexpected ATtiny202 SRAM map"
#endif

/* Sole owner of PA2 direction. The only true argument comes from the tested
 * recovery gate. No whole-port direction assignment. */
static void set_unload_tangle_output_direction(bool enable) {
    if (enable) {
        PORTA.DIRSET = PIN2_bm;
    } else {
        PORTA.DIRCLR = PIN2_bm;
    }
}

void board_init(void) {
    set_unload_tangle_output_direction(false);
    PORTA.DIRCLR = PIN3_bm | PIN7_bm;
    PORTA.PIN2CTRL = 0;
    PORTA.PIN3CTRL = 0; /* External filament pull-up, active low. */
    PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTA.OUTCLR = PIN1_bm | PIN2_bm | PIN6_bm;
    PORTA.DIRSET = PIN1_bm | PIN6_bm;
    /* PA0/PIN0CTRL never written; existing UPDI fuses preserved. */
}

bool board_clock_init(void) {
    /* Run the calibrated 16/20-MHz oscillator without a prescaler or fuse writes.
     * The CPU clock remains constant during LED transmission. */
    uint8_t oscillator_selection = FUSE.OSCCFG & FUSE_FREQSEL_gm;
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSC20M_gc);
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);
    return oscillator_selection == (CPU_CLOCK_MHZ == 16 ? FREQSEL_16MHZ_gc : FREQSEL_20MHZ_gc)
        && (FUSE.OSCCFG & FUSE_OSCLOCK_bm) == 0 && CLKCTRL.MCLKCTRLA == 0 && CLKCTRL.MCLKCTRLB == 0;
}

void board_adc_init(void) {
    /* Sample PA7 continuously at an ADC clock of 500/625 kHz. */
    ADC0.CTRLB = 0;
    ADC0.CTRLC = ADC_REFSEL_VDDREF_gc | ADC_PRESC_DIV32_gc | ADC_SAMPCAP_bm;
    ADC0.CTRLD = 0;
    ADC0.SAMPCTRL = 14;
    ADC0.MUXPOS = ADC_MUXPOS_AIN7_gc;
    ADC0.CALIB = ADC_DUTYCYC_DUTY50_gc;
    ADC0.CTRLA = ADC_RESSEL_8BIT_gc | ADC_FREERUN_bm | ADC_ENABLE_bm;
    ADC0.INTFLAGS = ADC_RESRDY_bm;
    ADC0.COMMAND = ADC_STCONV_bm;
}

static void read_adc_result(uint8_t* value) {
    *value = ADC0.RESL;
    ADC0.INTFLAGS = ADC_RESRDY_bm;
}

bool board_adc_read(uint8_t* value) {
    uint16_t remaining = 1024;
    do {
        if ((ADC0.INTFLAGS & ADC_RESRDY_bm) != 0) {
            read_adc_result(value);
            return true;
        }
    } while (--remaining != 0);
    return false;
}

bool board_adc_poll(uint8_t* value) {
    if ((ADC0.INTFLAGS & ADC_RESRDY_bm) != 0) {
        read_adc_result(value);
        return true;
    }
    return false;
}

void board_start_fast_tick(void) {
    /* Existing calibrated OSC20M selection, no fuse writes. TCB periodic IRQ,
     * clock=CPU/2, TOP+1=8000 or 10000 -> 1 ms nominal. WO output stays OFF:
     * no pin override or conflicts with PA6 LEDs / PA0 UPDI. */
    TCB0.CCMP = CPU_CLOCK_MHZ * 500U - 1;
    TCB0.INTFLAGS = TCB_CAPT_bm;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
}

void board_settle(void) {
    /* PA7's 10k+1k / 100nF idle charging time constant is 1.1 ms.
     * Allow >4 time constants before deciding: 100,000 CPU cycles gives
     * 5 ms at 20 MHz / 6.25 ms at 16 MHz. A shorter wait could misclassify
     * an otherwise idle board during power-on as ambiguous/recovery.
     * Also supplies the initial WS2812 low reset interval. */
    _delay_loop_2(25000);
}

void board_apply_outputs(const app_t* application, const recovery_t* recovery) {
    /* Disable before touching the latch when recovery/programming denies PA2.
     * On entry to normal operation the latch is set before enabling the driver. */
    if (!recovery_unload_tangle_output_enabled(recovery, application->unload_tangle_output_enabled)) {
        set_unload_tangle_output_direction(false);
        return;
    }
    if (application->unload_tangle_asserted) {
        PORTA.OUTSET = PIN2_bm;
    } else {
        PORTA.OUTCLR = PIN2_bm;
    }
    set_unload_tangle_output_direction(true);
}

extern void ws2812_send(const uint8_t bytes[6]);

void board_show(rgb_t color) {
    /* Two cascaded pixels, GRB byte order, each byte MSB first. */
    uint8_t bytes[6] = {color.green, color.red, color.blue, color.green, color.red, color.blue};
    /* Caller masks IRQs (before SEI or in the foreground atomic block).
     * Last bit stays low. MMU frames nominally 7.8125 ms apart; the runtime
     * bound leaves ample WS2812 reset/latch time between successive frames. */
    ws2812_send(bytes);
}

void board_start_tick(void) {
    /* Never write RTC.CTRLA: DS80000956C RTC/PIT interaction errata. */
    while (RTC.STATUS != 0 || (RTC.PITSTATUS & RTC_CTRLBUSY_bm) != 0) {}
    RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
    RTC.PITINTFLAGS = RTC_PI_bm;
    RTC.PITINTCTRL = RTC_PI_bm;
    RTC.PITCTRLA = RTC_PERIOD_CYC256_gc | RTC_PITEN_bm;
    while ((RTC.PITSTATUS & RTC_CTRLBUSY_bm) != 0) {}
}
