/*
 * Mano InMoov con AD8232 ECG (UNIT Electronics, alimentacion 3.3 V).
 * Deteccion de actividad muscular experimental; ver limitaciones en README.
 * Adaptacion del ejemplo Arduino Robotic Hand de Kuldip.K Gami.
 * Hardware y temporizacion: Arduino UNO R3, ATmega328P a 16 MHz.
 * Ver README.md antes de alimentar el sensor o conectar los tensores.
 */
#include <Arduino.h>
#include "Config.h"
#include "Control.h"
#include "Outputs.h"
#include "Diagnostics.h"
#include "Storage.h"
#include "Acquisition.h"

#if !defined(__AVR_ATmega328P__) || F_CPU != 16000000UL
#error "Este firmware requiere ATmega328P a 16 MHz (Arduino UNO R3)."
#endif

bool outputsReady = false;
Hand::Ad8232Signal signalProcessor;
Hand::Motion motion;
Hand::MuscleSwitch muscle;
Hand::Calibration calibration;
Hand::DebouncedInput button;
Hand::DebouncedInput selector;
Hand::Thresholds thresholds = {0, 0};

struct Sample {
  uint32_t sequence;
  uint16_t raw;
  uint16_t envelope;
  uint16_t baseline;
  uint16_t overruns;
  bool ready;
  bool clipped;
  bool leadsOff;
};

Sample latestSample = {};
uint32_t lastCapturedMs = 0;
bool haveCapturedSample = false;

bool emgMode = false;
bool calibrated = false;
bool savingCalibration = false;
bool calibrationPending = false;
bool signalFault = false;
uint32_t bootCount = 0;
uint32_t lastSampleMs = 0;
uint32_t lastSequence = 0;
uint32_t lastUiMs = 0;

// Puente temporal hacia el coordinador existente: la ISR ya solo captura.
Sample readSample() {
  const uint32_t now = millis();
  if (Acquisition::takeDataLoss()) {
    Acquisition::discardPending();
    signalProcessor.reset();
    latestSample.ready = false;
  }
  Hand::RawSample raw;
  for (uint8_t count = 0; count < Config::SAMPLE_QUEUE_CAPACITY - 1; ++count) {
    if (!Acquisition::take(raw)) break;
    if (uint32_t(now - raw.capturedMs) > Config::SAMPLE_TIMEOUT_MS) {
      signalProcessor.reset();
      latestSample.ready = false;
      continue;
    }
    if (haveCapturedSample && uint32_t(raw.capturedMs - lastCapturedMs) > Config::SAMPLE_TIMEOUT_MS)
      signalProcessor.reset();
    lastCapturedMs = raw.capturedMs;
    haveCapturedSample = true;
    latestSample.raw = raw.raw;
    latestSample.leadsOff = raw.leadsOff;
    latestSample.envelope = signalProcessor.add(raw.raw, raw.leadsOff);
    latestSample.baseline = signalProcessor.baseline();
    latestSample.ready = signalProcessor.ready();
    latestSample.clipped = signalProcessor.clipped();
    ++latestSample.sequence;
  }
  latestSample.overruns = Acquisition::overruns();
  return latestSample;
}

void startSampling() {
  if (!Acquisition::begin()) logEvent("ADC_INIT_FAILED", millis(), 0);
}

void logEvent(const char *name, uint32_t now, uint32_t value) {
  Diagnostics::event(name, now, value);
}

// EEPROM: contador en 0..7; calibracion AD8232 version 2 en 16..23.
// La firma se escribe al final para rechazar una recalibracion interrumpida.
void loadSettings() {
  const Storage::Settings settings = Storage::load();
  bootCount = settings.bootCount;
  thresholds = settings.thresholds;
  calibrated = settings.calibrated;
  if (!settings.bootVerified) logEvent("EEPROM_BOOT_FAILED", millis(), 0);
}

void saveCalibration() {
  savingCalibration = Storage::save(thresholds);
  if (!savingCalibration) logEvent("EEPROM_SAVE_FAILED", millis(), 0);
}

void pollStorage(uint32_t now) {
  if (!savingCalibration) return;
  const Hand::SaveResult result = Storage::poll();
  if (result == Hand::SaveResult::Saved || result == Hand::SaveResult::Failed) {
    savingCalibration = false;
    logEvent(result == Hand::SaveResult::Saved ? "EEPROM_SAVED" : "EEPROM_SAVE_FAILED", now, 0);
  }
}

void writeFingers() {
  if (!Outputs::setPosition(motion.position())) {
    outputsReady = false;
    logEvent("SERVO_WRITE_FAILED", millis(), motion.position());
  }
}

void requestHand(bool closed, uint32_t now) {
  if (!outputsReady) return;
  const bool changed = closed != motion.closed();
  if (motion.request(closed, now)) writeFingers();
  if (changed) logEvent(closed ? "CLOSE" : "OPEN", now, motion.position());
}

void requestCalibration(uint32_t now) {
  calibration.cancel();
  calibrationPending = true;
  muscle.reset();
  requestHand(false, now);
  logEvent("CAL_PENDING", now, 0);
}

void writeLed(bool red, bool green, bool blue) {
  Outputs::led(red, green, blue);
}

void updateLed(uint32_t now) {
  if ((signalFault && (emgMode || calibrationPending || calibration.running())) ||
      calibration.stage() == Hand::Calibration::FAILED) {
    writeLed((now / 250) % 2 == 0, false, (now / 250) % 2 == 0);
  } else if (calibrationPending || calibration.running()) {
    const bool measuring = calibration.stage() == Hand::Calibration::REST ||
                           calibration.stage() == Hand::Calibration::ACTIVE;
    const bool active = calibration.stage() == Hand::Calibration::PREPARE_ACTIVE ||
                        calibration.stage() == Hand::Calibration::ACTIVE;
    const bool on = measuring || (now / 250) % 2 == 0;
    writeLed(false, active && on, on); // Azul: reposo; cian: contraccion.
  } else if (emgMode && !calibrated) {
    writeLed((now / 250) % 2 == 0, false, (now / 250) % 2 == 0);
  } else {
    // El boton tambien cambia el indicador en la misma vuelta del super loop.
    writeLed(!emgMode, emgMode || motion.closed(), false);
  }
}

void updateUi(uint32_t now) {
  if (uint32_t(now - lastUiMs) < 5) return;
  lastUiMs = now;
  const bool modeChanged = selector.update(Outputs::emgSelected(), now);
  const bool buttonChanged = button.update(Outputs::buttonPressed(), now);
  if (modeChanged) {
    emgMode = selector.value();
    calibration.cancel();
    calibrationPending = false;
    muscle.reset();
    requestHand(false, now);
    logEvent("MODE", now, emgMode);
  }
  if (buttonChanged && button.value() && !modeChanged) {
    if (calibrationPending || calibration.running()) {
      calibration.cancel();
      calibrationPending = false;
      requestHand(false, now);
      logEvent("CAL_CANCEL", now, 0);
    } else if (!emgMode) {
      calibration.cancel();
      requestHand(!motion.closed(), now);
    }
  }
}

void updateCalibration(uint32_t now, const Sample &s, bool fresh, bool healthy) {
  if (calibrationPending && motion.isOpen() && healthy) {
    calibrationPending = false;
    calibration.start(now);
    logEvent("CAL_PREP_REST", now, 2000);
  }
  if (!calibration.running()) return;
  const Hand::Calibration::Stage previous = calibration.stage();
  // La calibracion arranca tras estabilizacion; cualquier perdida la invalida.
  if (!healthy) {
    calibration.update(now, 0, false);
  } else if (fresh) {
    calibration.update(now, s.envelope, true);
  }
  const Hand::Calibration::Stage stage = calibration.stage();
  if (stage == previous) return;
  switch (stage) {
    case Hand::Calibration::REST: logEvent("CAL_REST", now, 3000); break;
    case Hand::Calibration::PREPARE_ACTIVE: logEvent("CAL_PREP_CONTRACT", now, 2000); break;
    case Hand::Calibration::ACTIVE: logEvent("CAL_CONTRACT", now, 3000); break;
    case Hand::Calibration::SUCCESS:
      thresholds = calibration.thresholds();
      calibrated = true;
      saveCalibration(); // Solo con mano abierta; las ISR siguen funcionando.
      muscle.reset();
      logEvent("TH_OPEN", millis(), thresholds.open);
      logEvent("TH_CLOSE", millis(), thresholds.close);
      break;
    case Hand::Calibration::FAILED:
      muscle.reset();
      logEvent("CAL_FAILED", now, 0); // Conserva calibracion anterior en EEPROM.
      break;
    default: break;
  }
}

void updateSerial(uint32_t now, const Sample &s) {
  switch (Diagnostics::command()) {
    case Diagnostics::Command::Calibrate: requestCalibration(now); break;
    case Diagnostics::Command::TogglePlot: Diagnostics::togglePlot(); break;
    case Diagnostics::Command::Open:
      calibration.cancel();
      calibrationPending = false;
      muscle.reset();
      requestHand(false, now);
      break;
    case Diagnostics::Command::None: break;
  }
  Diagnostics::Sample data;
  data.raw = s.raw;
  data.baseline = s.baseline;
  data.envelope = s.envelope;
  data.overruns = s.overruns;
  data.faults = (s.leadsOff ? Config::FAULT_LEADS_OFF : 0) |
                (s.clipped ? Config::FAULT_CLIPPED : 0) |
                (!s.ready ? Config::FAULT_NOT_READY : 0) |
                (uint32_t(now - lastSampleMs) > Config::SAMPLE_TIMEOUT_MS ? Config::FAULT_STALE : 0);
  Diagnostics::plot(now, data, thresholds);
}

void setup() {
  Diagnostics::begin();
  outputsReady = Outputs::begin();
  if (!outputsReady) logEvent("SERVO_INIT_FAILED", millis(), 0);
  pinMode(Config::LEADS_OFF_PLUS_PIN, INPUT);
  pinMode(Config::LEADS_OFF_MINUS_PIN, INPUT);
  loadSettings();
  const uint32_t now = millis();
  emgMode = Outputs::emgSelected();
  selector.begin(emgMode, now);
  button.begin(Outputs::buttonPressed(), now);
  lastSampleMs = now;
  startSampling();
  logEvent("BOOT", now, bootCount);
  logEvent("MODE", now, emgMode);
  if (!calibrated || button.value()) requestCalibration(now);
  updateLed(now);
}

void loop() {
  const uint32_t now = millis();
  const Sample s = readSample();
  const bool fresh = s.sequence != lastSequence;
  if (fresh) {
    lastSequence = s.sequence;
    lastSampleMs = now;
  }
  const bool healthy = s.ready && !s.clipped && !s.leadsOff &&
                       uint32_t(now - lastSampleMs) <= 10;
  const bool fault = !healthy;
  if (fault != signalFault) {
    signalFault = fault;
    logEvent(fault ? "SIGNAL_FAULT" : "SIGNAL_RECOVERED", now, s.raw);
  }
  updateUi(now);
  updateSerial(now, s);
  updateCalibration(now, s, fresh, healthy);
  if (emgMode && !calibrationPending && !calibration.running()) {
    if (!healthy || !calibrated || calibration.stage() == Hand::Calibration::FAILED) {
      muscle.reset();
      requestHand(false, now);
    } else if (fresh) {
      requestHand(muscle.update(s.envelope, thresholds), now);
    }
  }
  if (motion.update(now)) writeFingers();
  updateLed(now);
  pollStorage(now);
}
