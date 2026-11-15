#include "Application.h"
#include "Acquisition.h"
#include "Outputs.h"
#include "Storage.h"
namespace {
// La ISR puede publicar despues de capturar 'now'. No confundir con antiguedad.
bool sampleExpired(uint32_t now, uint32_t capturedMs) {
  return int32_t(now-capturedMs) > int32_t(Config::SAMPLE_TIMEOUT_MS);
}
}
void Hand::Application::begin(uint32_t now) {
  Diagnostics::begin();
  outputFault_ = !Outputs::begin();
  const Storage::Settings settings = Storage::load(); // Contador; umbrales previos ignorados.
  storageFault_ = !settings.bootVerified;
  emgMode_ = Outputs::emgSelected();
  selector_.begin(emgMode_,now);
  button_.begin(Outputs::buttonPressed(),now);
  acquisitionFault_ = !Acquisition::begin();
  Diagnostics::event("BOOT",now,settings.bootCount);
  Diagnostics::event("MODE",now,emgMode_);
  if (outputFault_) Diagnostics::event("SERVO_INIT_FAILED",now,0);
  if (acquisitionFault_) Diagnostics::event("ADC_INIT_FAILED",now,0);
  if (storageFault_) Diagnostics::event("EEPROM_BOOT_FAILED",now,0);
  Diagnostics::requestStatus();
  updateLed(now);
}
Hand::Application::Status Hand::Application::status() const {
  return {emgMode_,healthy_,outputFault_,storageFault_,motion_.closed(),paused_,
          motion_.position(),motion_.target(),levels_.meanX10()};
}
void Hand::Application::requestPosition(uint16_t target,uint32_t now) {
  if (outputFault_) return;
  const bool changed = target != motion_.target();
  if (motion_.requestPosition(target,now) && !Outputs::setPosition(motion_.position())) {
    outputFault_=true;
    Diagnostics::event("SERVO_WRITE_FAILED",now,0);
  }
  if (changed) Diagnostics::event("TARGET_DEG",now,uint32_t(target)*180/Config::POSITION_FULL_SCALE);
}
void Hand::Application::updateUi(uint32_t now) {
  if (uint32_t(now-lastUiMs_) < Config::UI_PERIOD_MS) return;
  lastUiMs_=now;
  const bool changed=selector_.update(Outputs::emgSelected(),now);
  const bool pressed=button_.update(Outputs::buttonPressed(),now);
  if (changed) {
    emgMode_=selector_.value(); levels_.reset(); requestPosition(0,now);
    Diagnostics::event("MODE",now,emgMode_);
  }
  if (pressed && button_.value() && !changed && !emgMode_)
    requestPosition(motion_.closed() ? 0 : Config::POSITION_FULL_SCALE,now);
}
void Hand::Application::signalLost(uint32_t now) {
  signal_.reset(); healthy_=false; sample_.envelope=0;
  sample_.faults |= Config::FAULT_NOT_READY;
  levels_.reset();
  if (emgMode_) requestPosition(0,now);
}
void Hand::Application::consume(const RawSample &raw,uint32_t now) {
  if (sampleExpired(now,raw.capturedMs)) { dataLoss_=true; signalLost(now); return; }
  if (haveSample_ && uint32_t(raw.capturedMs-lastSampleMs_) > Config::SAMPLE_TIMEOUT_MS) {
    dataLoss_=true; signalLost(now);
  }
  lastSampleMs_=raw.capturedMs; haveSample_=true;
  sample_.raw=raw.raw;
  sample_.envelope=signal_.add(raw.raw,raw.leadsOff);
  sample_.baseline=signal_.baseline();
  sample_.faults=(raw.leadsOff ? Config::FAULT_LEADS_OFF : 0) |
      (signal_.clipped() ? Config::FAULT_CLIPPED : 0) |
      (!signal_.ready() ? Config::FAULT_NOT_READY : 0);
  healthy_=sample_.faults==0 && !acquisitionFault_;
  if (healthy_) dataLoss_=false;
  if (!healthy_ || paused_) {
    levels_.reset(); if (emgMode_) requestPosition(0,now);
  } else if (emgMode_) {
    // Cada muestra una vez. Rampa con tiempo del bucle, promedio con captura real.
    requestPosition(levels_.add(sample_.envelope,raw.capturedMs),now);
  }
}
void Hand::Application::reportSignal(uint32_t now) {
  if (healthy_==reportedHealthy_) return;
  reportedHealthy_=healthy_;
  Diagnostics::event(healthy_ ? "SIGNAL_RECOVERED" : "SIGNAL_FAULT",now,sample_.raw);
}
void Hand::Application::updateLed(uint32_t now) {
  const bool blink=(now/Config::LED_BLINK_MS)%2==0;
  if (outputFault_ || acquisitionFault_ || storageFault_ || (emgMode_ && !healthy_))
    Outputs::led(blink,false,blink);
  else if (emgMode_ && paused_) Outputs::led(false,false,true);
  else Outputs::led(!emgMode_,emgMode_ || motion_.closed(),false);
}
void Hand::Application::tick(uint32_t now) {
  // Prioridad: seguridad/muestras -> entradas -> rampa/LED -> UART.
  if (Acquisition::takeDataLoss()) {
    Acquisition::discardPending(); dataLoss_=true; signalLost(now);
  }
  RawSample raw;
  for (uint8_t n=0;n<Config::SAMPLE_QUEUE_CAPACITY-1;++n) {
    if (!Acquisition::take(raw)) break;
    consume(raw,now);
  }
  if (!haveSample_ || sampleExpired(now,lastSampleMs_)) {
    signalLost(now); sample_.faults |= Config::FAULT_STALE | Config::FAULT_NOT_READY;
  }
  if (dataLoss_) sample_.faults |= Config::FAULT_DATA_LOSS;
  if (acquisitionFault_ || outputFault_) sample_.faults |= Config::FAULT_HARDWARE;
  updateUi(now);
  switch (Diagnostics::command()) {
    case Diagnostics::Command::Resume:
      paused_=false; levels_.reset(); Diagnostics::event("CONTROL_PAUSED",now,0); break;
    case Diagnostics::Command::Open:
      paused_=true; levels_.reset(); requestPosition(0,now);
      Diagnostics::event("CONTROL_PAUSED",now,1); break;
    case Diagnostics::Command::TogglePlot: Diagnostics::togglePlot(); break;
    case Diagnostics::Command::Status: Diagnostics::requestStatus(); break;
    case Diagnostics::Command::None: break;
  }
  if (!outputFault_ && motion_.update(now) && !Outputs::setPosition(motion_.position())) {
    outputFault_=true; Diagnostics::event("SERVO_WRITE_FAILED",now,0);
  }
  updateLed(now); reportSignal(now);
  sample_.overruns=Acquisition::overruns();
  Diagnostics::plot(now,sample_,{Config::LEVEL_MID_START,Config::LEVEL_HIGH_START});
  Diagnostics::update(now,{paused_,levels_.meanX10(),
      uint16_t(uint32_t(motion_.target())*180/Config::POSITION_FULL_SCALE)});
}
