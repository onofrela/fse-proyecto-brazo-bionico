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
  assert(!Outputs::setPosition(Config::POSITION_FULL_SCALE + 1));
  assert(Outputs::setPosition(Config::POSITION_FULL_SCALE));
  for (uint8_t n = 0; n < Config::SERVO_COUNT; ++n) assert(servoPulses[Config::SERVO_PINS[n]] == Config::CLOSED_US[n]);
  result("DRV-01", "5 salidas con pulso de cierre y limites verificados", "5 pulsos de 2000 us ordenados", true);
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
  assert(Diagnostics::command() == Diagnostics::Command::Calibrate);
  Serial.pending = true; Serial.input = '?';
  assert(Diagnostics::command() == Diagnostics::Command::None);
  result("UART-F3", "read=-1 y comando invalido no generan orden", "ambos rechazados; c si inicia calibracion", Diagnostics::dropped() == before + 4);
  Diagnostics::togglePlot(); Diagnostics::Sample sample;
  sample.raw = 338; sample.baseline = 338;
  Diagnostics::plot(100, sample, {20, 40});
  result("UART-01", "linea de datos completa al recuperar capacidad", Serial.last.substr(0, Serial.last.size() - 1), Serial.last.find("D,100,338,338,") == 0);
}
