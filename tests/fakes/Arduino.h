#ifndef TEST_ARDUINO_H
#define TEST_ARDUINO_H
#include <stddef.h>
#include <stdint.h>
#include <string>
constexpr uint8_t INPUT_PULLUP = 2, OUTPUT = 1, HIGH = 1, LOW = 0;
extern bool pinValues[20];
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t pin, bool value) { pinValues[pin] = value; }
inline int digitalRead(uint8_t pin) { return pinValues[pin]; }
struct TestSerial {
  int capacity = 63, input = -1;
  bool pending = false, shortWrite = false;
  unsigned writes = 0;
  uint32_t baud = 0;
  std::string last;
  void begin(uint32_t value) { baud = value; }
  int available() { return pending; }
  int availableForWrite() { return capacity; }
  int read() { pending = false; return input; }
  size_t write(const uint8_t *data, size_t length) {
    ++writes; last.assign(reinterpret_cast<const char *>(data), length);
    return shortWrite ? length - 1 : length;
  }
};
extern TestSerial Serial;
#endif
