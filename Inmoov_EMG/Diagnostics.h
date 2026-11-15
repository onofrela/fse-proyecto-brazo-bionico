#ifndef INMOOV_DIAGNOSTICS_H
#define INMOOV_DIAGNOSTICS_H
#include <stdint.h>
#include "Decision.h"
namespace Diagnostics {
struct Sample {
  uint16_t raw = 0;
  uint16_t baseline = 0;
  uint16_t envelope = 0;
  uint16_t overruns = 0;
  uint8_t faults = Config::FAULT_NOT_READY;
};
struct ControlStatus {
  bool paused;
  uint16_t meanX10, targetAngle;
};
enum class Command : uint8_t { None, Resume, TogglePlot, Open, Status };
void begin();
Command command();
void event(const char *name, uint32_t now, uint32_t value);
void plot(uint32_t now, const Sample &sample, const Hand::Thresholds &thresholds);
void togglePlot();
uint32_t dropped();
void requestStatus();
void update(uint32_t now, const ControlStatus &status);
}
#endif
