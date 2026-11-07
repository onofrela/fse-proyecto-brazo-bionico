#include <Arduino.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "Acquisition.h"

#if !defined(__AVR_ATmega328P__) || F_CPU != 16000000UL
#error "Este firmware requiere ATmega328P a 16 MHz (Arduino UNO R3)."
#endif
static_assert(sizeof(Hand::RawSample) == 7, "Revisar presupuesto de SRAM de la cola");
static_assert(F_CPU / Config::TIMER2_PRESCALER /
              (Config::TIMER2_COMPARE + 1UL) == Config::SAMPLE_HZ,
              "Timer2 no coincide con la frecuencia de muestreo");
namespace {
Hand::SampleQueue<Config::SAMPLE_QUEUE_CAPACITY> queue;
volatile uint16_t lostSamples = 0;
volatile bool dataLoss = false;
void recordLoss() {
  if (lostSamples != UINT16_MAX) ++lostSamples; // Contador saturado, sin wrap.
  dataLoss = true;
}
}

// Prioridad de hardware: ISR breves, sin filtros, UART ni EEPROM.
// AVR no anida estas ISR; el orden de vectores resuelve eventos simultaneos.
ISR(TIMER2_COMPA_vect) {
  // ADIF pendiente tambien significa que aun no se ha recogido la conversion.
  if (ADCSRA & (_BV(ADSC) | _BV(ADIF))) recordLoss();
  else ADCSRA |= _BV(ADSC);
}
ISR(ADC_vect) {
  const uint16_t raw = ADC;
#if EMG_SAMPLE_PROBE
  PIND = _BV(PD2); // D2: 1 ms entre flancos; periodo completo de 2 ms.
#endif
  const bool leadsOff = digitalRead(Config::LEADS_OFF_PLUS_PIN) == HIGH ||
                        digitalRead(Config::LEADS_OFF_MINUS_PIN) == HIGH;
  const Hand::RawSample sample = {millis(), raw, leadsOff};
  if (!queue.push(sample)) recordLoss();
}

bool Acquisition::begin() {
  pinMode(Config::LEADS_OFF_PLUS_PIN, INPUT); // Sin pull-up de 5 V.
  pinMode(Config::LEADS_OFF_MINUS_PIN, INPUT);
#if EMG_SAMPLE_PROBE
  pinMode(Config::SAMPLE_PROBE_PIN, OUTPUT);
  digitalWrite(Config::SAMPLE_PROBE_PIN, LOW);
#endif
  ADMUX = _BV(REFS0); // AVcc, ADC0, alineacion derecha.
  ADCSRB = 0;
  DIDR0 |= _BV(ADC0D);
  const uint8_t adcConfiguration = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
  ADCSRA = adcConfiguration;
  ADCSRA |= _BV(ADSC);
  const uint32_t startedUs = micros();
  while (ADCSRA & _BV(ADSC)) {
    if (uint32_t(micros() - startedUs) >= Config::ADC_START_TIMEOUT_US) {
      ADCSRA = 0; // Fallo de arranque: no bloquear indefinidamente.
      return false;
    }
  }
  (void)ADC;
  if (ADMUX != _BV(REFS0) || (ADCSRA & adcConfiguration) != adcConfiguration) {
    ADCSRA = 0;
    return false;
  }
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    queue.clear();
    dataLoss = false;
    ADCSRA |= _BV(ADIF) | _BV(ADIE); // Limpiar bandera antes de habilitar Timer2.
    TCCR2A = _BV(WGM21);
    TCCR2B = 0;
    TCNT2 = 0;
    OCR2A = Config::TIMER2_COMPARE;
    TIFR2 = _BV(OCF2A);
    TIMSK2 = _BV(OCIE2A);
    TCCR2B = _BV(CS22);
  }
  const bool configured = TCCR2A == _BV(WGM21) && TCCR2B == _BV(CS22) &&
                          OCR2A == Config::TIMER2_COMPARE && TIMSK2 == _BV(OCIE2A);
  if (!configured) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { TIMSK2 = 0; TCCR2B = 0; ADCSRA = 0; }
  }
  return configured;
}
bool Acquisition::take(Hand::RawSample &sample) {
  bool present;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { present = queue.pop(sample); }
  return present;
}
bool Acquisition::takeDataLoss() {
  bool lost;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { lost = dataLoss; dataLoss = false; }
  return lost;
}
void Acquisition::discardPending() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { queue.clear(); }
}
uint16_t Acquisition::overruns() {
  uint16_t count;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { count = lostSamples; }
  return count;
}
