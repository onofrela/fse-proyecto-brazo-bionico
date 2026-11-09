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
  void reset() {
    armed_ = closed_ = initialized_ = lowTracking_ = false;
    activityQ8_ = 0;
  }
  uint16_t activity() const { return uint16_t(activityQ8_ / 256); }
  bool update(uint16_t value, const Thresholds &t, uint32_t now) {
    if (!t.valid()) { reset(); return false; }
    if (!initialized_) { initialized_ = true; last_ = now; return false; }
    const uint32_t dt = now - last_;
    if (!dt) return closed_;
    last_ = now;
    if (dt > Config::MAX_CONTROL_GAP_MS) {
      reset(); initialized_ = true; last_ = now; return false;
    }
    // Peso normalizado: <=abrir ->0; >=cerrar ->1000; entre ambos, lineal.
    const int32_t target = value <= t.open ? 0 : value >= t.close ? 256000L :
        int32_t(value - t.open) * 256000L / (t.close - t.open);
    const uint16_t tau = target > activityQ8_ ? Config::ACTIVITY_ATTACK_MS : Config::ACTIVITY_DECAY_MS;
    activityQ8_ += (target - activityQ8_) * int32_t(dt) / (int32_t(tau) + int32_t(dt));
    const bool low = value <= t.open && activity() <= Config::ACTIVITY_OPEN_LEVEL;
    if (low && !lowTracking_) { lowTracking_ = true; lowSince_ = now; }
    if (!low) lowTracking_ = false;
    if (!closed_) {
      if (lowTracking_ && uint32_t(now-lowSince_) >= Config::REST_ARM_MS) armed_ = true;
      if (armed_ && activity() >= Config::ACTIVITY_CLOSE_LEVEL) {
        closed_ = true; closedSince_ = now; lowTracking_ = false;
      }
    } else if (lowTracking_ && uint32_t(now-lowSince_) >= Config::RELAX_CONFIRM_MS &&
               uint32_t(now-closedSince_) >= Config::MIN_CLOSED_MS) {
      closed_ = false;
    }
    return closed_;
  }

 private:
  bool armed_ = false;
  bool closed_ = false;
  bool initialized_ = false, lowTracking_ = false;
  int32_t activityQ8_ = 0;
  uint32_t last_ = 0, lowSince_ = 0, closedSince_ = 0;
};

} // namespace Hand
#endif
