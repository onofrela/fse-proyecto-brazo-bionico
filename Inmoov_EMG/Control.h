#ifndef INMOOV_CONTROL_H
#define INMOOV_CONTROL_H

#include <stdint.h>
#include "Config.h"

namespace Hand {

// 32 muestras a 1 kHz: promedio de los ultimos 32 ms.
class MovingAverage {
 public:
  void clear() {
    for (uint8_t i = 0; i < 32; ++i) samples_[i] = 0;
    sum_ = 0;
    index_ = 0;
    count_ = 0;
  }
  uint16_t add(uint16_t value) {
    sum_ -= samples_[index_];
    samples_[index_] = value;
    sum_ += value;
    index_ = (index_ + 1) & 31;
    if (count_ < 32) ++count_;
    return sum_ >> 5;
  }
  bool ready() const { return count_ == 32; }

 private:
  uint16_t samples_[32] = {};
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
      baselineQ8_ = int32_t(raw) * 256;
      initialized_ = true;
    }
    baselineQ8_ += (int32_t(raw) * 256 - baselineQ8_) / 256;
    const int32_t deviation = int32_t(raw) * 256 - baselineQ8_;
    const uint16_t magnitude = (deviation < 0 ? -deviation : deviation) / 256;
    envelope_ = window_.add(magnitude);
    if (stableSamples_ < Config::SIGNAL_SETTLE_SAMPLES) ++stableSamples_;
    return envelope_;
  }
  bool ready() const {
    return initialized_ && window_.ready() &&
           stableSamples_ >= Config::SIGNAL_SETTLE_SAMPLES;
  }
  bool clipped() const { return clipped_; }
  uint16_t baseline() const { return initialized_ ? baselineQ8_ / 256 : 0; }

 private:
  MovingAverage window_;
  int32_t baselineQ8_ = 0;
  uint16_t stableSamples_ = 0;
  uint16_t envelope_ = 0;
  bool initialized_ = false;
  bool clipped_ = false;
};

struct Thresholds {
  uint16_t open;
  uint16_t close;
  bool valid() const { return open > 0 && close > open && close < Config::ADC_HIGH_CLIP; }
};

// Se exige reposo al entrar al modo EMG o recuperarse de una falla.
class MuscleSwitch {
 public:
  void reset() { armed_ = false; closed_ = false; }
  bool update(uint16_t value, const Thresholds &t) {
    if (value <= t.open) {
      armed_ = true;
      closed_ = false;
    } else if (armed_ && value >= t.close) {
      closed_ = true;
    }
    return closed_;
  }

 private:
  bool armed_ = false;
  bool closed_ = false;
};

class DebouncedInput {
 public:
  void begin(bool value, uint32_t now) {
    stable_ = candidate_ = value;
    since_ = now;
  }
  bool update(bool value, uint32_t now) {
    if (value != candidate_) {
      candidate_ = value;
      since_ = now;
    }
    if (candidate_ != stable_ && uint32_t(now - since_) >= 10) {
      stable_ = candidate_;
      return true;
    }
    return false;
  }
  bool value() const { return stable_; }

 private:
  bool stable_ = false;
  bool candidate_ = false;
  uint32_t since_ = 0;
};

// Todos los dedos avanzan a la vez. Un recorrido dura 800 ms nominales.
class Motion {
 public:
  bool request(bool closed, uint32_t now) {
    const uint16_t target = closed ? 1000 : 0;
    if (target == target_) return false;
    target_ = target;
    last_ = now;
    return step(); // Primer cambio de pulso sin esperar otro periodo de 20 ms.
  }
  bool update(uint32_t now) {
    if (uint32_t(now - last_) < 20) return false;
    last_ = now;
    return step();
  }
  bool closed() const { return target_ == 1000; }
  uint16_t position() const { return position_; }
  bool isOpen() const { return position_ == 0 && target_ == 0; }

 private:
  bool step() {
    if (position_ == target_) return false;
    if (position_ < target_) {
      position_ += (target_ - position_ < 25) ? target_ - position_ : 25;
    } else {
      position_ -= (position_ - target_ < 25) ? position_ - target_ : 25;
    }
    return true;
  }
  uint16_t position_ = 0;
  uint16_t target_ = 0;
  uint32_t last_ = 0;
};

inline uint16_t pulse(uint16_t position, uint16_t open, uint16_t closed) {
  return int32_t(open) + (int32_t(closed) - open) * position / 1000;
}

class Calibration {
 public:
  enum Stage : uint8_t { IDLE, PREPARE_REST, REST, PREPARE_ACTIVE, ACTIVE,
                         SUCCESS, FAILED };
  void start(uint32_t now) {
    stage_ = PREPARE_REST;
    since_ = now;
    sum_ = 0;
    count_ = 0;
    restMax_ = 0;
  }
  void cancel() { stage_ = IDLE; }
  bool running() const { return stage_ >= PREPARE_REST && stage_ <= ACTIVE; }
  Stage stage() const { return stage_; }
  Thresholds thresholds() const { return result_; }

  // Llamar una vez por muestra nueva; no recontar la misma muestra.
  void update(uint32_t now, uint16_t value, bool healthy) {
    if (!running()) return;
    if (!healthy) { stage_ = FAILED; return; }
    const uint32_t elapsed = now - since_;
    if (stage_ == PREPARE_REST || stage_ == PREPARE_ACTIVE) {
      if (elapsed >= 2000) {
        stage_ = (stage_ == PREPARE_REST) ? REST : ACTIVE;
        since_ = now;
        sum_ = 0;
        count_ = 0;
      }
      return;
    }
    sum_ += value;
    ++count_;
    if (stage_ == REST && value > restMax_) restMax_ = value;
    if (elapsed < 3000) return;
    if (count_ < 2400) { stage_ = FAILED; return; }
    const uint16_t mean = sum_ / count_;
    if (stage_ == REST) {
      restMean_ = mean;
      stage_ = PREPARE_ACTIVE;
      since_ = now;
      return;
    }
    if (mean < restMean_ + Config::MIN_CALIBRATION_RANGE) { stage_ = FAILED; return; }
    const uint16_t range = mean - restMean_;
    result_.open = restMean_ + range / 4;
    result_.close = restMean_ + range / 2;
    stage_ = (result_.valid() && result_.open > restMax_ + Config::REST_NOISE_MARGIN)
                 ? SUCCESS : FAILED;
  }

 private:
  Stage stage_ = IDLE;
  uint32_t since_ = 0;
  uint32_t sum_ = 0;
  uint16_t count_ = 0;
  uint16_t restMean_ = 0;
  uint16_t restMax_ = 0;
  Thresholds result_ = {0, 0};
};

} // namespace Hand
#endif
