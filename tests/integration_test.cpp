#include <cassert>
#include <deque>
#include <string>
#include <vector>
#include <algorithm>
#include "TestSupport.h"
#include "../Inmoov_EMG/Application.h"
#include "../Inmoov_EMG/Acquisition.h"
#include "../Inmoov_EMG/Outputs.h"
#include "../Inmoov_EMG/Storage.h"

namespace Fake {
Hand::SampleQueue<Config::SAMPLE_QUEUE_CAPACITY> queue;
bool lost, selected, pressed, servoOk, adcOk, saveOk, bootOk, preCalibrated;
bool red, green, blue;
uint16_t position, overruns;
unsigned writes;
Diagnostics::Command next;
Hand::Thresholds saved;
std::vector<std::string> events;
Diagnostics::Sample diagnostic;
void reset() {
  queue.clear(); lost = selected = pressed = red = green = blue = false;
  servoOk = adcOk = saveOk = bootOk = preCalibrated = true;
  position = overruns = 0; writes = 0; saved = {0, 0};
  next = Diagnostics::Command::None; events.clear();
}
void push(uint32_t time, uint16_t raw = 338, bool leadsOff = false) {
  if (!queue.push({time, raw, leadsOff})) { lost = true; ++overruns; }
}
void feed(Hand::Application &app, uint32_t from, uint32_t count, int amplitude = 0,
          bool leadsOff = false) {
  for (uint32_t offset = 0; offset < count; ++offset) {
    const uint32_t now = from + offset;
    const int wave = (now / 10) % 2 ? amplitude : -amplitude;
    push(now, uint16_t(338 + wave), leadsOff);
    app.tick(now);
  }
}
bool event(const char *name) { return std::find(events.begin(), events.end(), name) != events.end(); }
}
bool Acquisition::begin() { return Fake::adcOk; }
bool Acquisition::take(Hand::RawSample &sample) { return Fake::queue.pop(sample); }
bool Acquisition::takeDataLoss() { const bool lost = Fake::lost; Fake::lost = false; return lost; }
void Acquisition::discardPending() { Fake::queue.clear(); }
uint16_t Acquisition::overruns() { return Fake::overruns; }
bool Outputs::begin() { return Fake::servoOk; }
bool Outputs::setPosition(uint16_t position) {
  ++Fake::writes; if (!Fake::servoOk) return false; Fake::position = position; return true;
}
void Outputs::led(bool red, bool green, bool blue) { Fake::red = red; Fake::green = green; Fake::blue = blue; }
bool Outputs::emgSelected() { return Fake::selected; }
bool Outputs::buttonPressed() { return Fake::pressed; }
Storage::Settings Storage::load() { return {{20, 40}, 1, Fake::preCalibrated, Fake::bootOk}; }
bool Storage::save(const Hand::Thresholds &thresholds) { Fake::saved = thresholds; return true; }
Hand::SaveResult Storage::poll() { return Fake::saveOk ? Hand::SaveResult::Saved : Hand::SaveResult::Failed; }
void Diagnostics::begin() {}
Diagnostics::Command Diagnostics::command() { const Command value = Fake::next; Fake::next = Command::None; return value; }
void Diagnostics::event(const char *name, uint32_t, uint32_t) { Fake::events.emplace_back(name); }
void Diagnostics::plot(uint32_t, const Sample &sample, const Hand::Thresholds &) { Fake::diagnostic = sample; }
void Diagnostics::togglePlot() {}
uint32_t Diagnostics::dropped() { return 0; }

void testQueue() {
  Hand::SampleQueue<4> queue;
  Hand::RawSample sample;
  assert(!queue.pop(sample));
  for (uint32_t n = 0; n < 3; ++n) assert(queue.push({n, uint16_t(n + 10), bool(n % 2)}));
  assert(!queue.push({9, 99, true}));
  for (uint32_t n = 0; n < 3; ++n) {
    assert(queue.pop(sample));
    assert(sample.capturedMs == n && sample.raw == n + 10 && sample.leadsOff == bool(n % 2));
  }
  for (uint32_t n = 0; n < 1000; ++n) {
    assert(queue.push({n, 123, false})); assert(queue.pop(sample)); assert(sample.capturedMs == n);
  }
  result("IPC-01", "FIFO sin sobrescritura y 1000 vueltas coherentes", "FIFO y campos completos", true);
}
void testUiAndMotion() {
  uint32_t maximumLatency = 0;
  for (uint32_t phase = 0; phase < Config::UI_PERIOD_MS; ++phase) {
    Fake::reset(); Hand::Application app; app.begin(0);
    Fake::pressed = true;
    const uint32_t pressedAt = 1 + phase;
    uint32_t closedAt = 0;
    for (uint32_t now = pressedAt; now <= 30; ++now) {
      app.tick(now);
      if (app.status().closing && !closedAt) closedAt = now;
    }
    assert(closedAt && Fake::red && Fake::green);
    maximumLatency = std::max(maximumLatency, closedAt - pressedAt);
    Fake::feed(app, 31, 800);
    assert(app.status().position == Config::POSITION_FULL_SCALE);
    assert(std::count(Fake::events.begin(), Fake::events.end(), "CLOSE") == 1);
  }
  result("R9-SW", "LED y orden <=20 ms (fases 0..4, bucle 1 ms)", std::to_string(maximumLatency) + " ms maximo simulado", maximumLatency <= 20);
  Hand::Motion motion; motion.request(true, 0);
  uint32_t finish = 0;
  for (uint32_t now = 1; now <= 1000; ++now) { motion.update(now); if (motion.position() == 1000) { finish = now; break; } }
  result("R2-SW", "recorrido ordenado <=1000 ms", std::to_string(finish) + " ms simulados", finish > 0 && finish <= 1000);
}
void testEndToEndAndRecovery() {
  Fake::reset(); Fake::selected=true; Hand::Application app; app.begin(0); uint32_t now=0;
  auto feed=[&](uint32_t count, int amplitude=0, bool leadsOff=false) {
    Fake::feed(app,now+1,count,amplitude,leadsOff); now+=count;
  };
  feed(2200); assert(app.status().healthy && !app.status().closing);
  const uint32_t started=now+1; uint32_t closeAt=0;
  for(int i=0;i<200;++i) { feed(1,100); if(app.status().closing && !closeAt) closeAt=now; }
  result("E2E-01", "ADC -> cola -> filtro -> ponderacion -> servo", "cierre con primer paso y LED verde", closeAt && Fake::position>0 && Fake::green);
  result("LAT-SW", "primer paso <200 ms sinteticos; R1 de 50 ms no demostrado", std::to_string(closeAt-started)+" ms simulados", closeAt && closeAt-started<200);
  feed(1,0,true); assert(!app.status().healthy && !app.status().closing);
  feed(1999); assert(!app.status().healthy); feed(1); assert(app.status().healthy);
  feed(200); feed(200,100);
  result("F1-SW", "falla interrumpe retencion; estabilizacion y reposo rearman", "nuevo cierre sin reset", app.status().closing);
  now+=11; app.tick(now); assert(!app.status().healthy && !app.status().closing);
  feed(2200); feed(200,100);
  result("F2-SW", "ADC detenido abre y permite recuperacion", "timeout y nuevo cierre", app.status().closing);
  for(uint32_t i=1;i<=32;++i) Fake::push(now+i);
  now+=32; app.tick(now);
  result("F3-SW", "cola llena inhibe EMG y descarta pendientes", "apertura y bit de perdida", !app.status().healthy && !app.status().closing && (Fake::diagnostic.faults & Config::FAULT_DATA_LOSS));
  feed(2200); assert(app.status().healthy);
  Fake::push(now); now+=11; app.tick(now);
  result("F4-SW", "muestra de 11 ms no renueva salud", "muestra antigua rechazada", !app.status().healthy);
  feed(2200); assert(app.status().healthy);
  now+=14; Fake::push(now); app.tick(now);
  result("F9-SW", "hueco seguido de muestra nueva reinicia estabilizacion", "no habilita EMG", !app.status().healthy && !app.status().closing);
}
uint16_t bufferedActivity(unsigned batch) {
  Fake::reset(); Fake::selected=true; Hand::Application app; app.begin(0);
  for(uint32_t end=batch;end<=2280;end+=batch) {
    for(uint32_t time=end-batch+1;time<=end;++time) {
      const int amplitude=time<=2200 ? 0 : 100;
      Fake::push(time,uint16_t(338+((time/10)%2 ? amplitude : -amplitude)));
    }
    app.tick(end);
  }
  assert(app.status().healthy);
  return app.status().activity;
}
void testBufferedDecisionClock() {
  const auto single=bufferedActivity(1), batched=bufferedActivity(5);
  result("IPC-TIME", "ponderacion identica al consumir 1 o 5 muestras por vuelta", std::to_string(single)+" / "+std::to_string(batched), single>0 && single==batched);
}
void testCalibration(bool saveFailure) {
  Fake::reset(); Fake::selected = true; Fake::preCalibrated = false; Fake::saveOk = !saveFailure;
  Fake::pressed = true; Hand::Application app; app.begin(0);
  for (uint32_t now = 1; now <= 24100; ++now) {
    const auto stage = app.status().calibration;
    const int amplitude = stage == Hand::Calibration::ACTIVE || stage == Hand::Calibration::PREPARE_ACTIVE ? 100 : 2;
    Fake::feed(app, now, 1, amplitude);
  }
  assert(app.status().calibrated && Fake::saved.valid());
  if (saveFailure) result("F5-SW", "error de persistencia visible conservando control en RAM", "EEPROM_SAVE_FAILED y storageFault", app.status().storageFault && Fake::event("EEPROM_SAVE_FAILED"));
  else result("BOOT-01", "D8 al encender -> calibracion guiada -> guardado", "calibracion solicitada y EEPROM_SAVED", Fake::event("EEPROM_SAVED"));
}
void testExplicitCalibration() {
  Fake::reset(); Fake::selected=true; Fake::preCalibrated=false;
  Hand::Application app; app.begin(0); Fake::feed(app,1,5000);
  assert(app.status().healthy && !app.status().closing && app.status().calibration==Hand::Calibration::IDLE);
  assert(!Fake::event("CAL_PENDING"));
  Fake::next=Diagnostics::Command::Calibrate; app.tick(5001); Fake::feed(app,5002,1);
  result("CAL-START", "sin umbrales espera; c inicia sin reset", "sin medicion inesperada y CAL_PREP_REST", Fake::event("CAL_PREP_REST"));
}
void testHardwareFailures() {
  Fake::reset(); Fake::servoOk = false; Hand::Application servo; servo.begin(0);
  Fake::pressed = true; Fake::feed(servo, 1, 100);
  result("F6-SW", "fallo attach impide ordenar motores", "SERVO_INIT_FAILED y cero escrituras", servo.status().outputFault && Fake::writes == 0 && Fake::event("SERVO_INIT_FAILED"));
  Fake::reset(); Fake::adcOk = false; Fake::selected = true; Hand::Application adc; adc.begin(0);
  Fake::feed(adc, 1, 2100, 100);
  result("F7-SW", "fallo de arranque ADC inhibe EMG", "ADC_INIT_FAILED sin cierre", !adc.status().healthy && !adc.status().closing && Fake::event("ADC_INIT_FAILED"));
  Fake::reset(); Fake::selected = true; Hand::Application clipping; clipping.begin(0);
  Fake::feed(clipping, 1, 2200); Fake::feed(clipping, 2201, 200, 100);
  assert(clipping.status().closing);
  Fake::push(2401, Config::ADC_HIGH_CLIP); clipping.tick(2401);
  result("F8-SW", "recorte alto abre e invalida filtro", "sin cierre ni salud y bit de recorte", !clipping.status().closing && !clipping.status().healthy && (Fake::diagnostic.faults & Config::FAULT_CLIPPED));
  Fake::reset(); Hand::Application wrap; wrap.begin(UINT32_MAX - 20);
  Fake::feed(wrap, UINT32_MAX - 19, 2100);
  result("TIME-01", "millis cruza UINT32_MAX sin falsa falla", "salud conservada tras wrap", wrap.status().healthy);
}
int main() {
  testQueue(); testUiAndMotion(); testEndToEndAndRecovery(); testBufferedDecisionClock();
  testCalibration(false); testCalibration(true); testExplicitCalibration(); testHardwareFailures();
}
