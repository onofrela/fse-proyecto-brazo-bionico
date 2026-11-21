#ifndef ARDUINO // INMOOV_NATIVE_MODULE
// Arduino IDE usa las definiciones integradas en Inmoov_EMG.ino.
#include <Arduino.h>
#include <EEPROM.h>
#include <avr/eeprom.h>
#include "Storage.h"
Storage::Settings Storage::load() {
  Settings result = {0, false};
  if (EEPROM.length() < Config::EEPROM_REQUIRED_BYTES) return result;
  uint32_t savedCount, inverse;
  EEPROM.get(Config::EEPROM_BOOT_ADDRESS, savedCount);
  EEPROM.get(Config::EEPROM_BOOT_INVERSE_ADDRESS, inverse);
  // UINT32_MAX se reserva como invalido; el ultimo contador valido se satura.
  constexpr uint32_t MAX_BOOT_COUNT = Config::INVALID_BOOT_COUNT - 1;
  const bool valid = (savedCount ^ inverse) == UINT32_MAX &&
                     savedCount != Config::INVALID_BOOT_COUNT;
  result.bootCount = !valid ? 1 : savedCount < MAX_BOOT_COUNT ? savedCount + 1 : MAX_BOOT_COUNT;
  inverse = ~result.bootCount;
  // Solo al arranque, antes de habilitar ADC: aqui se permite esperar EEPROM.
  EEPROM.put(Config::EEPROM_BOOT_ADDRESS, result.bootCount);
  EEPROM.put(Config::EEPROM_BOOT_INVERSE_ADDRESS, inverse);
  eeprom_busy_wait();
  uint32_t actualCount, actualInverse;
  EEPROM.get(Config::EEPROM_BOOT_ADDRESS, actualCount);
  EEPROM.get(Config::EEPROM_BOOT_INVERSE_ADDRESS, actualInverse);
  result.bootVerified = actualCount == result.bootCount && actualInverse == inverse;
  return result;
}
#endif // INMOOV_NATIVE_MODULE
