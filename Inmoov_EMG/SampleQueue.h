#ifndef INMOOV_SAMPLE_QUEUE_H
#define INMOOV_SAMPLE_QUEUE_H
#include <stdint.h>
#include "Config.h"

namespace Hand {
struct RawSample {
  uint32_t capturedMs;
  uint16_t raw;
  bool leadsOff;
};

// Decision D02: cola circular SPSC estatica, ISR productora y loop consumidor.
// volatile NO aporta atomicidad: pop/clear se invocan con interrupciones
// suspendidas por Acquisition.cpp. No hay hilos, procesos ni mutex de SO.
template <uint8_t Capacity> class SampleQueue {
 public:
  static_assert(Capacity > 1, "Se necesita una entrada y una posicion centinela");
  bool push(const RawSample &sample) {
    const uint8_t next = uint8_t((write_ + 1) % Capacity);
    if (next == read_) return false; // Llena: no sobrescribir datos pendientes.
    entries_[write_].capturedMs = sample.capturedMs;
    entries_[write_].raw = sample.raw;
    entries_[write_].leadsOff = sample.leadsOff;
    write_ = next; // Publicar solamente despues de copiar todos los campos.
    return true;
  }
  bool pop(RawSample &sample) {
    if (read_ == write_) return false;
    sample.capturedMs = entries_[read_].capturedMs;
    sample.raw = entries_[read_].raw;
    sample.leadsOff = entries_[read_].leadsOff;
    read_ = uint8_t((read_ + 1) % Capacity);
    return true;
  }
  void clear() { read_ = write_; }
 private:
  volatile RawSample entries_[Capacity] = {};
  volatile uint8_t read_ = 0;
  volatile uint8_t write_ = 0;
};
}
#endif
