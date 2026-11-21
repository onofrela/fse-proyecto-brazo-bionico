#include <cassert>
#include <cmath>
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
bool lost, selected, pressed, servoOk, adcOk, bootOk;
uint16_t position, overruns;
unsigned writes;
bool publicationDuringTick;
Diagnostics::Command next;
std::vector<std::string> events;
Diagnostics::Sample diagnostic;
void reset() {
  queue.clear(); lost = selected = pressed = false;
  servoOk = adcOk = bootOk = true;
  position = overruns = 0; writes = 0;
  next = Diagnostics::Command::None; events.clear(); publicationDuringTick = false;
}
void push(uint32_t time, uint16_t raw = 338, bool leadsOff = false) {
  if (!queue.push({time, raw, leadsOff})) { lost = true; ++overruns; }
}
void feed(Hand::Application &app, uint32_t from, uint32_t count, int amplitude = 0,
          bool leadsOff = false) {
  for (uint32_t offset = 0; offset < count; ++offset) {
    const uint32_t now = from + offset;
    const int wave = int(amplitude*std::sin(2*3.141592653589793*100*(now%1000)/1000));
    push(now, uint16_t(338 + wave), leadsOff);
    app.tick(now);
  }
}
bool event(const char *name) { return std::find(events.begin(), events.end(), name) != events.end(); }
}
bool Acquisition::begin() { return Fake::adcOk; }
bool Acquisition::take(Hand::RawSample &sample) {
  if (!Fake::queue.pop(sample)) return false;
  if (Fake::publicationDuringTick) ++sample.capturedMs;
  return true;
}
bool Acquisition::takeDataLoss() { const bool lost = Fake::lost; Fake::lost = false; return lost; }
void Acquisition::discardPending() { Fake::queue.clear(); }
uint16_t Acquisition::overruns() { return Fake::overruns; }
bool Outputs::begin() { return Fake::servoOk; }
bool Outputs::setPosition(uint16_t position) {
  ++Fake::writes; if (!Fake::servoOk) return false; Fake::position = position; return true;
}
bool Outputs::emgSelected() { return Fake::selected; }
bool Outputs::buttonPressed() { return Fake::pressed; }
Storage::Settings Storage::load() { return {1, Fake::bootOk}; }
void Diagnostics::begin() {}
Diagnostics::Command Diagnostics::command() { const Command value = Fake::next; Fake::next = Command::None; return value; }
void Diagnostics::event(const char *name, uint32_t, uint32_t) { Fake::events.emplace_back(name); }
void Diagnostics::plot(uint32_t, const Sample &sample, const Hand::Thresholds &) { Fake::diagnostic = sample; }
void Diagnostics::togglePlot() {}
uint32_t Diagnostics::dropped() { return 0; }
void Diagnostics::requestStatus() {}
void Diagnostics::update(uint32_t, const ControlStatus &) {}

void testAutonomousThreePositions() {
  Fake::reset(); Fake::selected=true;
  Hand::Application app; app.begin(0); uint32_t now=0;
  auto feed=[&](uint32_t n,int amplitude=0,bool off=false) { Fake::feed(app,now+1,n,amplitude,off);now+=n; };
  feed(2300); assert(app.status().healthy && app.status().position==0);
  feed(1400,12);
  assert(app.status().target==500 && app.status().position==500);
  feed(1400,40); assert(app.status().target==1000 && app.status().position==1000);
  feed(1600); assert(app.status().position==0);
  assert(!Fake::event("CAL_PENDING"));
  result("E2E-LEVEL","sin calibracion: ADC -> filtro -> media -> 0/90/180","tres destinos completos sin calibracion",true);
  feed(1400,40); assert(app.status().position==1000);
  Fake::next=Diagnostics::Command::Open;app.tick(now);
  feed(1400,40);assert(app.status().paused && app.status().position==0);
  Fake::next=Diagnostics::Command::Resume;app.tick(now);
  feed(1400,40);assert(!app.status().paused && app.status().position==1000);
  result("LEVEL-PAUSE","x abre y pausa; c reactiva sin calibrar","pausa persistente incluso con actividad alta",true);
  feed(1,0,true);assert(!app.status().healthy && app.status().target==0);
  assert(Fake::event("SIGNAL_FAULT"));
  feed(1600,40);assert(!app.status().healthy && app.status().position==0);
  feed(1400,40);assert(app.status().healthy && app.status().position==1000);
  assert(Fake::event("SIGNAL_RECOVERED"));
  now+=11;app.tick(now);assert(!app.status().healthy && app.status().target==0);
  feed(3300,40);assert(app.status().position==1000);
  Fake::lost=true;app.tick(now);assert(!app.status().healthy && app.status().target==0);
  assert(Fake::diagnostic.faults & Config::FAULT_DATA_LOSS);
  result("LEVEL-SAFETY","contacto/timeout/cola abren y vacian promedio","fallas respetadas y recuperacion sin calibrar",true);
}
void testClockAndBatch() {
  for(uint32_t origin : {uint32_t(0),uint32_t(UINT32_MAX-100)}) {
    Fake::reset();Fake::selected=true;Fake::publicationDuringTick=true;
    Hand::Application app;app.begin(origin);
    Fake::feed(app,origin+1,2500);
    assert(app.status().healthy && !(Fake::diagnostic.faults & Config::FAULT_DATA_LOSS));
    Fake::publicationDuringTick=false;Fake::push(origin+2501);app.tick(origin+2512);
    assert(!app.status().healthy);
  }
  result("TIME-ISR","muestra posterior a tick no parece antigua, tambien al wrap","sin falsa perdida; 11 ms antiguos siguen rechazados",true);
  uint16_t means[2]={};unsigned index=0;
  for(unsigned batch : {1u,5u}) {
    Fake::reset();Fake::selected=true;Hand::Application app;app.begin(0);
    for(uint32_t end=batch;end<=3500;end+=batch) {
      for(uint32_t time=end-batch+1;time<=end;++time)
        Fake::push(time,uint16_t(338+(time<=2200 ? 0 : int(12*std::sin(2*3.141592653589793*100*(time%1000)/1000)))));
      app.tick(end);
    }
    assert(app.status().healthy && app.status().target==500);
    means[index++]=app.status().meanX10;
  }
  assert(means[0]==means[1]);
  result("IPC-TIME","cada muestra contada una vez en lotes 1/5","misma media y objetivo 90 grados",true);
}
void testHardwareAndManual() {
  Fake::reset();Fake::servoOk=false;Hand::Application badServo;badServo.begin(0);
  Fake::feed(badServo,1,2500,40);
  assert(badServo.status().outputFault && !Fake::writes);
  assert(Fake::event("SERVO_INIT_FAILED"));
  Fake::reset();Fake::adcOk=false;Fake::selected=true;Hand::Application badAdc;badAdc.begin(0);
  Fake::feed(badAdc,1,3500,40);assert(!badAdc.status().healthy && !badAdc.status().target);
  assert(Fake::event("ADC_INIT_FAILED"));
  Fake::reset();Fake::bootOk=false;Hand::Application badStorage;badStorage.begin(0);
  assert(badStorage.status().storageFault && !badStorage.status().outputFault);
  assert(Fake::event("EEPROM_BOOT_FAILED"));
  Fake::reset();Hand::Application writeFailure;writeFailure.begin(0);
  Fake::servoOk=false;Fake::pressed=true;Fake::feed(writeFailure,1,20);
  assert(writeFailure.status().outputFault && Fake::event("SERVO_WRITE_FAILED"));
  result("HW-UART","fallas siguen visibles sin RGB","eventos de senal, servos, ADC y EEPROM conservados",true);
  Fake::reset();Hand::Application manual;manual.begin(0);Fake::pressed=true;
  Fake::feed(manual,1,1000);assert(manual.status().position==1000);
  Fake::pressed=false;Fake::feed(manual,1001,20);Fake::pressed=true;
  Fake::feed(manual,1021,1000);assert(manual.status().position==0);
  result("HW-MANUAL","fallos de arranque inhiben salida y D8 conserva modo manual","servos/ADC fallidos y pulsaciones manuales verificados",true);
}

void testFw8Latency() {
  constexpr uint32_t LOOP_MS = 1;
  constexpr uint16_t REST_ADC = 338;
  constexpr int MID_AMPLITUDE_ADC = 12, HIGH_AMPLITUDE_ADC = 40;
  constexpr unsigned CARRIER_HZ = 100;
  constexpr double PI = 3.141592653589793;
  // La muestra SIGNAL_SETTLE_SAMPLES ya es valida y cuenta en el primer bloque.
  constexpr uint32_t REST_SAMPLES = Config::SIGNAL_SETTLE_SAMPLES +
                                  Config::LEVEL_AVERAGE_SAMPLES - 1;
  constexpr uint32_t OBSERVATION_MS = 2 * Config::LEVEL_AVERAGE_SAMPLES +
                                     Config::FILTER_WINDOW_SAMPLES;
  static_assert(Config::FIRMWARE_VERSION == 8, "Revisar LAT-FW8 al cambiar firmware");
  static_assert(Config::SAMPLE_HZ == 1000, "El bucle simula una muestra cada ms");
  static_assert(Config::LEVEL_AVERAGE_SAMPLES == 200, "Revisar las fases de LAT-FW8");
  struct Extremes {
    uint32_t best = UINT32_MAX, worst = 0;
    unsigned bestPhase = 0, worstPhase = 0;
    void add(uint32_t delay, unsigned phase) {
      if (delay < best) { best = delay; bestPhase = phase; }
      if (delay > worst) { worst = delay; worstPhase = phase; }
    }
    std::string describe() const {
      return "mejor " + std::to_string(best) + " ms (fase " + std::to_string(bestPhase) +
             "), peor " + std::to_string(worst) + " ms (fase " + std::to_string(worstPhase) + ")";
    }
  };
  std::string evidence;
  for (int amplitude : {MID_AMPLITUDE_ADC, HIGH_AMPLITUDE_ADC}) {
    Extremes targetTimes, rampTimes;
    for (unsigned phase = 0; phase < Config::LEVEL_AVERAGE_SAMPLES; ++phase) {
      Fake::reset(); Fake::selected = true;
      Hand::Application app; app.begin(0);
      // Fase = muestras de reposo ya acumuladas en el bloque actual (0..199).
      Fake::feed(app, LOOP_MS, REST_SAMPLES + phase);
      assert(app.status().healthy && app.status().meanX10 == 0);
      assert(app.status().target == 0 && app.status().position == 0 && Fake::position == 0);
      const uint32_t onset = (REST_SAMPLES + phase + 1) * LOOP_MS;
      uint32_t targetDelay = UINT32_MAX, rampDelay = UINT32_MAX;
      for (uint32_t delay = 0; delay < OBSERVATION_MS; delay += LOOP_MS) {
        const uint32_t now = onset + delay;
        // Contraccion artificial sostenida: misma onda desde el inicio en todas las fases.
        // El origen es la primera muestra ADC del estimulo, no el arranque del firmware.
        const int wave = int(amplitude * std::sin(2 * PI * CARRIER_HZ * (delay + LOOP_MS) / Config::SAMPLE_HZ));
        Fake::push(now, uint16_t(REST_ADC + wave));
        const unsigned writesBefore = Fake::writes;
        app.tick(now);
        const auto status = app.status();
        assert(status.healthy && !status.outputFault && !status.paused);
        if (targetDelay == UINT32_MAX && status.target != 0) {
          targetDelay = delay;
          assert(status.target == Config::POSITION_FULL_SCALE / 2 ||
                 status.target == Config::POSITION_FULL_SCALE);
          if (amplitude == MID_AMPLITUDE_ADC) assert(status.target == Config::POSITION_FULL_SCALE / 2);
          // Solo puede cambiar al completar un bloque; incluye la muestra de inicio.
          assert((phase + 1 + delay / LOOP_MS) % Config::LEVEL_AVERAGE_SAMPLES == 0);
        }
        // Observar la escritura al doble de Outputs, no inferirla del destino.
        if (rampDelay == UINT32_MAX && Fake::position != 0) {
          rampDelay = delay;
          assert(Fake::writes > writesBefore && Fake::position == Config::MOTION_STEP);
          assert(status.position == Fake::position);
        }
        if (targetDelay != UINT32_MAX && rampDelay != UINT32_MAX) break;
      }
      assert(targetDelay != UINT32_MAX && rampDelay != UINT32_MAX);
      assert(rampDelay == targetDelay); // FW8 ordena el primer paso en la misma vuelta.
      targetTimes.add(targetDelay, phase);
      rampTimes.add(rampDelay, phase);
    }
    assert(targetTimes.best < targetTimes.worst);
    if (!evidence.empty()) evidence += "; ";
    evidence += "amplitud " + std::to_string(amplitude) + " ADC: destino " + targetTimes.describe() +
                ", primer paso escrito " + rampTimes.describe();
  }
  result("LAT-FW8",
         "simulacion de software, bucle 1 ms, 200 fases por amplitud; ADC -> filtro -> bloque -> rampa; no acredita R1",
         evidence + "; reposo ADC 338, seno 100 Hz; fase=0..199 muestras previas; "
                    "retardo desde primera muestra de contraccion (0 ms si misma vuelta); "
                    "sin ISR/PWM ni medicion en la mano", true);
}

int main() {testAutonomousThreePositions();testClockAndBatch();testHardwareAndManual();testFw8Latency();}
