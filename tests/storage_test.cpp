#include <array>
#include <cassert>
#include "TestSupport.h"
#include "../Inmoov_EMG/CalibrationStore.h"
struct Memory {
  std::array<uint8_t, Config::EEPROM_REQUIRED_BYTES> bytes;
  bool busy = false, broken = false;
  unsigned writes = 0;
  Memory() { bytes.fill(UINT8_MAX); }
  int length() { return int(bytes.size()); }
  bool ready() { return !busy; }
  uint8_t read(int address) { return bytes.at(address); }
  void update(int address, uint8_t value) { ++writes; if (!broken) bytes.at(address) = value; }
};
int main() {
  const Hand::Thresholds expected = {20, 40};
  Memory memory; Hand::CalibrationStore<Memory> store(memory); Hand::Thresholds actual;
  assert(!store.load(actual) && !actual.valid());
  assert(!store.start({50, 20}));
  assert(store.start(expected));
  memory.busy = true; assert(store.poll() == Hand::SaveResult::Busy && memory.writes == 0);
  memory.busy = false;
  for (int step = 0; step < 11; ++step) store.poll();
  assert(store.poll() == Hand::SaveResult::Saved && memory.writes == 10);
  result("EEP-01", "firma/datos verificados y 10 bytes en pasos no bloqueantes", "10 escrituras y umbrales 20/40", store.load(actual) && actual.open == 20 && actual.close == 40);
  for (unsigned cut = 1; cut < 10; ++cut) {
    Memory interrupted = memory;
    Hand::CalibrationStore<Memory> writer(interrupted); assert(writer.start({30, 60}));
    for (unsigned step = 0; step < cut; ++step) writer.poll();
    Hand::CalibrationStore<Memory> reboot(interrupted);
    assert(!reboot.load(actual));
  }
  result("EEP-F1", "cortes tras escrituras 1..9 rechazan registro parcial", "9 cortes rechazados al reiniciar", true);
  memory.bytes[Config::EEPROM_CHECKSUM_ADDRESS] ^= 1;
  result("EEP-F2", "checksum corrupto no habilita EMG", "registro rechazado", !store.load(actual));
  Memory broken; broken.broken = true; Hand::CalibrationStore<Memory> bad(broken);
  assert(bad.start(expected)); bad.poll();
  result("EEP-F3", "fallo de escritura detectado por lectura", "SaveResult::Failed", bad.poll() == Hand::SaveResult::Failed);
}
