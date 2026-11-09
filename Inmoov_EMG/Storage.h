#ifndef INMOOV_STORAGE_H
#define INMOOV_STORAGE_H
#include "CalibrationStore.h"
namespace Storage {
struct Settings {
  Hand::Thresholds thresholds;
  uint32_t bootCount;
  bool calibrated;
  bool bootVerified;
};
Settings load();
bool save(const Hand::Thresholds &thresholds);
Hand::SaveResult poll();
}
#endif
