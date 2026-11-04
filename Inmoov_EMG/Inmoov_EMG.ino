/*
 * Mano InMoov con AD8232 ECG (UNIT Electronics, alimentacion 3.3 V).
 * Deteccion de actividad muscular experimental; ver limitaciones en README.
 * Adaptacion del ejemplo Arduino Robotic Hand de Kuldip.K Gami.
 * Hardware y temporizacion: Avance 1 del proyecto FSE, Arduino UNO.
 * Ver README.md antes de alimentar el sensor o conectar los tensores.
 */
#include <Arduino.h>
#include <Servo.h>
#include <EEPROM.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "Config.h"
#include "Control.h"

#if !defined(__AVR_ATmega328P__) || F_CPU != 16000000UL
#error "Este firmware requiere ATmega328P a 16 MHz (Arduino UNO R3)."
#endif

Servo fingers[5];
Hand::Ad8232Signal signalProcessor;
Hand::Motion motion;
Hand::MuscleSwitch muscle;
Hand::Calibration calibration;
Hand::DebouncedInput button;
Hand::DebouncedInput selector;
Hand::Thresholds thresholds = {0, 0};

struct Sample {
  uint32_t sequence;
  uint16_t raw;
  uint16_t envelope;
  uint16_t baseline;
  uint16_t overruns;
  bool ready;
  bool clipped;
  bool leadsOff;
};

volatile uint32_t sampleSequence = 0;
volatile uint16_t sampleRaw = 0;
volatile uint16_t sampleEnvelope = 0;
volatile uint16_t sampleBaseline = 0;
volatile uint16_t sampleOverruns = 0;
volatile bool sampleReady = false;
volatile bool sampleClipped = false;
volatile bool sampleLeadsOff = true;

bool emgMode = false;
bool calibrated = false;
bool calibrationPending = false;
bool signalFault = false;
bool plotEnabled = false;
uint32_t bootCount = 0;
uint32_t lastSampleMs = 0;
uint32_t lastSequence = 0;
uint32_t lastUiMs = 0;
uint32_t lastPlotMs = 0;
uint32_t droppedLogs = 0;

// Muestreo por Timer2; Timer1 queda reservado a Servo y Timer0 a millis().
ISR(TIMER2_COMPA_vect) {
  if (ADCSRA & _BV(ADSC)) {
    ++sampleOverruns;
  } else {
    ADCSRA |= _BV(ADSC);
  }
}

ISR(ADC_vect) {
  const uint16_t raw = ADC;
#if EMG_SAMPLE_PROBE
  PIND = _BV(PD2); // Conmuta D2 sin digitalWrite dentro de la ISR.
#endif
  // Son entradas de 3.3 V: nunca activar INPUT_PULLUP del UNO (5 V).
  const bool leadsOff = digitalRead(Config::LEADS_OFF_PLUS_PIN) == HIGH ||
                        digitalRead(Config::LEADS_OFF_MINUS_PIN) == HIGH;
  sampleRaw = raw;
  sampleEnvelope = signalProcessor.add(raw, leadsOff);
  sampleBaseline = signalProcessor.baseline();
  sampleReady = signalProcessor.ready();
  sampleClipped = signalProcessor.clipped();
  sampleLeadsOff = leadsOff;
  ++sampleSequence;
}

Sample readSample() {
  Sample s;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    s.sequence = sampleSequence;
    s.raw = sampleRaw;
    s.envelope = sampleEnvelope;
    s.baseline = sampleBaseline;
    s.overruns = sampleOverruns;
    s.ready = sampleReady;
    s.clipped = sampleClipped;
    s.leadsOff = sampleLeadsOff;
  }
  return s;
}

void startSampling() {
#if EMG_SAMPLE_PROBE
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
#endif
  ADMUX = _BV(REFS0); // AVcc, canal ADC0 (A0), resultado alineado a la derecha.
  ADCSRB = 0;
  DIDR0 |= _BV(ADC0D);
  ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
  ADCSRA |= _BV(ADSC);
  while (ADCSRA & _BV(ADSC)) {} // Solo arranque: primera conversion (~200 us).
  (void)ADC;
  ADCSRA |= _BV(ADIF) | _BV(ADIE);
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    TCCR2A = _BV(WGM21);
    TCCR2B = 0;
    TCNT2 = 0;
    OCR2A = 249; // 16 MHz / 64 / (249 + 1) = 1000 Hz.
    TIFR2 = _BV(OCF2A);
    TIMSK2 = _BV(OCIE2A);
    TCCR2B = _BV(CS22);
  }
}

void logEvent(const char *name, uint32_t now, uint32_t value) {
#if EMG_SERIAL_DEBUG
  char line[62];
  const int length = snprintf(line, sizeof(line), "E,%lu,%s,%lu\n",
                             (unsigned long)now, name, (unsigned long)value);
  if (length > 0 && length < (int)sizeof(line) &&
      Serial.availableForWrite() >= length) {
    Serial.write((const uint8_t *)line, length);
  } else {
    ++droppedLogs;
  }
#else
  (void)name;
  (void)now;
  (void)value;
#endif
}

// EEPROM: contador en 0..7; calibracion AD8232 version 2 en 16..23.
// La firma se escribe al final para rechazar una recalibracion interrumpida.
void loadSettings() {
  uint32_t savedCount, inverse;
  EEPROM.get(0, savedCount);
  EEPROM.get(4, inverse);
  bootCount = ((savedCount ^ inverse) == 0xFFFFFFFFUL &&
               savedCount != 0xFFFFFFFFUL) ? savedCount + 1 : 1;
  EEPROM.put(0, bootCount);
  inverse = ~bootCount;
  EEPROM.put(4, inverse);

  uint16_t signature, checksum;
  EEPROM.get(16, signature);
  EEPROM.get(18, thresholds.open);
  EEPROM.get(20, thresholds.close);
  EEPROM.get(22, checksum);
  calibrated = signature == Config::CALIBRATION_SIGNATURE && thresholds.valid() &&
               checksum == uint16_t(thresholds.open ^ thresholds.close ^ 0xA57C);
  if (!calibrated) thresholds = Hand::Thresholds{0, 0};
}

void saveCalibration() {
  const uint16_t invalid = 0;
  const uint16_t signature = Config::CALIBRATION_SIGNATURE;
  const uint16_t checksum = thresholds.open ^ thresholds.close ^ 0xA57C;
  EEPROM.put(16, invalid);
  EEPROM.put(18, thresholds.open);
  EEPROM.put(20, thresholds.close);
  EEPROM.put(22, checksum);
  EEPROM.put(16, signature);
}

void writeFingers() {
  for (uint8_t i = 0; i < 5; ++i) {
    fingers[i].writeMicroseconds(Hand::pulse(motion.position(),
                                Config::OPEN_US[i], Config::CLOSED_US[i]));
  }
}

void requestHand(bool closed, uint32_t now) {
  const bool changed = closed != motion.closed();
  if (motion.request(closed, now)) writeFingers();
  if (changed) logEvent(closed ? "CLOSE" : "OPEN", now, motion.position());
}

void requestCalibration(uint32_t now) {
  calibration.cancel();
  calibrationPending = true;
  muscle.reset();
  requestHand(false, now);
  logEvent("CAL_PENDING", now, 0);
}

void writeLed(bool red, bool green, bool blue) {
  digitalWrite(Config::LED_RED, red != Config::RGB_COMMON_ANODE);
  digitalWrite(Config::LED_GREEN, green != Config::RGB_COMMON_ANODE);
  digitalWrite(Config::LED_BLUE, blue != Config::RGB_COMMON_ANODE);
}

void updateLed(uint32_t now) {
  if ((signalFault && (emgMode || calibrationPending || calibration.running())) ||
      calibration.stage() == Hand::Calibration::FAILED) {
    writeLed((now / 250) % 2 == 0, false, (now / 250) % 2 == 0);
  } else if (calibrationPending || calibration.running()) {
    const bool measuring = calibration.stage() == Hand::Calibration::REST ||
                           calibration.stage() == Hand::Calibration::ACTIVE;
    const bool active = calibration.stage() == Hand::Calibration::PREPARE_ACTIVE ||
                        calibration.stage() == Hand::Calibration::ACTIVE;
    const bool on = measuring || (now / 250) % 2 == 0;
    writeLed(false, active && on, on); // Azul: reposo; cian: contraccion.
  } else if (emgMode && !calibrated) {
    writeLed((now / 250) % 2 == 0, false, (now / 250) % 2 == 0);
  } else {
    // El boton tambien cambia el indicador en la misma vuelta del super loop.
    writeLed(!emgMode, emgMode || motion.closed(), false);
  }
}

void updateUi(uint32_t now) {
  if (uint32_t(now - lastUiMs) < 5) return;
  lastUiMs = now;
  const bool modeChanged = selector.update(digitalRead(Config::MODE_PIN) == HIGH, now);
  const bool buttonChanged = button.update(digitalRead(Config::BUTTON_PIN) == LOW, now);
  if (modeChanged) {
    emgMode = selector.value();
    calibration.cancel();
    calibrationPending = false;
    muscle.reset();
    requestHand(false, now);
    logEvent("MODE", now, emgMode);
  }
  if (buttonChanged && button.value() && !modeChanged) {
    if (calibrationPending || calibration.running()) {
      calibration.cancel();
      calibrationPending = false;
      requestHand(false, now);
      logEvent("CAL_CANCEL", now, 0);
    } else if (!emgMode) {
      calibration.cancel();
      requestHand(!motion.closed(), now);
    }
  }
}

void updateCalibration(uint32_t now, const Sample &s, bool fresh, bool healthy) {
  if (calibrationPending && motion.isOpen() && healthy) {
    calibrationPending = false;
    calibration.start(now);
    logEvent("CAL_PREP_REST", now, 2000);
  }
  if (!calibration.running()) return;
  const Hand::Calibration::Stage previous = calibration.stage();
  // La calibracion arranca tras estabilizacion; cualquier perdida la invalida.
  if (!healthy) {
    calibration.update(now, 0, false);
  } else if (fresh) {
    calibration.update(now, s.envelope, true);
  }
  const Hand::Calibration::Stage stage = calibration.stage();
  if (stage == previous) return;
  switch (stage) {
    case Hand::Calibration::REST: logEvent("CAL_REST", now, 3000); break;
    case Hand::Calibration::PREPARE_ACTIVE: logEvent("CAL_PREP_CONTRACT", now, 2000); break;
    case Hand::Calibration::ACTIVE: logEvent("CAL_CONTRACT", now, 3000); break;
    case Hand::Calibration::SUCCESS:
      thresholds = calibration.thresholds();
      calibrated = true;
      saveCalibration(); // Solo con mano abierta; las ISR siguen funcionando.
      muscle.reset();
      logEvent("TH_OPEN", millis(), thresholds.open);
      logEvent("TH_CLOSE", millis(), thresholds.close);
      break;
    case Hand::Calibration::FAILED:
      muscle.reset();
      logEvent("CAL_FAILED", now, 0); // Conserva calibracion anterior en EEPROM.
      break;
    default: break;
  }
}

void updateSerial(uint32_t now, const Sample &s) {
#if EMG_SERIAL_DEBUG
  // A lo sumo un byte por vuelta para no monopolizar el super loop.
  if (Serial.available()) {
    const char command = Serial.read();
    if (command == 'c' || command == 'C') requestCalibration(now);
    if (command == 'p' || command == 'P') plotEnabled = !plotEnabled;
    if (command == 'x' || command == 'X') {
      calibration.cancel();
      calibrationPending = false;
      muscle.reset();
      requestHand(false, now);
    }
  }
  if (plotEnabled && uint32_t(now - lastPlotMs) >= 100) {
    lastPlotMs = now;
    char line[64];
    const int length = snprintf(line, sizeof(line), "D,%lu,%u,%u,%u,%u,%u,%u,%u,%lu\n",
        (unsigned long)now, s.raw, s.baseline, s.envelope,
        thresholds.open, thresholds.close,
        unsigned((s.leadsOff ? 1 : 0) | (s.clipped ? 2 : 0) | (!s.ready ? 4 : 0) |
                 (uint32_t(now - lastSampleMs) > 10 ? 8 : 0)),
        s.overruns, (unsigned long)droppedLogs);
    if (length > 0 && length < (int)sizeof(line) &&
        Serial.availableForWrite() >= length) {
      Serial.write((const uint8_t *)line, length);
    } else {
      ++droppedLogs;
    }
  }
#else
  (void)now;
  (void)s;
#endif
}

void setup() {
#if EMG_SERIAL_DEBUG
  Serial.begin(115200);
#endif
  pinMode(Config::BUTTON_PIN, INPUT_PULLUP);
  pinMode(Config::MODE_PIN, INPUT_PULLUP);
  pinMode(Config::LEADS_OFF_PLUS_PIN, INPUT);
  pinMode(Config::LEADS_OFF_MINUS_PIN, INPUT);
  pinMode(Config::LED_RED, OUTPUT);
  pinMode(Config::LED_GREEN, OUTPUT);
  pinMode(Config::LED_BLUE, OUTPUT);
  loadSettings();
  for (uint8_t i = 0; i < 5; ++i) {
    fingers[i].writeMicroseconds(Config::OPEN_US[i]);
    fingers[i].attach(Config::SERVO_PINS[i]);
    fingers[i].writeMicroseconds(Config::OPEN_US[i]);
  }
  const uint32_t now = millis();
  emgMode = digitalRead(Config::MODE_PIN) == HIGH;
  selector.begin(emgMode, now);
  button.begin(digitalRead(Config::BUTTON_PIN) == LOW, now);
  lastSampleMs = now;
  startSampling();
  logEvent("BOOT", now, bootCount);
  logEvent("MODE", now, emgMode);
  // Sin USB: mantener D8 pulsado al encender inicia la calibracion.
  if (!calibrated || button.value()) requestCalibration(now);
  updateLed(now);
}

void loop() {
  const uint32_t now = millis();
  const Sample s = readSample();
  const bool fresh = s.sequence != lastSequence;
  if (fresh) {
    lastSequence = s.sequence;
    lastSampleMs = now;
  }
  const bool healthy = s.ready && !s.clipped && !s.leadsOff &&
                       uint32_t(now - lastSampleMs) <= 10;
  const bool fault = !healthy;
  if (fault != signalFault) {
    signalFault = fault;
    logEvent(fault ? "SIGNAL_FAULT" : "SIGNAL_RECOVERED", now, s.raw);
  }
  updateUi(now);
  updateSerial(now, s);
  updateCalibration(now, s, fresh, healthy);
  if (emgMode && !calibrationPending && !calibration.running()) {
    if (!healthy || !calibrated || calibration.stage() == Hand::Calibration::FAILED) {
      muscle.reset();
      requestHand(false, now);
    } else if (fresh) {
      requestHand(muscle.update(s.envelope, thresholds), now);
    }
  }
  if (motion.update(now)) writeFingers();
  updateLed(now);
}
