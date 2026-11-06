#include <Arduino.h>
#include <EEPROM.h>
#include <avr/eeprom.h>
#include "Storage.h"
namespace {
struct EepromBytes {
  int length() const { return EEPROM.length(); }
  bool ready() const { return eeprom_is_ready(); }
  uint8_t read(int address) { return EEPROM.read(address); }
  void update(int address, uint8_t value) { EEPROM.update(address, value); }
} memory;
Hand::CalibrationStore<EepromBytes> calibrationStore(memory);
}
Storage::Settings Storage::load() {
  Settings result = {{0, 0}, 0, false, false};
  if (EEPROM.length() < Config::EEPROM_REQUIRED_BYTES) return result;
  uint32_t savedCount, inverse;
  EEPROM.get(Config::EEPROM_BOOT_ADDRESS, savedCount);
  EEPROM.get(Config::EEPROM_BOOT_INVERSE_ADDRESS, inverse);
  result.bootCount = (savedCount ^ inverse) == UINT32_MAX &&
                     savedCount < Config::INVALID_BOOT_COUNT - 1 ? savedCount + 1 : 1;
  inverse = ~result.bootCount;
  // Solo al arranque, antes de habilitar ADC: aqui se permite esperar EEPROM.
  EEPROM.put(Config::EEPROM_BOOT_ADDRESS, result.bootCount);
  EEPROM.put(Config::EEPROM_BOOT_INVERSE_ADDRESS, inverse);
  eeprom_busy_wait();
  uint32_t actualCount, actualInverse;
  EEPROM.get(Config::EEPROM_BOOT_ADDRESS, actualCount);
  EEPROM.get(Config::EEPROM_BOOT_INVERSE_ADDRESS, actualInverse);
  result.bootVerified = actualCount == result.bootCount && actualInverse == inverse;
  result.calibrated = calibrationStore.load(result.thresholds);
  return result;
}
bool Storage::save(const Hand::Thresholds &thresholds) { return calibrationStore.start(thresholds); }
Hand::SaveResult Storage::poll() { return calibrationStore.poll(); }
