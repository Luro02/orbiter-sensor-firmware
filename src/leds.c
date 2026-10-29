#include "leds.h"
#include "config.h"

#if STARTUP_ANIMATION == STARTUP_RAINBOW
    #define STARTUP_FRAMES 48
    #define STARTUP_FRAME_TICKS 7
#else
    #define STARTUP_FRAMES 18
    #define STARTUP_FRAME_TICKS 19
#endif

void startup_color(startup_t* startup, rgb_t* color, bool cancel_animation) {
#if STARTUP_ANIMATION != STARTUP_NONE
    if (cancel_animation) {
        startup->frame = STARTUP_FRAMES;
    }
    if (startup->frame >= STARTUP_FRAMES) {
        return;
    }
    #if STARTUP_ANIMATION == STARTUP_ORIGINAL
    *color = (rgb_t){0, 0, 0};
    if ((startup->frame & 1) == 0) {
        /* Original nine lit colors, interleaved with black. Bits are RGB. */
        static const uint8_t colors[9] = {4, 2, 1, 3, 6, 5, 7, 7, 7};
        uint8_t color_mask = colors[startup->frame >> 1];
        if (color_mask & 4) {
            color->red = 255;
        }
        if (color_mask & 2) {
            color->green = 255;
        }
        if (color_mask & 1) {
            color->blue = 255;
        }
    }
    #else
    /* Six full-saturation hue sectors, eight steps each. All arithmetic is
     * 8-bit; no division, floats, extra pixel buffer or runtime color table.
     * 48 * 7 / 128 = 2.625 s. The ISR continues to publish FS throughout. */
    uint8_t rise = (uint8_t) (startup->frame << 5);
    uint8_t fall = (uint8_t) (255 - rise);
    switch (startup->frame >> 3) {
        case 0: *color = (rgb_t){255, rise, 0}; break;
        case 1: *color = (rgb_t){fall, 255, 0}; break;
        case 2: *color = (rgb_t){0, 255, rise}; break;
        case 3: *color = (rgb_t){0, fall, 255}; break;
        case 4: *color = (rgb_t){rise, 0, 255}; break;
        default: *color = (rgb_t){255, 0, fall}; break;
    }
    #endif
    if (++startup->frame_ticks == STARTUP_FRAME_TICKS) {
        startup->frame_ticks = 0;
        ++startup->frame;
    }
#else
    (void) startup;
    (void) color;
    (void) cancel_animation;
#endif
}
