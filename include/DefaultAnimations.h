#ifndef ANIMATION
#include <AnimationDriver.h>
#endif

// Macro used to generate struct for solid colors
#define SOLID_COLOR(r, g, b) ((struct AnimationDriver::animation){{                     \
                                                                      {{r, g, b}, 0},   \
                                                                      {{r, g, b}, 100}, \
                                                                  },                    \
                                                                  2,                    \
                                                                  100})

// Macro used to generate struct for breathing animation
#define BREATHE_COLOR(r, g, b, time) ((struct AnimationDriver::animation){{                                          \
                                                                              {{r, g, b}, 0},                        \
                                                                              {{r / 2, g / 2, b / 2}, time / 4},     \
                                                                              {{0, 0, 0}, time / 2},                 \
                                                                              {{r / 2, g / 2, b / 2}, time * 3 / 4}, \
                                                                              {{r, g, b}, time},                     \
                                                                          },                                         \
                                                                          5,                                         \
                                                                          time})

struct AnimationDriver::animation rainbow_anim = {
    {
        // Red
        {
            {255, 0, 0}, // Color
            0            // Time
        },
        // Orange
        {
            {255, 127, 0}, // Color
            500            // Time
        },
        // Yellow
        {
            {255, 255, 0}, // Color
            1000           // Time
        },
        // Lime
        {
            {127, 255, 0}, // Color
            1500           // Time
        },
        // Green
        {
            {0, 255, 0}, // Color
            2000         // Time
        },
        // Seafoam
        {
            {0, 255, 127}, // Color
            2500           // Time
        },
        // Cyan
        {
            {0, 255, 255}, // Color
            3000           // Time
        },
        // Light Blue?
        {
            {0, 127, 255}, // Color
            3500           // Time
        },
        // Blue
        {
            {0, 0, 255}, // Color
            4000         // Time
        },
        // Lavender
        {
            {127, 0, 255}, // Color
            4500           // Time
        },
        // Pink
        {
            {255, 0, 255}, // Color
            5000           // Time
        },
        // Hot Pink
        {
            {255, 0, 127}, // Color
            5500           // Time
        },
        // Red
        {
            {255, 0, 0}, // Color
            6000         // Time
        },
    },
    13,  // Frame count (must match above array length)
    6000 // Total length (must match last time member of array)
};

struct AnimationDriver::animation breathBlue_anim = {
    // Animation frame array
    {
        // Blue full bright
        {
            {0, 0, 255}, // Color
            0            // Time
        },
        // Blue half bright
        {
            {0, 0, 127}, // Color
            500          // Time
        },
        // Off
        {
            {0, 0, 0}, // Color
            1000       // Time
        },
        // Blue half bright
        {
            {0, 0, 127}, // Color
            1500         // Time
        },
        // Blue Full bright
        {
            {0, 0, 255}, // Color
            2000         // Time
        },
    },
    5,   // Frame count (must match above array length)
    2000 // Total length (must match last time member of array)
};
