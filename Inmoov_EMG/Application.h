#ifndef INMOOV_APPLICATION_H
#define INMOOV_APPLICATION_H
#include "Control.h"
#include "Diagnostics.h"
#include "SampleQueue.h"
namespace Hand {
// Coordinador: unico propietario del estado. Los drivers no deciden movimientos.
class Application {
 public:
  void begin(uint32_t now);
  void tick(uint32_t now);
  struct Status {
    bool emgMode, calibrated, healthy, outputFault, storageFault, closing;
    uint16_t position;
    Calibration::Stage calibration;
    uint16_t activity;
  };
  Status status() const;
 private:
  void requestHand(bool closed, uint32_t now);
  void requestCalibration(uint32_t now);
  void cancelCalibration(uint32_t now);
  void updateUi(uint32_t now);
  void updateLed(uint32_t now);
  void consume(const RawSample &raw, uint32_t now);
  void updateCalibration(uint32_t now);
  void signalLost(uint32_t now);
  void reportSignal(uint32_t now);
  void updateControl(uint32_t sampleTime, uint32_t motionTime);
  Ad8232Signal signal_;
  Motion motion_;
  MuscleSwitch muscle_;
  Calibration calibration_;
  DebouncedInput button_, selector_;
  Thresholds thresholds_ = {0, 0};
  Diagnostics::Sample sample_;
  uint32_t lastSampleMs_ = 0, lastUiMs_ = 0;
  bool haveSample_ = false, healthy_ = false, reportedHealthy_ = false;
  bool emgMode_ = false, calibrated_ = false, calibrationPending_ = false;
  bool outputFault_ = false, acquisitionFault_ = false, storageFault_ = false;
  bool saving_ = false, dataLoss_ = false;
};
}
#endif
