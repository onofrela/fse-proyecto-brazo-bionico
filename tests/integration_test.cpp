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
bool red, green, blue;
uint16_t position, overruns;
unsigned writes;
bool publicationDuringTick;
Diagnostics::Command next;
std::vector<std::string> events;
Diagnostics::Sample diagnostic;
void reset() {
  queue.clear(); lost = selected = pressed = red = green = blue = false;
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
void Outputs::led(bool red, bool green, bool blue) { Fake::red = red; Fake::green = green; Fake::blue = blue; }
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
  feed(1600,40);assert(!app.status().healthy && app.status().position==0);
  feed(1400,40);assert(app.status().healthy && app.status().position==1000);
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
  Fake::reset();Fake::adcOk=false;Fake::selected=true;Hand::Application badAdc;badAdc.begin(0);
  Fake::feed(badAdc,1,3500,40);assert(!badAdc.status().healthy && !badAdc.status().target);
  Fake::reset();Hand::Application manual;manual.begin(0);Fake::pressed=true;
  Fake::feed(manual,1,1000);assert(manual.status().position==1000);
  Fake::pressed=false;Fake::feed(manual,1001,20);Fake::pressed=true;
  Fake::feed(manual,1021,1000);assert(manual.status().position==0);
  result("HW-MANUAL","fallos de arranque inhiben salida y D8 conserva modo manual","servos/ADC fallidos y pulsaciones manuales verificados",true);
}
int main() {testAutonomousThreePositions();testClockAndBatch();testHardwareAndManual();}
