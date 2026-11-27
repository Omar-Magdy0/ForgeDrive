#pragma once
#include <math.h>
#include <stdint.h>
#define FORCE_INLINE __attribute__((always_inline))
#if defined(__arm__) || defined(__thumb__) || defined(_M_ARM)
extern "C" {
#include "cmsis_gcc.h"
}
#endif
namespace fd {
#if defined(__ARM_FEATURE_DSP) && (__ARM_FEATURE_DSP == 1)
// Requires the appropriate CMSIS core instruction header.
#define FD_HAS_ARM_SAT_ARITH 1
#else
#define FD_HAS_ARM_SAT_ARITH 0
#endif
//  Signed saturating addition
FORCE_INLINE static inline int32_t sat_add(int32_t a, int32_t b) {
#if FD_HAS_ARM_SAT_ARITH
  return __QADD(a, b);
#else
  int64_t r = (int64_t)a + b;
  return r > INT32_MAX ? INT32_MAX : r < INT32_MIN ? INT32_MIN : (int32_t)r;
#endif
}
// Signed saturating subtraction
FORCE_INLINE static inline int32_t sat_sub(int32_t a, int32_t b) {
#if FD_HAS_ARM_SAT_ARITH
  return __QSUB(a, b);
#else
  int64_t r = (int64_t)a - b;
  return r > INT32_MAX ? INT32_MAX : r < INT32_MIN ? INT32_MIN : (int32_t)r;
#endif
}
// Unsigned saturating addition
FORCE_INLINE static inline uint32_t sat_add(uint32_t a, uint32_t b) {
  uint64_t r = (uint64_t)a + b;
  return r > UINT32_MAX ? UINT32_MAX : (uint32_t)r;
}
// Unsigned saturating subtraction
FORCE_INLINE static inline uint32_t sat_sub(uint32_t a, uint32_t b) { return a < b ? 0 : a - b; }
// constants
inline constexpr int32_t                                         MaxaMinbA0 = static_cast<int32_t>((127.0 / 128.0) * INT32_MAX);
inline constexpr int32_t                                         MaxaMinbA1 = static_cast<int32_t>((27.0 / 32.0) * INT32_MAX);
inline constexpr int32_t                                         MaxaMinbB0 = static_cast<int32_t>((3.0 / 16.0) * INT32_MAX);
inline constexpr int32_t                                         MaxaMinbB1 = static_cast<int32_t>((71.0 / 128.0) * INT32_MAX);
inline constexpr float                                           Pi         = 3.14159265358979323846f;
inline constexpr double                                          Kilo       = 1e3;
inline constexpr double                                          Mega       = 1e6;
inline constexpr double                                          Giga       = 1e9;
inline constexpr double                                          Milli      = 1e-3;
inline constexpr double                                          Micro      = 1e-6;
inline constexpr double                                          Nano       = 1e-9;
inline constexpr double                                          Pico       = 1e-12;
template <typename T, typename U> constexpr auto                 Min(T a, U b) { return a < b ? a : b; }
template <typename T, typename U> constexpr auto                 Max(T a, U b) { return a > b ? a : b; }
template <typename T, typename Min, typename Max> constexpr auto Clamp(T value, Min min, Max max) { return value < min ? min : (value > max ? max : value); }
template <typename T> constexpr T                                Abs(T value) { return value < T{0} ? -value : value; }
class pid_q31 {
  int32_t A[3];
  int32_t x[2];
public:
  bool gainUpdate(float Kp, float Ki, float Kd) {
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
  void getGain(float& Kp, float& Ki, float& Kd) {
    constexpr float Q31_SCALE = 1.0f / INT32_MAX;
    float           a0        = A[0] * Q31_SCALE;
    float           a1        = A[1] * Q31_SCALE;
    float           a2        = A[2] * Q31_SCALE;
    Kd                        = a2;
    Kp                        = -a1 - 2.0f * a2;
    Ki                        = a0 + a1 + a2;
  }
  pid_q31() {
    A[0] = 0;
    A[1] = 0;
    A[2] = 0;
    x[0] = 0;
    x[1] = 0;
  };
  pid_q31(float Kp, float Ki, float Kd) {
    gainUpdate(Kp, Ki, Kd);
    x[0] = 0;
    x[1] = 0;
  };
  int32_t step(int32_t e) {
    int64_t acum = (int64_t)A[0] * e + (int64_t)A[1] * x[0] + (int64_t)A[2] * x[1];
    int32_t diff = acum >> 31;
    x[1]         = x[0];
    x[0]         = e;
    return diff;
  }
  void reset() {
    x[0] = 0;
    x[1] = 0;
  }
};
static inline uint32_t maxa_minb_uq31(int32_t i1, int32_t i2) {
  uint32_t a   = i1 < 0 ? 0u - (uint32_t)i1 : (uint32_t)i1;
  uint32_t b   = i2 < 0 ? 0u - (uint32_t)i2 : (uint32_t)i2;
  uint32_t max = a > b ? a : b;
  uint32_t min = a > b ? b : a;
  int32_t  z0  = (int32_t)(((int64_t)MaxaMinbA0 * max + (int64_t)MaxaMinbB0 * min) >> 31);
  int32_t  z1  = (int32_t)(((int64_t)MaxaMinbA1 * max + (int64_t)MaxaMinbB1 * min) >> 31);
  return (uint32_t)(z0 > z1 ? z0 : z1);
}
} // namespace fd
