#ifndef ORBITER_CONFIG_H
#define ORBITER_CONFIG_H

#ifndef CPU_CLOCK_MHZ
    #define CPU_CLOCK_MHZ 20
#endif
#if CPU_CLOCK_MHZ != 16 && CPU_CLOCK_MHZ != 20
    #error "CPU_CLOCK_MHZ must match existing oscillator fuse: 16 or 20"
#endif
#ifndef INSERT_DEBOUNCE_MS
    #define INSERT_DEBOUNCE_MS 5
#endif
#ifndef REMOVE_DEBOUNCE_MS
    #define REMOVE_DEBOUNCE_MS 5
#endif
#define VALID_DEBOUNCE(milliseconds) \
    ((milliseconds) == 0 || (milliseconds) == 5 || (milliseconds) == 10 || (milliseconds) == 20 \
     || (milliseconds) == 100)
#if !VALID_DEBOUNCE(INSERT_DEBOUNCE_MS) || !VALID_DEBOUNCE(REMOVE_DEBOUNCE_MS)
    #error "Debounce must be 0, 5, 10, 20 or 100 ms"
#endif
#define STARTUP_NONE 0
#define STARTUP_ORIGINAL 1
#define STARTUP_RAINBOW 2
#ifndef STARTUP_ANIMATION
    /* Preserve existing -DSTARTUP_SHOW=0/1 build commands. */
    #ifdef STARTUP_SHOW
        #define STARTUP_ANIMATION STARTUP_SHOW
    #else
        #define STARTUP_ANIMATION STARTUP_RAINBOW
    #endif
#endif
#if STARTUP_ANIMATION != STARTUP_NONE && STARTUP_ANIMATION != STARTUP_ORIGINAL && STARTUP_ANIMATION != STARTUP_RAINBOW
    #error "Startup animation must be STARTUP_NONE, STARTUP_ORIGINAL or STARTUP_RAINBOW"
#endif
#ifndef STARTUP_SHOW
    #define STARTUP_SHOW (STARTUP_ANIMATION != STARTUP_NONE)
#elif STARTUP_SHOW != (STARTUP_ANIMATION != STARTUP_NONE)
    #error "Conflicting STARTUP_SHOW and STARTUP_ANIMATION settings"
#endif
/* 10 preserves manufacturer unload. 2 is an optional shorter integrating
 * filter: three samples, >=31.25 ms from first sample to qualification.
 * Limit 1 is invalid: clearing qualification at count one would prevent assertion. */
#ifndef UNLOAD_LIMIT
    #define UNLOAD_LIMIT 10
#endif
#if UNLOAD_LIMIT != 10 && UNLOAD_LIMIT != 2
    #error "Unload limit must be manufacturer 10 or experimental short 2"
#endif

#endif
