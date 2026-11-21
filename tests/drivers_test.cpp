#include <cassert>
#include "fakes/Arduino.h"
#include "fakes/Servo.h"
#include "TestSupport.h"
#include "../Inmoov_EMG/Outputs.h"
#include "../Inmoov_EMG/Diagnostics.h"
bool pinValues[20] = {};
TestSerial Serial;
int failServoPin = Config::SERVO_PINS[2], attachedServos = 0;
uint16_t servoPulses[20] = {};
int main() {
  result("DRV-F1", "fallo del tercer attach deshace los anteriores", "cero servos conectados", !Outputs::begin() && attachedServos == 0);
  failServoPin = -1;
  assert(Outputs::begin() && attachedServos == Config::SERVO_COUNT);
  for (uint8_t n = 0; n < Config::SERVO_COUNT; ++n)
    assert(servoPulses[Config::SERVO_PINS[n]] == (n < 2 ? 544 : 2400));
  assert(!Outputs::setPosition(Config::POSITION_FULL_SCALE + 1));
  assert(Outputs::setPosition(Config::POSITION_FULL_SCALE));
  for (uint8_t n = 0; n < Config::SERVO_COUNT; ++n)
    assert(servoPulses[Config::SERVO_PINS[n]] == (n < 2 ? 2400 : 544));
  result("DRV-01", "cierre con medio, anular y menique invertidos", "2400 us en pulgar/indice; 544 us en medio/anular/menique", true);
  assert(Outputs::setPosition(500));
  for(uint8_t n=0;n<Config::SERVO_COUNT;++n) assert(servoPulses[Config::SERVO_PINS[n]]==1472);
  assert(Outputs::setPosition(0));
  for(uint8_t n=0;n<Config::SERVO_COUNT;++n)
    assert(servoPulses[Config::SERVO_PINS[n]]==(n < 2 ? 544 : 2400));
  result("DRV-MID","90 grados conservados y retorno a apertura por dedo","1472 us en cinco salidas; apertura 544/544/2400/2400/2400 us",true);
  for (uint16_t position=25; position<=Config::POSITION_FULL_SCALE; position+=25) {
    uint16_t previous[Config::SERVO_COUNT];
    for (uint8_t n=0; n<Config::SERVO_COUNT; ++n) previous[n]=servoPulses[Config::SERVO_PINS[n]];
    assert(Outputs::setPosition(position));
    for (uint8_t n=0; n<Config::SERVO_COUNT; ++n)
      assert(n < 2 ? servoPulses[Config::SERVO_PINS[n]] > previous[n] :
                     servoPulses[Config::SERVO_PINS[n]] < previous[n]);
  }
  result("DRV-REVERSE","rampa con sentidos opuestos entre los dos grupos","D3/D5 suben; D6/D10/D11 bajan en los 40 pasos",true);
  constexpr uint8_t FREE_PINS[] = {7, 12, 13};
  for (uint8_t pin : FREE_PINS)
    assert(testGpio().modeCalls[pin] == 0 && testGpio().writeCalls[pin] == 0);
  assert(testGpio().modeCalls[Config::BUTTON_PIN] > 0 &&
         testGpio().modeCalls[Config::MODE_PIN] > 0);
  result("DRV-FREE","D7/D12/D13 libres durante inicio y movimiento","sin pinMode ni digitalWrite en los tres pines",true);
  Diagnostics::begin(); assert(Serial.baud == Config::SERIAL_BAUD);
  Serial.capacity = 0; const auto before = Diagnostics::dropped();
  Diagnostics::event("TEST", 1, 2);
  result("UART-F1", "UART llena descarta linea sin llamar write", "una linea descartada y cero escrituras", Diagnostics::dropped() == before + 1 && Serial.writes == 0);
  Serial.capacity = 63; Serial.shortWrite = true;
  Diagnostics::event("TEST", 1, 2);
  result("UART-F2", "escritura corta se contabiliza como error", "contador de descartes incrementado", Diagnostics::dropped() == before + 2);
  Serial.shortWrite = false; Serial.pending = true; Serial.input = -1;
  assert(Diagnostics::command() == Diagnostics::Command::None);
  assert(Diagnostics::dropped() == before + 3);
  Serial.pending = true; Serial.input = 'c';
  assert(Diagnostics::command() == Diagnostics::Command::Resume);
  Serial.pending = true; Serial.input = '!';
  assert(Diagnostics::command() == Diagnostics::Command::None);
  result("UART-F3", "read=-1 y comando invalido no generan orden", "ambos rechazados; c reactiva control fijo", Diagnostics::dropped() == before + 4);
  Diagnostics::togglePlot(); Diagnostics::Sample sample;
  sample.raw = 338; sample.baseline = 338;
  Diagnostics::plot(100, sample, {20, 40});
  result("UART-01", "linea de datos completa al recuperar capacidad", Serial.last.substr(0, Serial.last.size() - 1), Serial.last.find("D,100,338,338,") == 0);
  Serial.pending=true; Serial.input='?';
  assert(Diagnostics::command()==Diagnostics::Command::Status);
  Diagnostics::requestStatus();
  const Diagnostics::ControlStatus state={false,64,90};
  Serial.capacity=0; const auto count=Serial.writes, lost=Diagnostics::dropped();
  Diagnostics::update(101,state);
  assert(Serial.writes==count && Diagnostics::dropped()==lost);
  Serial.capacity=63; Diagnostics::update(102,state);
  result("UART-STATUS", "consulta FW sin orden motora y sin bloquear TX", Serial.last.substr(0,Serial.last.size()-1), Serial.last.find("E,102,FW,8")==0);
  for(uint32_t now=103;now<112;++now) Diagnostics::update(now,state);
  Diagnostics::update(112,state);
  result("UART-REVERSE","consulta identifica los tres servos invertidos","SERVO_REVERSE_MASK=28",Serial.last.find("E,112,SERVO_REVERSE_MASK,28")==0);
  Diagnostics::plot(400,sample,{8,12}); Diagnostics::update(400,state);
  result("UART-MEAN", "D seguido de promedio en decimas", Serial.last.substr(0,Serial.last.size()-1), Serial.last.find("E,400,MEAN_X10,64")==0);
}
