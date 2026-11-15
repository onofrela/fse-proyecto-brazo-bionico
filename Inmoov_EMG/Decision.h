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

// Algoritmo anterior, conservado para las pruebas historicas; no usado por FW=8.
// Se exige reposo al entrar al modo EMG o recuperarse de una falla.
class MuscleSwitch {
 public:
  void reset() {
    armed_ = closed_ = initialized_ = lowTracking_ = openingHold_ = false;
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
      if (openingHold_ && uint32_t(now-openSince_) >= Config::MIN_OPEN_MS) openingHold_ = false;
      if (armed_ && !openingHold_ && activity() >= Config::ACTIVITY_CLOSE_LEVEL) {
        closed_ = true; closedSince_ = now; lowTracking_ = false;
      }
    } else if (lowTracking_ && uint32_t(now-lowSince_) >= Config::RELAX_CONFIRM_MS &&
               uint32_t(now-closedSince_) >= Config::MIN_CLOSED_MS) {
      closed_ = false;
      openingHold_ = true;
      openSince_ = now;
    }
    return closed_;
  }

 private:
  bool armed_ = false;
  bool closed_ = false;
  bool initialized_ = false, lowTracking_ = false, openingHold_ = false;
  int32_t activityQ8_ = 0;
  uint32_t last_ = 0, lowSince_ = 0, closedSince_ = 0, openSince_ = 0;
};

} // namespace Hand
#endif
