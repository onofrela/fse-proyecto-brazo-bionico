#ifndef TEST_SERVO_H
#define TEST_SERVO_H
#include <stdint.h>
constexpr uint8_t INVALID_SERVO = UINT8_MAX;
extern int failServoPin, attachedServos;
extern uint16_t servoPulses[20];
class Servo {
 public:
  uint8_t attach(uint8_t pin) {
    if (pin == failServoPin) return INVALID_SERVO;
    pin_ = pin; attached_ = true; ++attachedServos; servoPulses[pin_] = pulse_; return pin;
  }
  bool attached() const { return attached_; }
  void detach() { if (attached_) --attachedServos; attached_ = false; }
  void writeMicroseconds(uint16_t value) { pulse_ = value; if (attached_) servoPulses[pin_] = value; }
 private:
  uint8_t pin_ = 0;
  bool attached_ = false;
  uint16_t pulse_ = 1500;
};
#endif
