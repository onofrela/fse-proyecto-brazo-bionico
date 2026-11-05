#ifndef INMOOV_INPUTS_H
#define INMOOV_INPUTS_H

#include <stdint.h>
#include "Config.h"

namespace Hand {

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
    if (candidate_ != stable_ && uint32_t(now - since_) >= Config::DEBOUNCE_MS) {
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

} // namespace Hand
#endif
