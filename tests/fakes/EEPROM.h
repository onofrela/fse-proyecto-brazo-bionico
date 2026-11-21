#ifndef TEST_EEPROM_H
#define TEST_EEPROM_H
#include <array>
#include <cstddef>
#include <cstdint>

struct TestEeprom {
  // Ocho bytes exactos: cualquier acceso fuera del contador falla en at().
  std::array<uint8_t, 8> bytes;
  int availableBytes = int(bytes.size());
  int blockedAddress = -1;
  unsigned writes = 0, waits = 0;
  bool pending = false;
  int pendingAddress = 0;
  uint8_t pendingValue = 0;

  TestEeprom() { bytes.fill(UINT8_MAX); }
  int length() const { return availableBytes; }
  void finishWrite() {
    if (pending) bytes.at(pendingAddress) = pendingValue;
    pending = false;
  }
  template <class T> T &get(int address, T &value) {
    auto data = reinterpret_cast<uint8_t *>(&value);
    for (size_t index = 0; index < sizeof(T); ++index)
      data[index] = bytes.at(address + index);
    return value;
  }
  template <class T> const T &put(int address, const T &value) {
    const auto data = reinterpret_cast<const uint8_t *>(&value);
    for (size_t index = 0; index < sizeof(T); ++index) {
      finishWrite();
      ++writes;
      const int target = address + int(index);
      if (target == blockedAddress) continue;
      pendingAddress = target;
      pendingValue = data[index];
      pending = true; // El ultimo byte requiere eeprom_busy_wait(), como en AVR.
    }
    return value;
  }
};
extern TestEeprom EEPROM;
#endif
