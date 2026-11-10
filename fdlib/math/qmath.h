#pragma once

#pragma once
#include <stdint.h>


#define FORCE_INLINE __attribute__((always_inline))

namespace fd {
    
#if defined(__arm__) || defined(__thumb__) || defined(_M_ARM)
    #include "cmsis_compiler.h"
#endif
#if defined(__ARM_FEATURE_DSP) && (__ARM_FEATURE_DSP == 1)
    // Requires the appropriate CMSIS core instruction header.
    #define FD_HAS_ARM_SAT_ARITH 1
#else
    #define FD_HAS_ARM_SAT_ARITH 0
#endif


// Signed saturating addition
FORCE_INLINE static inline int32_t sat_add(int32_t a, int32_t b)
{
#if FD_HAS_ARM_SAT_ARITH
    return __QADD(a, b);
#else
    int64_t r = (int64_t)a + b;
    return r > INT32_MAX ? INT32_MAX : r < INT32_MIN ? INT32_MIN : (int32_t)r;
#endif
}

// Signed saturating subtraction
FORCE_INLINE static inline int32_t sat_sub(int32_t a, int32_t b)
{
#if FD_HAS_ARM_SAT_ARITH
    return __QSUB(a, b);
#else
    int64_t r = (int64_t)a - b;
    return r > INT32_MAX ? INT32_MAX : r < INT32_MIN ? INT32_MIN : (int32_t)r;
#endif
}

// Unsigned saturating addition
FORCE_INLINE static inline uint32_t sat_add(uint32_t a, uint32_t b)
{
    uint64_t r = (uint64_t)a + b;
    return r > UINT32_MAX ? UINT32_MAX : (uint32_t)r;
}

// Unsigned saturating subtraction
FORCE_INLINE static inline uint32_t sat_sub(uint32_t a, uint32_t b)
{
    return a < b ? 0 : a - b;
}


class pid_q31
{
    int32_t A[3];
    int32_t x[2];

    public:
    bool gainUpdate(float Kp, float Ki, float Kd)
    {
        float temp;

        temp = Kp + Ki + Kd;
        if (temp > 1.0f || temp < -1.0f) return false;
        A[0] = (int32_t)(temp * INT32_MAX);

        temp = -Kp - 2.0f * Kd;
        if (temp > 1.0f || temp < -1.0f) return false;
        A[1] = (int32_t)(temp * INT32_MAX);

        temp = Kd;
        if (temp > 1.0f || temp < -1.0f) return false;
        A[2] = (int32_t)(temp * INT32_MAX);

        return true;
    }
    void getGain(float &Kp, float &Ki, float &Kd)
    {
        constexpr float Q31_SCALE = 1.0f / INT32_MAX;
        
        float a0 = A[0] * Q31_SCALE;
        float a1 = A[1] * Q31_SCALE;
        float a2 = A[2] * Q31_SCALE;
        
        Kd = a2;
        Kp = -a1 - 2.0f * a2;
        Ki = a0 + a1 + a2;
    }
    pid_q31(){A[0]=0;A[1]=0;A[2]=0;x[0]=0;x[1]=0;};
    pid_q31(float Kp, float Ki, float Kd)
    {
        gainUpdate(Kp, Ki, Kd);
        x[0] = 0;
        x[1] = 0;
    };
    int32_t step(int32_t e)
    {
        int64_t acum = (int64_t)A[0]*e + (int64_t)A[1]*x[0] + (int64_t)A[2]*x[1];
        int32_t diff = acum>>31;
        x[1] = x[0];
        x[0] = e;
        return diff;
    }
    void reset()
    {
        x[0] = 0;
        x[1] = 0;
    }
};
}