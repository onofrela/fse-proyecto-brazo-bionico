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

} // namespace Hand
#endif
