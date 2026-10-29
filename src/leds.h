#ifndef ORBITER_LEDS_H
#define ORBITER_LEDS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgb_t;
typedef struct {
    uint8_t frame;
    uint8_t frame_ticks;
} startup_t;

/* Called after application work, once per 128-Hz RTC slot.
 * ORIGINAL: 18 frames at 19/128 s; RAINBOW: 48 hue steps at 7/128 s.
 * NONE leaves the display alone. Recovery/unload/tangle permanently cancel
 * the show. Filament sensing never waits for the animation. */
void startup_color(startup_t* startup, rgb_t* color, bool cancel_animation);

#endif
