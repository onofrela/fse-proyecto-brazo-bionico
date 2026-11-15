#include <Arduino.h>
#include <stdio.h>
#include "Diagnostics.h"
namespace {
uint32_t droppedLines = 0;
bool plotEnabled = false;
uint8_t pendingStatus = 0;
#if EMG_SERIAL_DEBUG
uint8_t auxiliary = 0;
#endif
#if EMG_SERIAL_DEBUG
uint32_t lastPlotMs = 0;
uint32_t lastActivityMs = 0;
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
      case 'c': case 'C': return Command::Resume;
      case 'p': case 'P': return Command::TogglePlot;
      case 'x': case 'X': return Command::Open;
      case '?': return Command::Status;
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
void Diagnostics::requestStatus() { pendingStatus = 1; }
void Diagnostics::update(uint32_t now, const ControlStatus &status) {
#if EMG_SERIAL_DEBUG
  // Solo auxiliar cuando TX esta vacio y lejos del proximo D; un evento/vuelta.
  if (Serial.availableForWrite() < 63 ||
      (plotEnabled && uint32_t(now-lastPlotMs) >= Config::PLOT_PERIOD_MS-20)) return;
  if (pendingStatus) {
    switch(pendingStatus++) {
      case 1: event("FW",now,Config::FIRMWARE_VERSION); break;
      case 2: event("HP_HZ",now,Config::FILTER_HIGH_PASS_HZ); break;
      case 3: event("NOTCH_HZ",now,Config::FILTER_NOTCH_HZ); break;
      case 4: event("FIXED_CONTROL",now,1); break;
      case 5: event("HAND_ONLY",now,1); break;
      case 6: event("AVERAGE_SAMPLES",now,Config::LEVEL_AVERAGE_SAMPLES); break;
      case 7: event("LEVEL_MID",now,Config::LEVEL_MID_START); break;
      case 8: event("LEVEL_HIGH",now,Config::LEVEL_HIGH_START); break;
      case 9: event("CONTROL_PAUSED",now,status.paused); break;
      case 10: event("TARGET_STATUS",now,status.targetAngle); break;
      case 11: {
        uint8_t reversed = 0;
        for (uint8_t n=0; n<Config::SERVO_COUNT; ++n)
          if (Config::OPEN_US[n] > Config::CLOSED_US[n]) reversed |= uint8_t(1U << n);
        event("SERVO_REVERSE_MASK",now,reversed);
        pendingStatus=0; break;
      }
      default: pendingStatus=0; break;
    }
  } else if (plotEnabled && uint32_t(now-lastActivityMs)>=200) {
    lastActivityMs=now;
    if (auxiliary==0) event("MEAN_X10",now,status.meanX10);
    else if (auxiliary==1) event("TARGET_STATUS",now,status.targetAngle);
    else event("CONTROL_PAUSED",now,status.paused);
    auxiliary=(auxiliary+1)%3;
  }
#else
  (void)now; (void)status;
#endif
}
