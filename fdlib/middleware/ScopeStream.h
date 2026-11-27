#pragma once
#include <stdint.h>
#include <string.h>
template <typename T> class ScopeStream {
public:
  T*                 buf;
  T                  trigLvl;
  T                  hysteresisLvl;
  bool               hystTrig;
  uint8_t            decimation;
  uint8_t            decimationCnt;
  uint16_t           writeIdx; // Simple linear index
  uint8_t            channelsNum;
  uint32_t           sampleDepth;
  bool               isTrig;
  volatile bool      frozen;
  constexpr uint32_t BufferSize(uint8_t channels, uint32_t sampleDepth) const { return sampleDepth * channels * sizeof(T); }
  ScopeStream(T* storage, uint8_t channels, uint32_t sampleDepth) {
    buf               = storage;
    channelsNum       = channels;
    decimation        = 1;
    decimationCnt     = 1;
    this->sampleDepth = sampleDepth;
    trigLvl           = 0;
    writeIdx          = 0;
    isTrig            = false;
    frozen            = false;
  }
  void Write(const T* data) {
    if (!frozen) {
      // 1. Decimation
      if (--decimationCnt > 0) return;
      decimationCnt = decimation;
      // 2. Trigger Logic
      if (!isTrig) {
        if (data[0] > trigLvl + hysteresisLvl && !hystTrig) {
          hystTrig = true;
          isTrig   = true;
          writeIdx = 0; // Start filling from the beginning
        } else if (data[0] < trigLvl - hysteresisLvl && hystTrig) {
          hystTrig = false;
        }
      }
      // 3. Sequential Fill
      if (isTrig) {
        // Copy channels for this specific sample
        memcpy(buf + writeIdx * channelsNum, data, sizeof(T) * channelsNum);
        writeIdx++;
        // 4. Check if Full
        if (writeIdx >= sampleDepth) { frozen = true; }
      }
    }
  }
  /**
   * Since the buffer is now linear and aligned, we just
   * copy the whole thing or even read it directly.
   */
  bool Read(T* out_buffer, uint16_t buffer_size) {
    if (!frozen) return false;
    if (buffer_size < sampleDepth * channelsNum * sizeof(T)) return false;
    // Copy the captured window to your display buffer
    memcpy(out_buffer, buf, sampleDepth * channelsNum * sizeof(T));
    return true;
  }
  bool Read(uint8_t channel, T* out_buffer, uint16_t buffer_size) {
    if (!frozen) return false;
    if (buffer_size < sampleDepth * sizeof(T)) return false;
    // Copy the captured window to your display buffer
    for (uint16_t i = 0; i < sampleDepth; i++) { out_buffer[i] = buf[i * channelsNum + channel]; }
    return true;
  }
  bool IsFrozen() const { return frozen; }
  bool IsisTrig() const { return isTrig; }
  void Reset() {
    writeIdx      = 0;
    isTrig        = false;
    frozen        = false;
    decimationCnt = decimation;
  }
  const T* GetBuffer() const { return buf; }
  void     SetTriggerLevel(T level) { trigLvl = level; }
  void     UpdateDecimation(uint8_t divisions, uint32_t time_per_div_us, uint32_t sample_rate) {
    uint32_t samples_per_div      = (sampleDepth / divisions);
    uint32_t base_time_per_div_us = (uint64_t(samples_per_div) * 1000000) / sample_rate;
    decimation                    = (time_per_div_us > base_time_per_div_us) ? (uint8_t)((float(time_per_div_us) / base_time_per_div_us) + 0.5) : 1;
    decimationCnt                 = decimation;
  }
  void UpdateDecimation(uint8_t decimation) {
    this->decimation = decimation;
    decimationCnt    = decimation;
  }
};