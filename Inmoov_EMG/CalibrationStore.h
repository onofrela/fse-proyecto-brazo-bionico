#ifndef INMOOV_CALIBRATION_STORE_H
#define INMOOV_CALIBRATION_STORE_H
#include "Decision.h"
namespace Hand {
enum class SaveResult : uint8_t { Idle, Busy, Saved, Failed };

// Adaptador de bytes: length/read/update/ready. La firma se confirma AL FINAL.
// Decision D03: como maximo un byte EEPROM por vuelta, solo si esta disponible.
// Esto evita esperar decenas de ms en EEPROM.put durante la operacion.
template <class Memory> class CalibrationStore {
 public:
  explicit CalibrationStore(Memory &memory) : memory_(memory) {}
  bool load(Thresholds &thresholds) {
    thresholds = Thresholds{0, 0};
    if (memory_.length() < Config::EEPROM_REQUIRED_BYTES || !memory_.ready()) return false;
    const Thresholds candidate = {readWord(Config::EEPROM_OPEN_ADDRESS),
                                  readWord(Config::EEPROM_CLOSE_ADDRESS)};
    if (readWord(Config::EEPROM_CAL_ADDRESS) != Config::CALIBRATION_SIGNATURE ||
        !candidate.valid() || readWord(Config::EEPROM_CHECKSUM_ADDRESS) !=
        uint16_t(candidate.open ^ candidate.close ^ Config::CALIBRATION_CHECK_SALT))
      return false;
    thresholds = candidate;
    return true;
  }
  bool start(const Thresholds &thresholds) {
    if (result_ == SaveResult::Busy || !thresholds.valid() ||
        memory_.length() < Config::EEPROM_REQUIRED_BYTES) return false;
    pending_ = thresholds;
    step_ = 0;
    verifying_ = false;
    result_ = SaveResult::Busy;
    return true;
  }
  SaveResult poll() {
    if (result_ != SaveResult::Busy || !memory_.ready()) return result_;
    if (verifying_) {
      if (memory_.read(address()) != value()) { result_ = SaveResult::Failed; return result_; }
      ++step_;
      verifying_ = false;
      if (step_ == WRITE_STEPS) { result_ = SaveResult::Saved; return result_; }
    }
    memory_.update(address(), value()); // Sin espera: ready() ya fue comprobado.
    verifying_ = true;
    return result_;
  }
 private:
  static constexpr uint8_t WORD_BYTES = sizeof(uint16_t);
  static constexpr uint8_t INVALIDATE_STEPS = WORD_BYTES;
  static constexpr uint8_t PAYLOAD_STEPS = 3 * WORD_BYTES;
  static constexpr uint8_t WRITE_STEPS = 2 * WORD_BYTES + PAYLOAD_STEPS;
  uint16_t readWord(int address) {
    return uint16_t(memory_.read(address)) | (uint16_t(memory_.read(address + 1)) << 8);
  }
  int address() const {
    if (step_ < INVALIDATE_STEPS) return Config::EEPROM_CAL_ADDRESS + step_;
    if (step_ < INVALIDATE_STEPS + PAYLOAD_STEPS)
      return Config::EEPROM_OPEN_ADDRESS + step_ - INVALIDATE_STEPS;
    return Config::EEPROM_CAL_ADDRESS + step_ - INVALIDATE_STEPS - PAYLOAD_STEPS;
  }
  uint8_t value() const {
    if (step_ < INVALIDATE_STEPS) return Config::INVALID_SIGNATURE;
    uint16_t word;
    if (step_ >= INVALIDATE_STEPS + PAYLOAD_STEPS) word = Config::CALIBRATION_SIGNATURE;
    else if (step_ < INVALIDATE_STEPS + WORD_BYTES) word = pending_.open;
    else if (step_ < INVALIDATE_STEPS + 2 * WORD_BYTES) word = pending_.close;
    else word = pending_.open ^ pending_.close ^ Config::CALIBRATION_CHECK_SALT;
    return uint8_t(word >> ((step_ % WORD_BYTES) * 8));
  }
  Memory &memory_;
  Thresholds pending_ = {0, 0};
  uint8_t step_ = 0;
  bool verifying_ = false;
  SaveResult result_ = SaveResult::Idle;
};
}
#endif
