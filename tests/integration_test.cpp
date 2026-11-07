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
  Fake::reset(); Fake::selected = true; Hand::Application app; app.begin(0);
  Fake::feed(app, 1, 2000);
  assert(app.status().healthy && !app.status().closing);
  uint32_t closeAt = 0;
  for (uint32_t now = 2001; now < 2101; ++now) {
    Fake::feed(app, now, 1, 100);
    if (app.status().closing && !closeAt) closeAt = now;
  }
  result("E2E-01", "ADC sintetico -> cola -> filtro -> umbral -> servo y LED", "cierre con primer paso y LED verde", closeAt && Fake::position > 0 && Fake::green);
  result("R1-SW", "primer paso <=50 ms desde escalon sintetico", std::to_string(closeAt - 2001) + " ms simulados (sin PWM fisico)", closeAt - 2001 <= 50);
  Fake::feed(app, 2101, 1, 0, true);
  assert(!app.status().closing && !app.status().healthy);
  Fake::feed(app, 2102, 1999);
  assert(!app.status().healthy);
  Fake::feed(app, 4101, 1);
  assert(app.status().healthy);
  Fake::feed(app, 4102, 100, 100);
  result("F1-SW", "LO alto abre; 2000 muestras y reposo permiten nuevo cierre sin reset", "apertura y recuperacion con nuevo cierre", app.status().closing);
  app.tick(4212); // >10 ms sin conversion.
  assert(!app.status().healthy && !app.status().closing);
  Fake::feed(app, 4213, 1999); assert(!app.status().healthy);
  Fake::feed(app, 6212, 1); assert(app.status().healthy);
  Fake::feed(app, 6213, 100, 100);
  result("F2-SW", "ADC detenido abre; recuperacion sin reset tras estabilizar", "timeout detectado y nuevo cierre", app.status().closing);
  for (uint32_t now = 6313; now < 6345; ++now) Fake::push(now);
  app.tick(6344);
  result("F3-SW", "cola llena inhibe EMG y descarta pendientes", "orden de apertura y alarma de perdida", !app.status().healthy && !app.status().closing && (Fake::diagnostic.faults & Config::FAULT_DATA_LOSS));
  Fake::feed(app, 6345, 2000); assert(app.status().healthy);
  Fake::push(8344, 338); app.tick(8355);
  result("F4-SW", "muestra de 11 ms no renueva salud", "muestra antigua rechazada", !app.status().healthy);
  Fake::feed(app, 8356, 2000); assert(app.status().healthy);
  Fake::push(10370); app.tick(10370);
  result("F9-SW", "hueco >10 ms seguido de muestra nueva reinicia estabilizacion", "nueva muestra aceptada sin habilitar EMG", !app.status().healthy && !app.status().closing);
}
void testCalibration(bool saveFailure) {
  Fake::reset(); Fake::selected = true; Fake::preCalibrated = false; Fake::saveOk = !saveFailure;
  Hand::Application app; app.begin(0);
  for (uint32_t now = 1; now <= 12100; ++now) {
    const auto stage = app.status().calibration;
    const int amplitude = stage == Hand::Calibration::ACTIVE || stage == Hand::Calibration::PREPARE_ACTIVE ? 100 : 2;
    Fake::feed(app, now, 1, amplitude);
  }
  assert(app.status().calibrated && Fake::saved.valid());
  if (saveFailure) result("F5-SW", "error de persistencia visible conservando control en RAM", "EEPROM_SAVE_FAILED y storageFault", app.status().storageFault && Fake::event("EEPROM_SAVE_FAILED"));
  else result("BOOT-01", "arranque sin comandos -> calibracion -> guardado", "calibracion automatica y EEPROM_SAVED", Fake::event("EEPROM_SAVED"));
}
void testHardwareFailures() {
  Fake::reset(); Fake::servoOk = false; Hand::Application servo; servo.begin(0);
  Fake::pressed = true; Fake::feed(servo, 1, 100);
  result("F6-SW", "fallo attach impide ordenar motores", "SERVO_INIT_FAILED y cero escrituras", servo.status().outputFault && Fake::writes == 0 && Fake::event("SERVO_INIT_FAILED"));
  Fake::reset(); Fake::adcOk = false; Fake::selected = true; Hand::Application adc; adc.begin(0);
  Fake::feed(adc, 1, 2100, 100);
  result("F7-SW", "fallo de arranque ADC inhibe EMG", "ADC_INIT_FAILED sin cierre", !adc.status().healthy && !adc.status().closing && Fake::event("ADC_INIT_FAILED"));
  Fake::reset(); Fake::selected = true; Hand::Application clipping; clipping.begin(0);
  Fake::feed(clipping, 1, 2000); Fake::feed(clipping, 2001, 100, 100);
  Fake::push(2101, Config::ADC_HIGH_CLIP); clipping.tick(2101);
  result("F8-SW", "recorte alto abre e invalida filtro", "sin cierre ni salud y bit de recorte", !clipping.status().closing && !clipping.status().healthy && (Fake::diagnostic.faults & Config::FAULT_CLIPPED));
  Fake::reset(); Hand::Application wrap; wrap.begin(UINT32_MAX - 20);
  Fake::feed(wrap, UINT32_MAX - 19, 2100);
  result("TIME-01", "millis cruza UINT32_MAX sin falsa falla", "salud conservada tras wrap", wrap.status().healthy);
}
int main() {
  testQueue(); testUiAndMotion(); testEndToEndAndRecovery();
  testCalibration(false); testCalibration(true); testHardwareFailures();
}
