#ifndef INMOOV_CONTROL_H
#define INMOOV_CONTROL_H
#include <stdint.h>
#include "Config.h"
#include "Signal.h"
#include "Decision.h"
#include "Inputs.h"

namespace Hand {

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
