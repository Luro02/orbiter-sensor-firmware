/* AVRxt only. ABI: r25:r24 -> six GRB bytes. Clobbers call-saved-by-caller
 * r18,r20,Z only. Caller disables interrupts throughout the 48-bit frame.
 * SBI/CBI take ONE cycle on AVRxt (not classic AVR's two).
 * Each bit: 23 cycles; high=8 (zero) or 14 (one). This single waveform meets
 * the referenced WS2812B windows at BOTH 16 MHz and 20 MHz. See timing audit.
 * No direction writes, whole-port writes, clock changes, or PA0/PA2 accesses.
 */
#include <avr/io.h>

.macro wait_cycles count
    .rept (\count / 2)
        rjmp .+0
    .endr
    .if (\count % 2)
        nop
    .endif
.endm

.macro pulse
    sbi _SFR_IO_ADDR(VPORTA_OUT), 6
    wait_cycles 6
    sbrs r18, 7
    cbi _SFR_IO_ADDR(VPORTA_OUT), 6
    wait_cycles 5
    cbi _SFR_IO_ADDR(VPORTA_OUT), 6
.endm

.section .text.ws2812_send,"ax",@progbits
.global ws2812_send
.type ws2812_send, @function
ws2812_send:
    movw r30, r24
    ldi r20, 6
    ld r18, Z+
1:
    .rept 7
        pulse
        lsl r18
        wait_cycles 7
    .endr
    pulse
    wait_cycles 2
    dec r20
    breq 2f
    ld r18, Z+
    rjmp 1b
2:
    ret
.size ws2812_send, .-ws2812_send
