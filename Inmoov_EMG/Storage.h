#ifndef INMOOV_STORAGE_H
#define INMOOV_STORAGE_H
#include "Config.h"
namespace Storage {
struct Settings {
  uint32_t bootCount;
  bool bootVerified;
};
Settings load();
}
#endif
