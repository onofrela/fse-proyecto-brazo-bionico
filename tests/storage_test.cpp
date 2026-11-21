#include <cassert>
#include <cstring>
#include <EEPROM.h>
#include "TestSupport.h"
#include "../Inmoov_EMG/Storage.h"

TestEeprom EEPROM;

void seed(uint32_t count, uint32_t inverse) {
  EEPROM = TestEeprom{};
  std::memcpy(EEPROM.bytes.data() + Config::EEPROM_BOOT_ADDRESS, &count, sizeof(count));
  std::memcpy(EEPROM.bytes.data() + Config::EEPROM_BOOT_INVERSE_ADDRESS, &inverse, sizeof(inverse));
}
bool stored(uint32_t expected) {
  uint32_t count, inverse;
  EEPROM.get(Config::EEPROM_BOOT_ADDRESS, count);
  EEPROM.get(Config::EEPROM_BOOT_INVERSE_ADDRESS, inverse);
  return count == expected && inverse == uint32_t(~expected);
}
int main() {
  static_assert(Config::EEPROM_REQUIRED_BYTES == 8, "Solo contador e inverso");
  auto settings = Storage::load();
  assert(settings.bootVerified && settings.bootCount == 1 && stored(1));
  assert(EEPROM.waits == 1 && !EEPROM.pending);
  settings = Storage::load();
  result("EEP-01", "arranques 1 y 2 con contador/inverso y escritura verificada",
         "8 bytes suficientes; contador 2 persistido", settings.bootVerified &&
         settings.bootCount == 2 && stored(2) && !EEPROM.pending);

  // Un corte entre contador e inverso deja una pareja incoherente.
  seed(42, ~uint32_t(41));
  settings = Storage::load();
  result("EEP-F1", "registro parcial rechazado al reiniciar", "reinicio en 1 verificado",
         settings.bootVerified && settings.bootCount == 1 && stored(1));

  seed(42, ~uint32_t(42) ^ 1u);
  settings = Storage::load();
  result("EEP-F2", "inverso corrupto rechazado", "reinicio en 1 verificado",
         settings.bootVerified && settings.bootCount == 1 && stored(1));

  // Fallar un byte en cualquiera de las dos palabras debe impedir la verificacion.
  for (int address : {Config::EEPROM_BOOT_ADDRESS, Config::EEPROM_BOOT_INVERSE_ADDRESS}) {
    seed(41, ~uint32_t(41));
    EEPROM.blockedAddress = address;
    settings = Storage::load();
    assert(!settings.bootVerified && settings.bootCount == 42 && !stored(42));
  }
  result("EEP-F3", "fallo de escritura detectado por lectura de ambas palabras",
         "bootVerified=false para fallo en contador o inverso", true);

  constexpr uint32_t MAX_BOOT_COUNT = Config::INVALID_BOOT_COUNT - 1;
  seed(MAX_BOOT_COUNT - 1, ~uint32_t(MAX_BOOT_COUNT - 1));
  settings = Storage::load();
  assert(settings.bootVerified && settings.bootCount == MAX_BOOT_COUNT && stored(MAX_BOOT_COUNT));
  for (unsigned boot = 0; boot < 2; ++boot) {
    settings = Storage::load();
    assert(settings.bootVerified && settings.bootCount == MAX_BOOT_COUNT && stored(MAX_BOOT_COUNT));
  }
  result("EEP-02", "desborde saturado sin volver a 1 ni a 0",
         "UINT32_MAX-1 conservado en arranques sucesivos", true);

  seed(Config::INVALID_BOOT_COUNT, ~Config::INVALID_BOOT_COUNT);
  settings = Storage::load();
  result("EEP-F4", "valor reservado rechazado aunque el inverso coincida",
         "reinicio en 1 verificado", settings.bootVerified && settings.bootCount == 1 && stored(1));

  EEPROM = TestEeprom{};
  EEPROM.availableBytes = Config::EEPROM_REQUIRED_BYTES - 1;
  settings = Storage::load();
  result("EEP-F5", "EEPROM insuficiente no se escribe ni se verifica",
         "contador 0 y bootVerified=false sin escrituras", !settings.bootVerified &&
         settings.bootCount == 0 && EEPROM.writes == 0 && EEPROM.waits == 0);
}
