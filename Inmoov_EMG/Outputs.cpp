#include <Arduino.h>
#include <Servo.h>
#include "Outputs.h"
#include "Config.h"
#include "Motion.h"
namespace {
Servo fingers[Config::SERVO_COUNT];
bool available = false;
void detachAll() {
  for (uint8_t finger = 0; finger < Config::SERVO_COUNT; ++finger)
    if (fingers[finger].attached()) fingers[finger].detach();
  available = false;
}
}
bool Outputs::begin() {
  pinMode(Config::BUTTON_PIN, INPUT_PULLUP);
  pinMode(Config::MODE_PIN, INPUT_PULLUP);
  pinMode(Config::LED_RED, OUTPUT);
  pinMode(Config::LED_GREEN, OUTPUT);
  pinMode(Config::LED_BLUE, OUTPUT);
  led(false, false, false);
  // Validar todos los limites antes de activar el primer motor.
  for (uint8_t finger = 0; finger < Config::SERVO_COUNT; ++finger) {
    if (Config::OPEN_US[finger] < Config::SERVO_MIN_US ||
        Config::OPEN_US[finger] > Config::SERVO_MAX_US ||
        Config::CLOSED_US[finger] < Config::SERVO_MIN_US ||
        Config::CLOSED_US[finger] > Config::SERVO_MAX_US) return false;
  }
  for (uint8_t finger = 0; finger < Config::SERVO_COUNT; ++finger) {
    // Precargar apertura evita enviar el pulso central por defecto al conectar.
    fingers[finger].writeMicroseconds(Config::OPEN_US[finger]);
    if (fingers[finger].attach(Config::SERVO_PINS[finger]) == INVALID_SERVO ||
        !fingers[finger].attached()) {
      detachAll(); // Deshacer arranque parcial si falta un canal.
      return false;
    }
    fingers[finger].writeMicroseconds(Config::OPEN_US[finger]);
  }
  available = true;
  return true;
}
bool Outputs::setPosition(uint16_t position) {
  if (!available || position > Config::POSITION_FULL_SCALE) return false;
  for (uint8_t finger = 0; finger < Config::SERVO_COUNT; ++finger) {
    if (!fingers[finger].attached()) { detachAll(); return false; }
    fingers[finger].writeMicroseconds(Hand::pulse(position,
        Config::OPEN_US[finger], Config::CLOSED_US[finger]));
  }
  // writeMicroseconds y GPIO no devuelven estado ni miden posicion fisica.
  return true;
}
void Outputs::led(bool red, bool green, bool blue) {
  digitalWrite(Config::LED_RED, red != Config::RGB_COMMON_ANODE);
  digitalWrite(Config::LED_GREEN, green != Config::RGB_COMMON_ANODE);
  digitalWrite(Config::LED_BLUE, blue != Config::RGB_COMMON_ANODE);
}
bool Outputs::emgSelected() { return digitalRead(Config::MODE_PIN) == HIGH; }
bool Outputs::buttonPressed() { return digitalRead(Config::BUTTON_PIN) == LOW; }
