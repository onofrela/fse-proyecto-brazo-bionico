#ifndef INMOOV_DECISION_H
#define INMOOV_DECISION_H

#include <stdint.h>
#include "Config.h"

namespace Hand {

struct Thresholds {
  uint16_t open;
  uint16_t close;
  bool valid() const { return open > 0 && close > open && close < Config::ADC_HIGH_CLIP; }
};

// Control activo FW=8: clasificar el promedio, nunca una lectura aislada.
// No hay buffer dinamico: suma de hasta 200 muestras completas a 1 kHz.
class ThreeLevelControl {
 public:
  void reset() {
    sum_ = 0; count_ = position_ = meanX10_ = 0;
    initialized_ = ready_ = false;
  }
  uint16_t add(uint16_t value, uint32_t now) {
    if (initialized_ && uint32_t(now-last_) > Config::MAX_CONTROL_GAP_MS) reset();
    last_ = now; initialized_ = true;
    sum_ += value;
    if (++count_ == Config::LEVEL_AVERAGE_SAMPLES) {
      // Comparar sumas evita que redondear 4.999 o 9.999 cambie la banda.
      position_ = sum_ < uint32_t(count_)*Config::LEVEL_MID_START ? 0 :
                  sum_ < uint32_t(count_)*Config::LEVEL_HIGH_START ?
                  Config::POSITION_FULL_SCALE/2 : Config::POSITION_FULL_SCALE;
      meanX10_ = uint16_t(sum_*10/count_);
      sum_ = 0; count_ = 0; ready_ = true;
    }
    return position_;
  }
  bool ready() const { return ready_; }
  uint16_t meanX10() const { return meanX10_; }
  uint16_t position() const { return position_; }
 private:
  uint32_t sum_ = 0, last_ = 0;
  uint16_t count_ = 0, position_ = 0, meanX10_ = 0;
  bool initialized_ = false, ready_ = false;
};

} // namespace Hand
#endif
