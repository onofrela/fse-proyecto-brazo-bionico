#ifndef INMOOV_CALIBRATION_H
#define INMOOV_CALIBRATION_H

#include <stdint.h>
#include "Config.h"
#include "Decision.h"

namespace Hand {

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
    if (!healthy || value >= Config::ADC_HIGH_CLIP) { stage_ = FAILED; return; }
    const uint32_t elapsed = now - since_;
    if (stage_ == PREPARE_REST || stage_ == PREPARE_ACTIVE) {
      if (elapsed >= Config::CAL_PREPARE_MS) {
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
    if (elapsed < Config::CAL_MEASURE_MS) return;
    if (count_ < Config::CAL_MIN_SAMPLES) { stage_ = FAILED; return; }
    const uint16_t mean = uint16_t(sum_ / count_);
    if (stage_ == REST) {
      restMean_ = mean;
      stage_ = PREPARE_ACTIVE;
      since_ = now;
      return;
    }
    if (mean < restMean_ + Config::MIN_CALIBRATION_RANGE) { stage_ = FAILED; return; }
    const uint16_t range = mean - restMean_;
    result_.open = restMean_ + range / Config::OPEN_THRESHOLD_DIVISOR;
    result_.close = restMean_ + range / Config::CLOSE_THRESHOLD_DIVISOR;
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
