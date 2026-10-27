#pragma once

// Generic versions with configurable min/max
#ifndef __cplusplus
namespace el
{
    template <typename T>
    bool constexpr in_range(T c, T min, T max) { return (c <= max) && (c >= min); }
    template <typename T>
    bool constexpr nearly_equal(T a, T b, T diff) { return ((a - b) <= diff) && ((a - b) >= -diff); }

    static inline int increment_roll(int x, int min, int max)
    {
        return ((x) >= (max)) ? (min) : ((x) + 1);
    }
    static inline int decrement_roll(int x, int min, int max)
    {
        return ((x) <= (min)) ? (max) : ((x)-1);
    }
};
#endif


#define FD_KILO  1e3
#define FD_MEGA  1e6
#define FD_GIGA  1e9

#define FD_MILLI 1e-3
#define FD_MICRO 1e-6
#define FD_NANO  1e-9
#define FD_PICO  1e-12
