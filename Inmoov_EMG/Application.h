#ifndef INMOOV_APPLICATION_H
#define INMOOV_APPLICATION_H
#include "Signal.h"
#include "Motion.h"
#include "Inputs.h"
#include "Diagnostics.h"
#include "SampleQueue.h"
namespace Hand {
// Coordinador: adquisicion, decision por promedio y rampa tienen duenos separados.
class Application {
 public:
  void begin(uint32_t now);
  void tick(uint32_t now);
  struct Status {
    bool emgMode, healthy, outputFault, storageFault, closing, paused;
    uint16_t position, target, meanX10;
  };
  Status status() const;
 private:
  void requestPosition(uint16_t target, uint32_t now);
  void updateUi(uint32_t now);
  void updateLed(uint32_t now);
  void consume(const RawSample &raw, uint32_t now);
  void signalLost(uint32_t now);
  void reportSignal(uint32_t now);
  Ad8232Signal signal_;
  Motion motion_;
  ThreeLevelControl levels_;
  DebouncedInput button_, selector_;
  Diagnostics::Sample sample_;
  uint32_t lastSampleMs_ = 0, lastUiMs_ = 0;
  bool haveSample_ = false, healthy_ = false, reportedHealthy_ = false;
  bool emgMode_ = false, paused_ = false;
  bool outputFault_ = false, acquisitionFault_ = false, storageFault_ = false;
  bool dataLoss_ = false;
};
}
#endif
