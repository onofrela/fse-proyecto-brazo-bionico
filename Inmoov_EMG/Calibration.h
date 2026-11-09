#ifndef INMOOV_CALIBRATION_H
#define INMOOV_CALIBRATION_H
#include "Decision.h"
namespace Hand {
class Calibration {
 public:
  enum Stage : uint8_t { IDLE, PREPARE_REST, REST, PREPARE_ACTIVE, ACTIVE,
                         SUCCESS, FAILED };
  enum Reason : uint8_t { NONE, SIGNAL_LOST, TOO_FEW_SAMPLES, LOW_SEPARATION, RANGE_EXCEEDED };
  void start(uint32_t now) {
    stage_ = PREPARE_REST;
    since_ = now;
    reason_ = NONE;
    restMedian_ = rest95_ = active70_ = 0;
    clearHistogram();
  }
  void cancel() { stage_ = IDLE; }
  bool running() const { return stage_ >= PREPARE_REST && stage_ <= ACTIVE; }
  Stage stage() const { return stage_; }
  Thresholds thresholds() const { return result_; }
  Reason reason() const { return reason_; }
  uint16_t rest95() const { return rest95_; }
  uint16_t active70() const { return active70_; }

  // Llamar una vez por muestra nueva; no recontar la misma muestra.
  void update(uint32_t now, uint16_t value, bool healthy) {
    if (!running()) return;
    if (!healthy) { fail(SIGNAL_LOST); return; }
    const uint32_t elapsed = now - since_;
    if (stage_ == PREPARE_REST || stage_ == PREPARE_ACTIVE) {
      if (elapsed >= Config::CAL_PREPARE_MS) {
        stage_ = (stage_ == PREPARE_REST) ? REST : ACTIVE;
        since_ = now;
        clearHistogram();
        lastSample_ = now;
      }
      return;
    }
    if (uint32_t(now-lastSample_) >= Config::CAL_SAMPLE_MS) {
      lastSample_ = now;
      ++histogram_[value / 2 < 128 ? value / 2 : 127];
      ++count_;
    }
    const uint16_t duration = stage_ == REST ? Config::CAL_REST_MS : Config::CAL_ACTIVE_MS;
    if (elapsed < duration) return;
    if (count_ < (duration / Config::CAL_SAMPLE_MS) * 8 / 10) { fail(TOO_FEW_SAMPLES); return; }
    if (histogram_[127] * 100UL > count_) { fail(RANGE_EXCEEDED); return; }
    if (stage_ == REST) {
      restMedian_ = percentile(50);
      rest95_ = percentile(95);
      stage_ = PREPARE_ACTIVE;
      since_ = now;
      return;
    }
    active70_ = percentile(70);
    const uint16_t spread = 2 * (rest95_ - restMedian_);
    const uint16_t required = spread > Config::MIN_CALIBRATION_RANGE ? spread : Config::MIN_CALIBRATION_RANGE;
    if (active70_ < rest95_ + required + Config::REST_NOISE_MARGIN) { fail(LOW_SEPARATION); return; }
    const uint16_t range = active70_ - rest95_;
    // Umbrales cerca del comienzo de actividad, no de sus picos mas altos.
    result_.open = rest95_ + range / 10;
    if (result_.open < Config::OBSERVED_REST_CEILING) result_.open = Config::OBSERVED_REST_CEILING;
    if (result_.open < rest95_ + Config::REST_NOISE_MARGIN) result_.open = rest95_ + Config::REST_NOISE_MARGIN;
    result_.close = rest95_ + range / 4;
    if (result_.close < Config::OBSERVED_ACTIVE_FLOOR) result_.close = Config::OBSERVED_ACTIVE_FLOOR;
    if (result_.close < result_.open + 3) result_.close = result_.open + 3;
    if (!result_.valid() || result_.close >= active70_) { fail(LOW_SEPARATION); return; }
    stage_ = SUCCESS;
  }

 private:
  Stage stage_ = IDLE;
  uint32_t since_ = 0;
  uint32_t lastSample_ = 0;
  uint16_t count_ = 0;
  uint16_t histogram_[128] = {};
  uint16_t restMedian_ = 0, rest95_ = 0, active70_ = 0;
  Reason reason_ = NONE;
  Thresholds result_ = {0, 0};
  void clearHistogram() {
    count_ = 0;
    for (uint8_t i=0; i<128; ++i) histogram_[i] = 0;
  }
  uint16_t percentile(uint8_t p) const {
    const uint16_t target = (uint32_t(count_) * p + 99) / 100;
    uint16_t total = 0;
    for (uint8_t i=0; i<128; ++i) {
      total += histogram_[i];
      if (total >= target) return uint16_t(i)*2 + 1; // Borde superior conservador.
    }
    return 255;
  }
  void fail(Reason reason) { reason_ = reason; stage_ = FAILED; }
};

}
#endif
