#ifndef ORBITER_BUTTONS_H
#define ORBITER_BUTTONS_H

/* Eight-bit ADC ranges; upper bounds are exclusive. Boot deliberately accepts
 * only idle/tangle, rather than reusing runtime button classification. */
enum {
    BUTTON_BOTH_MIN = 94,
    BUTTON_TANGLE_MIN = 115,
    BUTTON_TANGLE_END = 141,
    BUTTON_UNLOAD_MIN = 158,
    BUTTON_UNLOAD_END = 194,
    BUTTON_IDLE_MIN = 224
};

#endif
