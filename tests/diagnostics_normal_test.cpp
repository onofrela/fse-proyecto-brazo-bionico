#include <cassert>
#include <Arduino.h>
#include "TestSupport.h"
#include "../Inmoov_EMG/Diagnostics.h"

TestSerial Serial;
static_assert(EMG_SERIAL_DEBUG == 0, "El firmware normal debe excluir UART por defecto");

int main() {
  Serial.pending = true; Serial.input = 'p';
  Diagnostics::begin();
  assert(Diagnostics::command() == Diagnostics::Command::None);
  Diagnostics::event("TEST", 1, 2);
  Diagnostics::togglePlot();
  Diagnostics::plot(100, {}, {5, 10});
  Diagnostics::requestStatus();
  Diagnostics::update(200, {false, 64, 90});
  result("UART-NORMAL", "version normal sin inicio, lectura ni escritura UART",
         "sin baud, comando sin consumir y cero escrituras/descartes",
         Serial.baud == 0 && Serial.pending && Serial.writes == 0 && Diagnostics::dropped() == 0);
}
