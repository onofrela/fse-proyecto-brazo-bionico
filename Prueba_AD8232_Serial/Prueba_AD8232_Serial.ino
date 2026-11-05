/*
 * Lectura directa del AD8232 del proyecto, sin calibracion ni servos.
 * Arduino UNO: OUTPUT=A0, LO+=A1, LO-=A2, alimentacion AD8232=3.3 V.
 * Monitor Serie / Serial Plotter: 115200 baudios.
 */
#include <Arduino.h>

const byte PIN_SENAL = A0;
const byte PIN_LO_MAS = A1;
const byte PIN_LO_MENOS = A2;
const unsigned long PERIODO_US = 2000UL; // Muestreo nominal: 500 Hz.

unsigned long siguienteMuestraUs;

void setup() {
  pinMode(PIN_SENAL, INPUT);
  // No usar INPUT_PULLUP: el modulo trabaja a 3.3 V.
  pinMode(PIN_LO_MAS, INPUT);
  pinMode(PIN_LO_MENOS, INPUT);
  analogReference(DEFAULT); // Referencia AVcc del UNO, nominalmente 5 V.
  Serial.begin(115200);
  siguienteMuestraUs = micros();
}

void loop() {
  const unsigned long ahora = micros();
  if (static_cast<long>(ahora - siguienteMuestraUs) < 0) return;

  siguienteMuestraUs += PERIODO_US;
  // Si hubo un retraso, retomar el ritmo sin emitir muestras en rafaga.
  if (ahora - siguienteMuestraUs < 0x80000000UL) {
    siguienteMuestraUs = ahora + PERIODO_US;
  }

  const int lectura = analogRead(PIN_SENAL);
  const bool loMas = digitalRead(PIN_LO_MAS) == HIGH;
  const bool loMenos = digitalRead(PIN_LO_MENOS) == HIGH;

  // Formato compatible con Serial Plotter. Se conserva la lectura incluso
  // si hay un electrodo desconectado; LO_mas/LO_menos indican ese estado.
  Serial.print(F("ADC:"));
  Serial.print(lectura);
  Serial.print(F("\tLO_mas:"));
  Serial.print(loMas ? 1 : 0);
  Serial.print(F("\tLO_menos:"));
  Serial.println(loMenos ? 1 : 0);
}
