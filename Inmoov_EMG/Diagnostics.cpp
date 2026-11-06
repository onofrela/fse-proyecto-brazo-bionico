#include <Arduino.h>
#include <stdio.h>
#include "Diagnostics.h"
namespace {
uint32_t droppedLines = 0;
bool plotEnabled = false;
#if EMG_SERIAL_DEBUG
uint32_t lastPlotMs = 0;
void send(const char *line, int length, size_t capacity) {
  if (length <= 0 || size_t(length) >= capacity || Serial.availableForWrite() < length) {
    ++droppedLines; return; // Telemetria siempre de menor prioridad que control.
  }
  if (Serial.write(reinterpret_cast<const uint8_t *>(line), size_t(length)) != size_t(length))
    ++droppedLines;
}
#endif
}
void Diagnostics::begin() {
#if EMG_SERIAL_DEBUG
  Serial.begin(Config::SERIAL_BAUD); // API void, no esperar conexion USB.
#endif
}
Diagnostics::Command Diagnostics::command() {
#if EMG_SERIAL_DEBUG
  if (Serial.available() > 0) {
    const int byte = Serial.read(); // int conserva -1 (lectura sin dato).
    if (byte < 0) { ++droppedLines; return Command::None; }
    switch (byte) {
      case 'c': case 'C': return Command::Calibrate;
      case 'p': case 'P': return Command::TogglePlot;
      case 'x': case 'X': return Command::Open;
      case '\n': case '\r': break;
      default: ++droppedLines; break;
    }
  }
#endif
  return Command::None;
}
void Diagnostics::event(const char *name, uint32_t now, uint32_t value) {
#if EMG_SERIAL_DEBUG
  char line[Config::SERIAL_LINE_CAPACITY];
  const int length = snprintf(line, sizeof(line), "E,%lu,%s,%lu\n",
      (unsigned long)now, name, (unsigned long)value);
  send(line, length, sizeof(line));
#else
  (void)name; (void)now; (void)value;
#endif
}
void Diagnostics::plot(uint32_t now, const Sample &sample, const Hand::Thresholds &thresholds) {
#if EMG_SERIAL_DEBUG
  if (!plotEnabled || uint32_t(now - lastPlotMs) < Config::PLOT_PERIOD_MS) return;
  lastPlotMs = now;
  // El buffer del core UNO permite 63 bytes utiles; la linea mas larga se
  // descarta entera. No se hacen escrituras parciales ni esperas por UART.
  char line[Config::SERIAL_LINE_CAPACITY];
  const int length = snprintf(line, sizeof(line), "D,%lu,%u,%u,%u,%u,%u,%u,%u,%lu\n",
      (unsigned long)now, sample.raw, sample.baseline, sample.envelope,
      thresholds.open, thresholds.close, unsigned(sample.faults),
      unsigned(sample.overruns), (unsigned long)droppedLines);
  send(line, length, sizeof(line));
#else
  (void)now; (void)sample; (void)thresholds;
#endif
}
void Diagnostics::togglePlot() { plotEnabled = !plotEnabled; }
uint32_t Diagnostics::dropped() { return droppedLines; }
