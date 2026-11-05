#ifndef INMOOV_MOTION_H
#define INMOOV_MOTION_H

#include <stdint.h>
#include "Config.h"

namespace Hand {

// Todos los dedos avanzan a la vez. Un recorrido dura 800 ms nominales.
class Motion {
 public:
  bool request(bool closed, uint32_t now) {
    const uint16_t target = closed ? Config::POSITION_FULL_SCALE : 0;
    if (target == target_) return false;
    target_ = target;
    last_ = now;
    return step(); // Primer cambio de pulso sin esperar otro periodo de 20 ms.
  }
  bool update(uint32_t now) {
    if (uint32_t(now - last_) < Config::MOTION_PERIOD_MS) return false;
    last_ = now;
    return step();
  }
  bool closed() const { return target_ == Config::POSITION_FULL_SCALE; }
  uint16_t position() const { return position_; }
  bool isOpen() const { return position_ == 0 && target_ == 0; }

 private:
  bool step() {
    if (position_ == target_) return false;
    if (position_ < target_) {
      position_ += (target_ - position_ < Config::MOTION_STEP) ? target_ - position_ : Config::MOTION_STEP;
    } else {
      position_ -= (position_ - target_ < Config::MOTION_STEP) ? position_ - target_ : Config::MOTION_STEP;
    }
    return true;
  }
  uint16_t position_ = 0;
  uint16_t target_ = 0;
  uint32_t last_ = 0;
};

inline uint16_t pulse(uint16_t position, uint16_t open, uint16_t closed) {
  return int32_t(open) + (int32_t(closed) - open) * position / Config::POSITION_FULL_SCALE;
}

} // namespace Hand
#endif
