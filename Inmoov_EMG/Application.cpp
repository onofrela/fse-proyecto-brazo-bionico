#include "Application.h"
#include "Acquisition.h"
#include "Outputs.h"
#include "Storage.h"

void Hand::Application::begin(uint32_t now) {
  Diagnostics::begin();
  outputFault_ = !Outputs::begin();
  const Storage::Settings settings = Storage::load();
  thresholds_ = settings.thresholds;
  calibrated_ = settings.calibrated;
  storageFault_ = !settings.bootVerified;
  emgMode_ = Outputs::emgSelected();
  selector_.begin(emgMode_, now);
  button_.begin(Outputs::buttonPressed(), now);
  acquisitionFault_ = !Acquisition::begin();
  Diagnostics::event("BOOT", now, settings.bootCount);
  Diagnostics::event("MODE", now, emgMode_);
  if (outputFault_) Diagnostics::event("SERVO_INIT_FAILED", now, 0);
  if (acquisitionFault_) Diagnostics::event("ADC_INIT_FAILED", now, 0);
  if (storageFault_) Diagnostics::event("EEPROM_BOOT_FAILED", now, 0);
  // D04: control autonomo con calibracion guardada; medir solo al solicitarlo.
  if (button_.value()) requestCalibration(now);
  else if (!calibrated_) Diagnostics::event("CAL_REQUIRED", now, 0);
  updateLed(now);
}
Hand::Application::Status Hand::Application::status() const {
  return {emgMode_, calibrated_, healthy_, outputFault_, storageFault_,
          motion_.closed(), motion_.position(), calibration_.stage()};
}
void Hand::Application::requestHand(bool closed, uint32_t now) {
  if (outputFault_) return;
  const bool changed = closed != motion_.closed();
  if (motion_.request(closed, now) && !Outputs::setPosition(motion_.position())) {
    outputFault_ = true;
    Diagnostics::event("SERVO_WRITE_FAILED", now, 0);
  }
  if (changed) Diagnostics::event(closed ? "CLOSE" : "OPEN", now, motion_.position());
}
void Hand::Application::requestCalibration(uint32_t now) {
  if (saving_ || outputFault_) { Diagnostics::event("CAL_BUSY", now, 0); return; }
  calibration_.cancel();
  calibrationPending_ = true;
  muscle_.reset();
  requestHand(false, now);
  Diagnostics::event("CAL_PENDING", now, 0);
}
void Hand::Application::cancelCalibration(uint32_t now) {
  calibration_.cancel();
  calibrationPending_ = false;
  muscle_.reset();
  requestHand(false, now);
}
void Hand::Application::updateUi(uint32_t now) {
  if (uint32_t(now - lastUiMs_) < Config::UI_PERIOD_MS) return;
  lastUiMs_ = now;
  const bool changed = selector_.update(Outputs::emgSelected(), now);
  const bool pressed = button_.update(Outputs::buttonPressed(), now);
  if (changed) {
    emgMode_ = selector_.value();
    cancelCalibration(now);
    Diagnostics::event("MODE", now, emgMode_);
  }
  if (pressed && button_.value() && !changed) {
    if (calibrationPending_ || calibration_.running()) {
      cancelCalibration(now);
      Diagnostics::event("CAL_CANCEL", now, 0);
    } else if (!emgMode_) {
      calibration_.cancel();
      requestHand(!motion_.closed(), now);
    }
  }
}
void Hand::Application::signalLost(uint32_t now) {
  signal_.reset();
  healthy_ = false;
  sample_.envelope = 0;
  sample_.faults |= Config::FAULT_NOT_READY;
  muscle_.reset();
  if (calibration_.running()) updateCalibration(now);
  if (emgMode_ || calibrationPending_) requestHand(false, now);
}
void Hand::Application::consume(const RawSample &raw, uint32_t now) {
  if (uint32_t(now - raw.capturedMs) > Config::SAMPLE_TIMEOUT_MS) {
    dataLoss_ = true;
    signalLost(now);
    return;
  }
  if (haveSample_ && uint32_t(raw.capturedMs - lastSampleMs_) > Config::SAMPLE_TIMEOUT_MS) {
    // Detectar tambien un hueco aunque la primera muestra nueva ya este en cola.
    dataLoss_ = true;
    signalLost(now);
  }
  lastSampleMs_ = raw.capturedMs; // Edad real, no hora de lectura del consumidor.
  haveSample_ = true;
  sample_.raw = raw.raw;
  sample_.envelope = signal_.add(raw.raw, raw.leadsOff);
  sample_.baseline = signal_.baseline();
  sample_.faults = (raw.leadsOff ? Config::FAULT_LEADS_OFF : 0) |
                  (signal_.clipped() ? Config::FAULT_CLIPPED : 0) |
                  (!signal_.ready() ? Config::FAULT_NOT_READY : 0);
  healthy_ = sample_.faults == 0 && !acquisitionFault_;
  if (healthy_) dataLoss_ = false;
  // D05: cada muestra una sola vez; calibracion con hora de adquisicion.
  // La rampa usa 'now' para no acelerarse al vaciar datos acumulados.
  updateCalibration(raw.capturedMs);
  updateControl(now);
}
void Hand::Application::updateCalibration(uint32_t now) {
  if (calibrationPending_ && motion_.isOpen() && healthy_ && !outputFault_) {
    calibrationPending_ = false;
    calibration_.start(now);
    Diagnostics::event("CAL_PREP_REST", now, Config::CAL_PREPARE_MS);
  }
  if (!calibration_.running()) return;
  const Calibration::Stage previous = calibration_.stage();
  calibration_.update(now, sample_.envelope, healthy_);
  if (previous == calibration_.stage()) return;
  switch (calibration_.stage()) {
    case Calibration::REST: Diagnostics::event("CAL_REST", now, Config::CAL_REST_MS); break;
    case Calibration::PREPARE_ACTIVE:
      Diagnostics::event("CAL_PREP_CONTRACT", now, Config::CAL_PREPARE_MS); break;
    case Calibration::ACTIVE: Diagnostics::event("CAL_CONTRACT", now, Config::CAL_ACTIVE_MS); break;
    case Calibration::SUCCESS:
      thresholds_ = calibration_.thresholds();
      calibrated_ = true;
      saving_ = Storage::save(thresholds_);
      if (!saving_) { storageFault_ = true; Diagnostics::event("EEPROM_SAVE_FAILED", now, 0); }
      muscle_.reset();
      Diagnostics::event("TH_OPEN", now, thresholds_.open);
      Diagnostics::event("TH_CLOSE", now, thresholds_.close);
      break;
    case Calibration::FAILED:
      muscle_.reset();
      Diagnostics::event("CAL_FAILED", now, calibration_.reason()); break;
    default: break;
  }
}
void Hand::Application::updateControl(uint32_t now) {
  if (!emgMode_ || calibrationPending_ || calibration_.running()) return;
  if (!healthy_ || !calibrated_ || calibration_.stage() == Calibration::FAILED) {
    muscle_.reset();
    requestHand(false, now);
  } else requestHand(muscle_.update(sample_.envelope, thresholds_), now);
}
void Hand::Application::reportSignal(uint32_t now) {
  if (healthy_ == reportedHealthy_) return;
  reportedHealthy_ = healthy_;
  Diagnostics::event(healthy_ ? "SIGNAL_RECOVERED" : "SIGNAL_FAULT", now, sample_.raw);
}
void Hand::Application::updateLed(uint32_t now) {
  const bool blink = (now / Config::LED_BLINK_MS) % 2 == 0;
  if (outputFault_ || acquisitionFault_ || storageFault_ ||
      (!healthy_ && (emgMode_ || calibrationPending_ || calibration_.running())) ||
      calibration_.stage() == Calibration::FAILED || (emgMode_ && !calibrated_)) {
    Outputs::led(blink, false, blink);
  } else if (calibrationPending_ || calibration_.running()) {
    const bool measuring = calibration_.stage() == Calibration::REST ||
                           calibration_.stage() == Calibration::ACTIVE;
    const bool active = calibration_.stage() == Calibration::PREPARE_ACTIVE ||
                        calibration_.stage() == Calibration::ACTIVE;
    const bool on = measuring || blink;
    Outputs::led(false, active && on, on);
  } else Outputs::led(!emgMode_, emgMode_ || motion_.closed(), false);
}
void Hand::Application::tick(uint32_t now) {
  // D06: prioridades cooperativas: seguridad/muestras -> UI y comandos ->
  // rampa/LED -> persistencia -> UART. Solo las ISR interrumpen este bucle.
  if (Acquisition::takeDataLoss()) {
    Acquisition::discardPending();
    dataLoss_ = true;
    signalLost(now);
  }
  RawSample raw;
  // Trabajo acotado aunque lleguen muestras nuevas durante el procesamiento.
  for (uint8_t count = 0; count < Config::SAMPLE_QUEUE_CAPACITY - 1; ++count) {
    if (!Acquisition::take(raw)) break;
    consume(raw, now);
  }
  if (!haveSample_ || uint32_t(now - lastSampleMs_) > Config::SAMPLE_TIMEOUT_MS) {
    signalLost(now);
    sample_.faults |= Config::FAULT_STALE | Config::FAULT_NOT_READY;
  }
  if (dataLoss_) sample_.faults |= Config::FAULT_DATA_LOSS;
  if (acquisitionFault_ || outputFault_) sample_.faults |= Config::FAULT_HARDWARE;
  updateUi(now);
  switch (Diagnostics::command()) {
    case Diagnostics::Command::Calibrate: requestCalibration(now); break;
    case Diagnostics::Command::TogglePlot: Diagnostics::togglePlot(); break;
    case Diagnostics::Command::Open: cancelCalibration(now); break;
    case Diagnostics::Command::None: break;
  }
  if (!healthy_) updateControl(now);
  if (!outputFault_ && motion_.update(now) && !Outputs::setPosition(motion_.position())) {
    outputFault_ = true;
    Diagnostics::event("SERVO_WRITE_FAILED", now, 0);
  }
  updateLed(now);
  if (saving_) {
    const SaveResult result = Storage::poll();
    if (result == SaveResult::Saved || result == SaveResult::Failed) {
      saving_ = false;
      if (result == SaveResult::Failed) storageFault_ = true;
      Diagnostics::event(result == SaveResult::Saved ? "EEPROM_SAVED" : "EEPROM_SAVE_FAILED", now, 0);
    }
  }
  reportSignal(now);
  sample_.overruns = Acquisition::overruns();
  Diagnostics::plot(now, sample_, thresholds_);
}
