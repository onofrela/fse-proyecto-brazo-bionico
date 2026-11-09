#ifndef INMOOV_SIGNAL_H
#define INMOOV_SIGNAL_H

#include <stdint.h>
#include "Config.h"

namespace Hand {

// 32 muestras a 1 kHz: promedio de los ultimos 32 ms.
class MovingAverage {
 public:
  void clear() {
    for (uint8_t i = 0; i < Config::FILTER_WINDOW_SAMPLES; ++i) samples_[i] = 0;
    sum_ = 0;
    index_ = 0;
    count_ = 0;
  }
  uint16_t add(uint16_t value) {
    sum_ -= samples_[index_];
    samples_[index_] = value;
    sum_ += value;
    index_ = (index_ + 1) % Config::FILTER_WINDOW_SAMPLES;
    if (count_ < Config::FILTER_WINDOW_SAMPLES) ++count_;
    return sum_ / Config::FILTER_WINDOW_SAMPLES;
  }
  bool ready() const { return count_ == Config::FILTER_WINDOW_SAMPLES; }

 private:
  uint16_t samples_[Config::FILTER_WINDOW_SAMPLES] = {};
  uint16_t sum_ = 0;
  uint8_t index_ = 0;
  uint8_t count_ = 0;
};

// OUTPUT del AD8232 NO es una envolvente EMG. Remueve el nivel de reposo
// con un IIR (tau ~256 ms), rectifica y promedia magnitud en 32 muestras.
// Es una medida experimental de actividad, no un clasificador muscular/ECG.
class Ad8232Signal {
 public:
  uint16_t add(uint16_t raw, bool leadsOff) {
    const bool clip = raw <= Config::ADC_LOW_CLIP || raw >= Config::ADC_HIGH_CLIP;
    if (leadsOff || clip) {
      if (initialized_) {
        window_.clear();
        initialized_ = false;
      }
      stableSamples_ = 0;
      envelope_ = 0;
      clipped_ = clip;
      return 0;
    }
    clipped_ = false;
    if (!initialized_) {
      baselineQ8_ = int32_t(raw) * Config::BASELINE_Q_SCALE;
      initialized_ = true;
    }
    baselineQ8_ += (int32_t(raw) * Config::BASELINE_Q_SCALE - baselineQ8_) / Config::BASELINE_TIME_SAMPLES;
    const int32_t deviation = int32_t(raw) * Config::BASELINE_Q_SCALE - baselineQ8_;
    const uint16_t magnitude = uint16_t((deviation < 0 ? -deviation : deviation) / Config::BASELINE_Q_SCALE);
    envelope_ = window_.add(magnitude);
    if (stableSamples_ < Config::SIGNAL_SETTLE_SAMPLES) ++stableSamples_;
    return envelope_;
  }
  bool ready() const {
    return initialized_ && window_.ready() &&
           stableSamples_ >= Config::SIGNAL_SETTLE_SAMPLES;
  }
  bool clipped() const { return clipped_; }
  uint16_t baseline() const { return initialized_ ? uint16_t(baselineQ8_ / Config::BASELINE_Q_SCALE) : 0; }

 private:
  MovingAverage window_;
  int32_t baselineQ8_ = 0;
  uint16_t stableSamples_ = 0;
  uint16_t envelope_ = 0;
  bool initialized_ = false;
  bool clipped_ = false;
};

} // namespace Hand
#endif
