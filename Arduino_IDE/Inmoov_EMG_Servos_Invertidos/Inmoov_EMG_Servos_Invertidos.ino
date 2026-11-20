/*
 * Mano InMoov - version autocontenida para Arduino IDE / Arduino Uno R3.
 * Medio (D6), anular (D10) y menique (D11): 0 -> 180, 90 -> 90, 180 -> 0.
 * Pulgar (D3) e indice (D5) conservan su direccion.
 * Abrir este archivo y seleccionar Arduino Uno y su puerto.
 * Bibliotecas: Servo; EEPROM incluida en Arduino AVR Boards.
 * Generado con python scripts/export_ino.py desde los modulos .h/.cpp de Inmoov_EMG/.
 * Toda la configuracion esta incluida abajo; no requiere archivos del proyecto.
 * Monitor Serie a 115200: enviar ? y comprobar SERVO_REVERSE_MASK,28.
 * Los .cpp de esta carpeta se reservan para pruebas nativas, evitando duplicados.
 */
#include <Arduino.h>

// ===== Config.h =====
#ifndef INMOOV_CONFIG_H
#define INMOOV_CONFIG_H

#include <stdint.h>

namespace Config {
// Decision D01 (docs/bitacora.md): memoria estatica y tareas sin RTOS.
constexpr uint8_t SERVO_COUNT = 5;
constexpr uint16_t ADC_FULL_SCALE = 1023;
constexpr uint8_t FILTER_WINDOW_SAMPLES = 32;
constexpr uint16_t BASELINE_Q_SCALE = 256;
constexpr uint16_t BASELINE_TIME_SAMPLES = 256;
constexpr uint16_t POSITION_FULL_SCALE = 1000;
constexpr uint16_t MOTION_STEP = 25;
constexpr uint16_t MOTION_PERIOD_MS = 20;
constexpr uint16_t UI_PERIOD_MS = 5;
constexpr uint16_t DEBOUNCE_MS = 10;
constexpr uint16_t CAL_PREPARE_MS = 3000;
constexpr uint16_t CAL_REST_MS = 6000;
constexpr uint16_t CAL_ACTIVE_MS = 10000;
constexpr uint16_t CAL_SAMPLE_MS = 20;
constexpr uint16_t ACTIVITY_ATTACK_MS = 120; // Un pico aislado de 100 ms no ordena cierre.
constexpr uint16_t ACTIVITY_DECAY_MS = 180;
constexpr uint16_t ACTIVITY_CLOSE_LEVEL = 650; // Ponderacion 0..1000.
constexpr uint16_t ACTIVITY_OPEN_LEVEL = 200;
constexpr uint16_t REST_ARM_MS = 100;
constexpr uint16_t RELAX_CONFIRM_MS = 250;
constexpr uint16_t MIN_CLOSED_MS = 900; // Permite completar la rampa de 780..800 ms.
constexpr uint16_t MIN_OPEN_MS = 900; // Completar tambien la apertura antes del siguiente cierre.
constexpr uint16_t MAX_CONTROL_GAP_MS = 20;
constexpr uint16_t LED_BLINK_MS = 250;
constexpr uint16_t PLOT_PERIOD_MS = 100;
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint16_t SAMPLE_HZ = 1000;
constexpr uint8_t FIRMWARE_VERSION = 8;
// Promedio por bloques de envolvente: 200 muestras a 1 kHz (~200 ms).
constexpr uint16_t LEVEL_AVERAGE_SAMPLES = 200;
constexpr uint16_t LEVEL_MID_START = 5;
constexpr uint16_t LEVEL_HIGH_START = 10;
static_assert(LEVEL_AVERAGE_SAMPLES > 0 && LEVEL_AVERAGE_SAMPLES <= 1000, "Bloque invalido");
static_assert(LEVEL_MID_START < LEVEL_HIGH_START, "Bandas invalidas");
constexpr uint16_t FILTER_HIGH_PASS_HZ = 30;
constexpr uint16_t FILTER_NOTCH_HZ = 60;
constexpr uint16_t FILTER_LOW_PASS_HZ = 200;
constexpr uint16_t TIMER2_PRESCALER = 64;
constexpr uint8_t TIMER2_COMPARE = 249;
constexpr uint16_t ADC_START_TIMEOUT_US = 1000;
constexpr uint16_t SAMPLE_TIMEOUT_MS = 10;
constexpr uint8_t SAMPLE_QUEUE_CAPACITY = 32; // 31 entradas utiles (SPSC).
constexpr uint8_t SAMPLE_PROBE_PIN = 2;
constexpr uint16_t SERVO_MIN_US = 544; // Limites aceptados por Servo 1.2.2.
constexpr uint16_t SERVO_MAX_US = 2400;
constexpr uint16_t SERIAL_LINE_CAPACITY = 96;
constexpr uint8_t FAULT_LEADS_OFF = 1;
constexpr uint8_t FAULT_CLIPPED = 2;
constexpr uint8_t FAULT_NOT_READY = 4;
constexpr uint8_t FAULT_STALE = 8;
constexpr uint8_t FAULT_DATA_LOSS = 16;
constexpr uint8_t FAULT_HARDWARE = 32;
constexpr int EEPROM_BOOT_ADDRESS = 0;
constexpr int EEPROM_BOOT_INVERSE_ADDRESS = 4;
constexpr int EEPROM_CAL_ADDRESS = 16;
constexpr int EEPROM_OPEN_ADDRESS = 18;
constexpr int EEPROM_CLOSE_ADDRESS = 20;
constexpr int EEPROM_CHECKSUM_ADDRESS = 22;
constexpr int EEPROM_REQUIRED_BYTES = 24;
constexpr uint16_t CALIBRATION_CHECK_SALT = 0xA57C;
constexpr uint16_t INVALID_SIGNATURE = 0;
constexpr uint32_t INVALID_BOOT_COUNT = UINT32_MAX;
// Asignacion de pines de los perifericos del proyecto.
constexpr uint8_t SERVO_PINS[SERVO_COUNT] = {3, 5, 6, 10, 11};
constexpr uint8_t BUTTON_PIN = 8;
constexpr uint8_t MODE_PIN = 9; // GND = manual; abierto = EMG.
// Salidas del AD8232, activas en alto. INPUT sin pull-up interno de 5 V.
constexpr uint8_t LEADS_OFF_PLUS_PIN = 15;  // A1 en UNO.
constexpr uint8_t LEADS_OFF_MINUS_PIN = 16; // A2 en UNO.
constexpr uint8_t LED_RED = 7;
constexpr uint8_t LED_GREEN = 12;
constexpr uint8_t LED_BLUE = 13;
constexpr bool RGB_COMMON_ANODE = false;

// OUTPUT del AD8232: 0..3.3 V; el ADC conserva AVcc como referencia.
// Ajustar ADC_REFERENCE_MV si se mide una AVcc diferente en el montaje.
constexpr uint16_t ADC_REFERENCE_MV = 5000;
constexpr uint16_t SENSOR_SUPPLY_MV = 3300;
constexpr uint16_t SENSOR_MAX_ADC =
    (uint32_t(SENSOR_SUPPLY_MV) * ADC_FULL_SCALE + ADC_REFERENCE_MV / 2) / ADC_REFERENCE_MV;
constexpr uint16_t ADC_LOW_CLIP = 5;
constexpr uint16_t ADC_HIGH_CLIP_MARGIN = 6;
constexpr uint16_t ADC_HIGH_CLIP = SENSOR_MAX_ADC - ADC_HIGH_CLIP_MARGIN; // 669 con AVcc=5 V.
constexpr uint16_t SIGNAL_SETTLE_SAMPLES = 2000; // 2 s continuos sin falla.
constexpr uint16_t MIN_CALIBRATION_RANGE = 4; // Envolvente, cuentas ADC.
constexpr uint16_t MIN_THRESHOLD_GAP = 2; // Histeresis minima, incluso con actividad de una cifra.
constexpr uint16_t CALIBRATION_SIGNATURE = 0xE707; // Umbrales adaptativos sin pisos fijos: recalibrar.

static_assert(ADC_REFERENCE_MV >= SENSOR_SUPPLY_MV, "Referencia ADC insuficiente");
static_assert(SENSOR_MAX_ADC > ADC_HIGH_CLIP_MARGIN + ADC_LOW_CLIP &&
              SENSOR_MAX_ADC <= ADC_FULL_SCALE, "Rango ADC invalido");

// Extremos equivalentes a Servo.write(0) y Servo.write(180) con attach por defecto.
// Son extremos ordenados, no mediciones de angulo fisico. Ajustar con tensores sueltos.
// Orden: pulgar, indice, medio, anular, menique. Los tres ultimos giran invertidos.
constexpr uint16_t OPEN_US[SERVO_COUNT] = {544, 544, 2400, 2400, 2400};
constexpr uint16_t CLOSED_US[SERVO_COUNT] = {2400, 2400, 544, 544, 544};
static_assert(FILTER_WINDOW_SAMPLES > 0 && FILTER_WINDOW_SAMPLES <= 64,
              "Ventana estatica fuera del presupuesto previsto");
static_assert(MOTION_STEP > 0 && MOTION_STEP <= POSITION_FULL_SCALE, "Paso invalido");
static_assert(SAMPLE_QUEUE_CAPACITY > 1, "Cola insuficiente");
}

// D2 conmuta en cada ISR ADC: 1000 flancos/s, onda cuadrada de 500 Hz.
// Desactivar si D2 se utiliza para otro componente.
#ifndef EMG_SAMPLE_PROBE
#define EMG_SAMPLE_PROBE 1
#endif

// UART de diagnostico, nunca es una entrada Bluetooth.
#ifndef EMG_SERIAL_DEBUG
#define EMG_SERIAL_DEBUG 1
#endif
#endif

// ===== SampleQueue.h =====
#ifndef INMOOV_SAMPLE_QUEUE_H
#define INMOOV_SAMPLE_QUEUE_H
#include <stdint.h>

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

// ===== Acquisition.h =====
#ifndef INMOOV_ACQUISITION_H
#define INMOOV_ACQUISITION_H

namespace Acquisition {
bool begin();
bool take(Hand::RawSample &sample);
bool takeDataLoss(); // Lee y limpia la alarma de forma atomica.
void discardPending();
uint16_t overruns();
}
#endif

// ===== SignalFilter.h =====
#ifndef INMOOV_SIGNAL_FILTER_H
#define INMOOV_SIGNAL_FILTER_H
#include <stdint.h>

namespace Hand {
// DF1: senal Q6, coeficientes Q12, estados limitados a +/-1023 cuentas.
// Max sum(abs(coef))=27228; 65472*27228 < 1.79e9 < INT32_MAX.
// Sin float, int64 ni asignacion dinamica; se ejecuta en el consumidor de muestras.
static_assert(Config::SAMPLE_HZ == 1000, "Los coeficientes requieren muestreo a 1 kHz");
class Biquad {
 public:
  void clear() { x1_ = x2_ = y1_ = y2_ = 0; }
  int32_t add(int32_t x, int16_t b0, int16_t b1, int16_t b2,
              int16_t a1, int16_t a2) {
    if (x > 65472) x = 65472;
    if (x < -65472) x = -65472;
    const int32_t acc = int32_t(b0)*x + int32_t(b1)*x1_ + int32_t(b2)*x2_
                      - int32_t(a1)*y1_ - int32_t(a2)*y2_;
    // Redondeo simetrico; evita el sesgo DC de desplazar negativos sin redondear.
    int32_t y = acc >= 0 ? (acc + 2048) / 4096 : -((-acc + 2048) / 4096);
    if (y > 65472) y = 65472;
    if (y < -65472) y = -65472;
    x2_ = x1_; x1_ = x; y2_ = y1_; y1_ = y;
    return y;
  }
 private:
  int32_t x1_ = 0, x2_ = 0, y1_ = 0, y2_ = 0;
};

class ActivityFilter {
 public:
  void clear() { high_.clear(); notch_.clear(); low_.clear(); }
  int32_t addQ6(int32_t value) {
    // scipy.signal.butter(2,30,'high',fs=1000), iirnotch(60,20,fs=1000),
    // butter(2,200,'low',fs=1000); round(coeff*4096).
    // b1=-2*b0 conserva exactamente el cero en DC del pasa-altas.
    value = high_.add(value, 3585, -7170, 3585, -7105, 3138);
    value = notch_.add(value, 4058, -7546, 4058, -7546, 4020);
    return low_.add(value, 846, 1692, 846, -1514, 802);
  }
 private:
  Biquad high_, notch_, low_;
};
} // namespace Hand
#endif

// ===== Signal.h =====
#ifndef INMOOV_SIGNAL_H
#define INMOOV_SIGNAL_H

#include <stdint.h>


namespace Hand {

// 32 muestras a 1 kHz: promedio de los ultimos 32 ms.
class MovingAverage {
 public:
  void clear() {
    for (uint8_t i = 0; i < Config::FILTER_WINDOW_SAMPLES; ++i) samples_[i] = 0;
    sum_ = 0;
    index_ = 0;
    count_ = 0;
  }
  uint16_t add(uint16_t value) {
    sum_ -= samples_[index_];
    samples_[index_] = value;
    sum_ += value;
    index_ = (index_ + 1) % Config::FILTER_WINDOW_SAMPLES;
    if (count_ < Config::FILTER_WINDOW_SAMPLES) ++count_;
    return uint16_t(sum_ / Config::FILTER_WINDOW_SAMPLES);
  }
  bool ready() const { return count_ == Config::FILTER_WINDOW_SAMPLES; }

 private:
  uint16_t samples_[Config::FILTER_WINDOW_SAMPLES] = {};
  uint32_t sum_ = 0; // La ventana conserva magnitudes Q6 antes de redondear.
  uint8_t index_ = 0;
  uint8_t count_ = 0;
};

// Consumidor a 1 kHz: DC -> SignalFilter -> rectificacion -> media 32 ms.
// Adquisicion/ISR permanecen separadas. No clasifica dedos ni garantiza quitar ECG.
class Ad8232Signal {
 public:
  // Un hueco de adquisicion invalida tanto la ventana como el tiempo estable.
  void reset() {
    window_.clear();
    filter_.clear();
    baselineQ8_ = 0;
    stableSamples_ = 0;
    envelope_ = 0;
    initialized_ = false;
    clipped_ = false;
  }
  uint16_t add(uint16_t raw, bool leadsOff) {
    const bool clip = raw <= Config::ADC_LOW_CLIP || raw >= Config::ADC_HIGH_CLIP;
    if (leadsOff || clip) {
      if (initialized_) {
        window_.clear();
        filter_.clear();
        initialized_ = false;
      }
      stableSamples_ = 0;
      envelope_ = 0;
      clipped_ = clip;
      return 0;
    }
    clipped_ = false;
    if (!initialized_) {
      baselineQ8_ = int32_t(raw) * Config::BASELINE_Q_SCALE;
      initialized_ = true;
    }
    baselineQ8_ += (int32_t(raw) * Config::BASELINE_Q_SCALE - baselineQ8_) / Config::BASELINE_TIME_SAMPLES;
    const int32_t deviation = int32_t(raw) * Config::BASELINE_Q_SCALE - baselineQ8_;
    const int32_t filtered = filter_.addQ6(deviation / (Config::BASELINE_Q_SCALE / 64));
    const uint16_t magnitude = uint16_t(filtered < 0 ? -filtered : filtered);
    envelope_ = window_.add(magnitude) / 64;
    if (stableSamples_ < Config::SIGNAL_SETTLE_SAMPLES) ++stableSamples_;
    return envelope_;
  }
  bool ready() const {
    return initialized_ && window_.ready() &&
           stableSamples_ >= Config::SIGNAL_SETTLE_SAMPLES;
  }
  bool clipped() const { return clipped_; }
  uint16_t baseline() const { return initialized_ ? uint16_t(baselineQ8_ / Config::BASELINE_Q_SCALE) : 0; }

 private:
  MovingAverage window_;
  ActivityFilter filter_;
  int32_t baselineQ8_ = 0;
  uint16_t stableSamples_ = 0;
  uint16_t envelope_ = 0;
  bool initialized_ = false;
  bool clipped_ = false;
};

} // namespace Hand
#endif

// ===== Motion.h =====
#ifndef INMOOV_MOTION_H
#define INMOOV_MOTION_H

#include <stdint.h>

namespace Hand {

// Todos los dedos avanzan a la vez. Un recorrido dura 800 ms nominales.
class Motion {
 public:
  bool request(bool closed, uint32_t now) {
    return requestPosition(closed ? Config::POSITION_FULL_SCALE : 0, now);
  }
  bool requestPosition(uint16_t target, uint32_t now) {
    if (target > Config::POSITION_FULL_SCALE) return false;
    if (target == target_) return false;
    target_ = target;
    last_ = now;
    return step(); // Primer cambio de pulso sin esperar otro periodo de 20 ms.
  }
  bool update(uint32_t now) {
    if (uint32_t(now - last_) < Config::MOTION_PERIOD_MS) return false;
    last_ = now;
    return step();
  }
  uint16_t target() const { return target_; }
  bool closed() const { return target_ == Config::POSITION_FULL_SCALE; }
  uint16_t position() const { return position_; }
  bool isOpen() const { return position_ == 0 && target_ == 0; }

 private:
  bool step() {
    if (position_ == target_) return false;
    if (position_ < target_) {
      position_ += (target_ - position_ < Config::MOTION_STEP) ? target_ - position_ : Config::MOTION_STEP;
    } else {
      position_ -= (position_ - target_ < Config::MOTION_STEP) ? position_ - target_ : Config::MOTION_STEP;
    }
    return true;
  }
  uint16_t position_ = 0;
  uint16_t target_ = 0;
  uint32_t last_ = 0;
};

inline uint16_t pulse(uint16_t position, uint16_t open, uint16_t closed) {
  return int32_t(open) + (int32_t(closed) - open) * position / Config::POSITION_FULL_SCALE;
}

} // namespace Hand
#endif

// ===== Inputs.h =====
#ifndef INMOOV_INPUTS_H
#define INMOOV_INPUTS_H

#include <stdint.h>

namespace Hand {

class DebouncedInput {
 public:
  void begin(bool value, uint32_t now) {
    stable_ = candidate_ = value;
    since_ = now;
  }
  bool update(bool value, uint32_t now) {
    if (value != candidate_) {
      candidate_ = value;
      since_ = now;
    }
    if (candidate_ != stable_ && uint32_t(now - since_) >= Config::DEBOUNCE_MS) {
      stable_ = candidate_;
      return true;
    }
    return false;
  }
  bool value() const { return stable_; }

 private:
  bool stable_ = false;
  bool candidate_ = false;
  uint32_t since_ = 0;
};

} // namespace Hand
#endif

// ===== Decision.h =====
#ifndef INMOOV_DECISION_H
#define INMOOV_DECISION_H

#include <stdint.h>

namespace Hand {

struct Thresholds {
  uint16_t open;
  uint16_t close;
  bool valid() const { return open > 0 && close > open && close < Config::ADC_HIGH_CLIP; }
};

// Control activo FW=8: clasificar el promedio, nunca una lectura aislada.
// No hay buffer dinamico: suma de hasta 200 muestras completas a 1 kHz.
class ThreeLevelControl {
 public:
  void reset() {
    sum_ = 0; count_ = position_ = meanX10_ = 0;
    initialized_ = ready_ = false;
  }
  uint16_t add(uint16_t value, uint32_t now) {
    if (initialized_ && uint32_t(now-last_) > Config::MAX_CONTROL_GAP_MS) reset();
    last_ = now; initialized_ = true;
    sum_ += value;
    if (++count_ == Config::LEVEL_AVERAGE_SAMPLES) {
      // Comparar sumas evita que redondear 4.999 o 9.999 cambie la banda.
      position_ = sum_ < uint32_t(count_)*Config::LEVEL_MID_START ? 0 :
                  sum_ < uint32_t(count_)*Config::LEVEL_HIGH_START ?
                  Config::POSITION_FULL_SCALE/2 : Config::POSITION_FULL_SCALE;
      meanX10_ = uint16_t(sum_*10/count_);
      sum_ = 0; count_ = 0; ready_ = true;
    }
    return position_;
  }
  bool ready() const { return ready_; }
  uint16_t meanX10() const { return meanX10_; }
  uint16_t position() const { return position_; }
 private:
  uint32_t sum_ = 0, last_ = 0;
  uint16_t count_ = 0, position_ = 0, meanX10_ = 0;
  bool initialized_ = false, ready_ = false;
};

// Algoritmo anterior, conservado para las pruebas historicas; no usado por FW=8.
// Se exige reposo al entrar al modo EMG o recuperarse de una falla.
class MuscleSwitch {
 public:
  void reset() {
    armed_ = closed_ = initialized_ = lowTracking_ = openingHold_ = false;
    activityQ8_ = 0;
  }
  uint16_t activity() const { return uint16_t(activityQ8_ / 256); }
  bool update(uint16_t value, const Thresholds &t, uint32_t now) {
    if (!t.valid()) { reset(); return false; }
    if (!initialized_) { initialized_ = true; last_ = now; return false; }
    const uint32_t dt = now - last_;
    if (!dt) return closed_;
    last_ = now;
    if (dt > Config::MAX_CONTROL_GAP_MS) {
      reset(); initialized_ = true; last_ = now; return false;
    }
    // Peso normalizado: <=abrir ->0; >=cerrar ->1000; entre ambos, lineal.
    const int32_t target = value <= t.open ? 0 : value >= t.close ? 256000L :
        int32_t(value - t.open) * 256000L / (t.close - t.open);
    const uint16_t tau = target > activityQ8_ ? Config::ACTIVITY_ATTACK_MS : Config::ACTIVITY_DECAY_MS;
    activityQ8_ += (target - activityQ8_) * int32_t(dt) / (int32_t(tau) + int32_t(dt));
    const bool low = value <= t.open && activity() <= Config::ACTIVITY_OPEN_LEVEL;
    if (low && !lowTracking_) { lowTracking_ = true; lowSince_ = now; }
    if (!low) lowTracking_ = false;
    if (!closed_) {
      if (lowTracking_ && uint32_t(now-lowSince_) >= Config::REST_ARM_MS) armed_ = true;
      if (openingHold_ && uint32_t(now-openSince_) >= Config::MIN_OPEN_MS) openingHold_ = false;
      if (armed_ && !openingHold_ && activity() >= Config::ACTIVITY_CLOSE_LEVEL) {
        closed_ = true; closedSince_ = now; lowTracking_ = false;
      }
    } else if (lowTracking_ && uint32_t(now-lowSince_) >= Config::RELAX_CONFIRM_MS &&
               uint32_t(now-closedSince_) >= Config::MIN_CLOSED_MS) {
      closed_ = false;
      openingHold_ = true;
      openSince_ = now;
    }
    return closed_;
  }

 private:
  bool armed_ = false;
  bool closed_ = false;
  bool initialized_ = false, lowTracking_ = false, openingHold_ = false;
  int32_t activityQ8_ = 0;
  uint32_t last_ = 0, lowSince_ = 0, closedSince_ = 0, openSince_ = 0;
};

} // namespace Hand
#endif

// ===== Diagnostics.h =====
#ifndef INMOOV_DIAGNOSTICS_H
#define INMOOV_DIAGNOSTICS_H
#include <stdint.h>

namespace Diagnostics {
struct Sample {
  uint16_t raw = 0;
  uint16_t baseline = 0;
  uint16_t envelope = 0;
  uint16_t overruns = 0;
  uint8_t faults = Config::FAULT_NOT_READY;
};
struct ControlStatus {
  bool paused;
  uint16_t meanX10, targetAngle;
};
enum class Command : uint8_t { None, Resume, TogglePlot, Open, Status };
void begin();
Command command();
void event(const char *name, uint32_t now, uint32_t value);
void plot(uint32_t now, const Sample &sample, const Hand::Thresholds &thresholds);
void togglePlot();
uint32_t dropped();
void requestStatus();
void update(uint32_t now, const ControlStatus &status);
}
#endif

// ===== Application.h =====
#ifndef INMOOV_APPLICATION_H
#define INMOOV_APPLICATION_H





namespace Hand {
// Coordinador: adquisicion, decision por promedio y rampa tienen duenos separados.
class Application {
 public:
  void begin(uint32_t now);
  void tick(uint32_t now);
  struct Status {
    bool emgMode, healthy, outputFault, storageFault, closing, paused;
    uint16_t position, target, meanX10;
  };
  Status status() const;
 private:
  void requestPosition(uint16_t target, uint32_t now);
  void updateUi(uint32_t now);
  void updateLed(uint32_t now);
  void consume(const RawSample &raw, uint32_t now);
  void signalLost(uint32_t now);
  void reportSignal(uint32_t now);
  Ad8232Signal signal_;
  Motion motion_;
  ThreeLevelControl levels_;
  DebouncedInput button_, selector_;
  Diagnostics::Sample sample_;
  uint32_t lastSampleMs_ = 0, lastUiMs_ = 0;
  bool haveSample_ = false, healthy_ = false, reportedHealthy_ = false;
  bool emgMode_ = false, paused_ = false;
  bool outputFault_ = false, acquisitionFault_ = false, storageFault_ = false;
  bool dataLoss_ = false;
};
}
#endif

// ===== Outputs.h =====
#ifndef INMOOV_OUTPUTS_H
#define INMOOV_OUTPUTS_H
#include <stdint.h>
namespace Outputs {
bool begin();
bool setPosition(uint16_t position);
void led(bool red, bool green, bool blue);
bool emgSelected();
bool buttonPressed();
}
#endif

// ===== CalibrationStore.h =====
#ifndef INMOOV_CALIBRATION_STORE_H
#define INMOOV_CALIBRATION_STORE_H

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

// ===== Storage.h =====
#ifndef INMOOV_STORAGE_H
#define INMOOV_STORAGE_H

namespace Storage {
struct Settings {
  Hand::Thresholds thresholds;
  uint32_t bootCount;
  bool calibrated;
  bool bootVerified;
};
Settings load();
bool save(const Hand::Thresholds &thresholds);
Hand::SaveResult poll();
}
#endif

// ===== Acquisition.cpp =====
// Arduino IDE usa las definiciones integradas en Inmoov_EMG.ino.
#include <Arduino.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#if !defined(__AVR_ATmega328P__) || F_CPU != 16000000UL
#error "Este firmware requiere ATmega328P a 16 MHz (Arduino UNO R3)."
#endif
static_assert(sizeof(Hand::RawSample) == 7, "Revisar presupuesto de SRAM de la cola");
static_assert(F_CPU / Config::TIMER2_PRESCALER /
              (Config::TIMER2_COMPARE + 1UL) == Config::SAMPLE_HZ,
              "Timer2 no coincide con la frecuencia de muestreo");
namespace {
Hand::SampleQueue<Config::SAMPLE_QUEUE_CAPACITY> queue;
volatile uint16_t lostSamples = 0;
volatile bool dataLoss = false;
void recordLoss() {
  if (lostSamples != UINT16_MAX) ++lostSamples; // Contador saturado, sin wrap.
  dataLoss = true;
}
}

// Prioridad de hardware: ISR breves, sin filtros, UART ni EEPROM.
// AVR no anida estas ISR; el orden de vectores resuelve eventos simultaneos.
ISR(TIMER2_COMPA_vect) {
  // ADIF pendiente tambien significa que aun no se ha recogido la conversion.
  if (ADCSRA & (_BV(ADSC) | _BV(ADIF))) recordLoss();
  else ADCSRA |= _BV(ADSC);
}
ISR(ADC_vect) {
  const uint16_t raw = ADC;
#if EMG_SAMPLE_PROBE
  PIND = _BV(PD2); // D2: 1 ms entre flancos; periodo completo de 2 ms.
#endif
  const bool leadsOff = digitalRead(Config::LEADS_OFF_PLUS_PIN) == HIGH ||
                        digitalRead(Config::LEADS_OFF_MINUS_PIN) == HIGH;
  const Hand::RawSample sample = {millis(), raw, leadsOff};
  if (!queue.push(sample)) recordLoss();
}

bool Acquisition::begin() {
  pinMode(Config::LEADS_OFF_PLUS_PIN, INPUT); // Sin pull-up de 5 V.
  pinMode(Config::LEADS_OFF_MINUS_PIN, INPUT);
#if EMG_SAMPLE_PROBE
  pinMode(Config::SAMPLE_PROBE_PIN, OUTPUT);
  digitalWrite(Config::SAMPLE_PROBE_PIN, LOW);
#endif
  ADMUX = _BV(REFS0); // AVcc, ADC0, alineacion derecha.
  ADCSRB = 0;
  DIDR0 |= _BV(ADC0D);
  const uint8_t adcConfiguration = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
  ADCSRA = adcConfiguration;
  ADCSRA |= _BV(ADSC);
  const uint32_t startedUs = micros();
  while (ADCSRA & _BV(ADSC)) {
    if (uint32_t(micros() - startedUs) >= Config::ADC_START_TIMEOUT_US) {
      ADCSRA = 0; // Fallo de arranque: no bloquear indefinidamente.
      return false;
    }
  }
  (void)ADC;
  if (ADMUX != _BV(REFS0) || (ADCSRA & adcConfiguration) != adcConfiguration) {
    ADCSRA = 0;
    return false;
  }
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    queue.clear();
    dataLoss = false;
    ADCSRA |= _BV(ADIF) | _BV(ADIE); // Limpiar bandera antes de habilitar Timer2.
    TCCR2A = _BV(WGM21);
    TCCR2B = 0;
    TCNT2 = 0;
    OCR2A = Config::TIMER2_COMPARE;
    TIFR2 = _BV(OCF2A);
    TIMSK2 = _BV(OCIE2A);
    TCCR2B = _BV(CS22);
  }
  const bool configured = TCCR2A == _BV(WGM21) && TCCR2B == _BV(CS22) &&
                          OCR2A == Config::TIMER2_COMPARE && TIMSK2 == _BV(OCIE2A);
  if (!configured) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { TIMSK2 = 0; TCCR2B = 0; ADCSRA = 0; }
  }
  return configured;
}
bool Acquisition::take(Hand::RawSample &sample) {
  bool present;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { present = queue.pop(sample); }
  return present;
}
bool Acquisition::takeDataLoss() {
  bool lost;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { lost = dataLoss; dataLoss = false; }
  return lost;
}
void Acquisition::discardPending() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { queue.clear(); }
}
uint16_t Acquisition::overruns() {
  uint16_t count;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { count = lostSamples; }
  return count;
}

// ===== Application.cpp =====
// Arduino IDE usa las definiciones integradas en Inmoov_EMG.ino.




namespace {
// La ISR puede publicar despues de capturar 'now'. No confundir con antiguedad.
bool sampleExpired(uint32_t now, uint32_t capturedMs) {
  return int32_t(now-capturedMs) > int32_t(Config::SAMPLE_TIMEOUT_MS);
}
}
void Hand::Application::begin(uint32_t now) {
  Diagnostics::begin();
  outputFault_ = !Outputs::begin();
  const Storage::Settings settings = Storage::load(); // Contador; umbrales previos ignorados.
  storageFault_ = !settings.bootVerified;
  emgMode_ = Outputs::emgSelected();
  selector_.begin(emgMode_,now);
  button_.begin(Outputs::buttonPressed(),now);
  acquisitionFault_ = !Acquisition::begin();
  Diagnostics::event("BOOT",now,settings.bootCount);
  Diagnostics::event("MODE",now,emgMode_);
  if (outputFault_) Diagnostics::event("SERVO_INIT_FAILED",now,0);
  if (acquisitionFault_) Diagnostics::event("ADC_INIT_FAILED",now,0);
  if (storageFault_) Diagnostics::event("EEPROM_BOOT_FAILED",now,0);
  Diagnostics::requestStatus();
  updateLed(now);
}
Hand::Application::Status Hand::Application::status() const {
  return {emgMode_,healthy_,outputFault_,storageFault_,motion_.closed(),paused_,
          motion_.position(),motion_.target(),levels_.meanX10()};
}
void Hand::Application::requestPosition(uint16_t target,uint32_t now) {
  if (outputFault_) return;
  const bool changed = target != motion_.target();
  if (motion_.requestPosition(target,now) && !Outputs::setPosition(motion_.position())) {
    outputFault_=true;
    Diagnostics::event("SERVO_WRITE_FAILED",now,0);
  }
  if (changed) Diagnostics::event("TARGET_DEG",now,uint32_t(target)*180/Config::POSITION_FULL_SCALE);
}
void Hand::Application::updateUi(uint32_t now) {
  if (uint32_t(now-lastUiMs_) < Config::UI_PERIOD_MS) return;
  lastUiMs_=now;
  const bool changed=selector_.update(Outputs::emgSelected(),now);
  const bool pressed=button_.update(Outputs::buttonPressed(),now);
  if (changed) {
    emgMode_=selector_.value(); levels_.reset(); requestPosition(0,now);
    Diagnostics::event("MODE",now,emgMode_);
  }
  if (pressed && button_.value() && !changed && !emgMode_)
    requestPosition(motion_.closed() ? 0 : Config::POSITION_FULL_SCALE,now);
}
void Hand::Application::signalLost(uint32_t now) {
  signal_.reset(); healthy_=false; sample_.envelope=0;
  sample_.faults |= Config::FAULT_NOT_READY;
  levels_.reset();
  if (emgMode_) requestPosition(0,now);
}
void Hand::Application::consume(const RawSample &raw,uint32_t now) {
  if (sampleExpired(now,raw.capturedMs)) { dataLoss_=true; signalLost(now); return; }
  if (haveSample_ && uint32_t(raw.capturedMs-lastSampleMs_) > Config::SAMPLE_TIMEOUT_MS) {
    dataLoss_=true; signalLost(now);
  }
  lastSampleMs_=raw.capturedMs; haveSample_=true;
  sample_.raw=raw.raw;
  sample_.envelope=signal_.add(raw.raw,raw.leadsOff);
  sample_.baseline=signal_.baseline();
  sample_.faults=(raw.leadsOff ? Config::FAULT_LEADS_OFF : 0) |
      (signal_.clipped() ? Config::FAULT_CLIPPED : 0) |
      (!signal_.ready() ? Config::FAULT_NOT_READY : 0);
  healthy_=sample_.faults==0 && !acquisitionFault_;
  if (healthy_) dataLoss_=false;
  if (!healthy_ || paused_) {
    levels_.reset(); if (emgMode_) requestPosition(0,now);
  } else if (emgMode_) {
    // Cada muestra una vez. Rampa con tiempo del bucle, promedio con captura real.
    requestPosition(levels_.add(sample_.envelope,raw.capturedMs),now);
  }
}
void Hand::Application::reportSignal(uint32_t now) {
  if (healthy_==reportedHealthy_) return;
  reportedHealthy_=healthy_;
  Diagnostics::event(healthy_ ? "SIGNAL_RECOVERED" : "SIGNAL_FAULT",now,sample_.raw);
}
void Hand::Application::updateLed(uint32_t now) {
  const bool blink=(now/Config::LED_BLINK_MS)%2==0;
  if (outputFault_ || acquisitionFault_ || storageFault_ || (emgMode_ && !healthy_))
    Outputs::led(blink,false,blink);
  else if (emgMode_ && paused_) Outputs::led(false,false,true);
  else Outputs::led(!emgMode_,emgMode_ || motion_.closed(),false);
}
void Hand::Application::tick(uint32_t now) {
  // Prioridad: seguridad/muestras -> entradas -> rampa/LED -> UART.
  if (Acquisition::takeDataLoss()) {
    Acquisition::discardPending(); dataLoss_=true; signalLost(now);
  }
  RawSample raw;
  for (uint8_t n=0;n<Config::SAMPLE_QUEUE_CAPACITY-1;++n) {
    if (!Acquisition::take(raw)) break;
    consume(raw,now);
  }
  if (!haveSample_ || sampleExpired(now,lastSampleMs_)) {
    signalLost(now); sample_.faults |= Config::FAULT_STALE | Config::FAULT_NOT_READY;
  }
  if (dataLoss_) sample_.faults |= Config::FAULT_DATA_LOSS;
  if (acquisitionFault_ || outputFault_) sample_.faults |= Config::FAULT_HARDWARE;
  updateUi(now);
  switch (Diagnostics::command()) {
    case Diagnostics::Command::Resume:
      paused_=false; levels_.reset(); Diagnostics::event("CONTROL_PAUSED",now,0); break;
    case Diagnostics::Command::Open:
      paused_=true; levels_.reset(); requestPosition(0,now);
      Diagnostics::event("CONTROL_PAUSED",now,1); break;
    case Diagnostics::Command::TogglePlot: Diagnostics::togglePlot(); break;
    case Diagnostics::Command::Status: Diagnostics::requestStatus(); break;
    case Diagnostics::Command::None: break;
  }
  if (!outputFault_ && motion_.update(now) && !Outputs::setPosition(motion_.position())) {
    outputFault_=true; Diagnostics::event("SERVO_WRITE_FAILED",now,0);
  }
  updateLed(now); reportSignal(now);
  sample_.overruns=Acquisition::overruns();
  Diagnostics::plot(now,sample_,{Config::LEVEL_MID_START,Config::LEVEL_HIGH_START});
  Diagnostics::update(now,{paused_,levels_.meanX10(),
      uint16_t(uint32_t(motion_.target())*180/Config::POSITION_FULL_SCALE)});
}

// ===== Diagnostics.cpp =====
// Arduino IDE usa las definiciones integradas en Inmoov_EMG.ino.
#include <Arduino.h>
#include <stdio.h>

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

// ===== Outputs.cpp =====
// Arduino IDE usa las definiciones integradas en Inmoov_EMG.ino.
#include <Arduino.h>
#include <Servo.h>



namespace {
Servo fingers[Config::SERVO_COUNT];
bool available = false;
void detachAll() {
  for (uint8_t finger = 0; finger < Config::SERVO_COUNT; ++finger)
    if (fingers[finger].attached()) fingers[finger].detach();
  available = false;
}
}
bool Outputs::begin() {
  pinMode(Config::BUTTON_PIN, INPUT_PULLUP);
  pinMode(Config::MODE_PIN, INPUT_PULLUP);
  pinMode(Config::LED_RED, OUTPUT);
  pinMode(Config::LED_GREEN, OUTPUT);
  pinMode(Config::LED_BLUE, OUTPUT);
  led(false, false, false);
  // Validar todos los limites antes de activar el primer motor.
  for (uint8_t finger = 0; finger < Config::SERVO_COUNT; ++finger) {
    if (Config::OPEN_US[finger] < Config::SERVO_MIN_US ||
        Config::OPEN_US[finger] > Config::SERVO_MAX_US ||
        Config::CLOSED_US[finger] < Config::SERVO_MIN_US ||
        Config::CLOSED_US[finger] > Config::SERVO_MAX_US) return false;
  }
  for (uint8_t finger = 0; finger < Config::SERVO_COUNT; ++finger) {
    // Precargar apertura evita enviar el pulso central por defecto al conectar.
    fingers[finger].writeMicroseconds(Config::OPEN_US[finger]);
    if (fingers[finger].attach(Config::SERVO_PINS[finger]) == INVALID_SERVO ||
        !fingers[finger].attached()) {
      detachAll(); // Deshacer arranque parcial si falta un canal.
      return false;
    }
    fingers[finger].writeMicroseconds(Config::OPEN_US[finger]);
  }
  available = true;
  return true;
}
bool Outputs::setPosition(uint16_t position) {
  if (!available || position > Config::POSITION_FULL_SCALE) return false;
  for (uint8_t finger = 0; finger < Config::SERVO_COUNT; ++finger) {
    if (!fingers[finger].attached()) { detachAll(); return false; }
    fingers[finger].writeMicroseconds(Hand::pulse(position,
        Config::OPEN_US[finger], Config::CLOSED_US[finger]));
  }
  // writeMicroseconds y GPIO no devuelven estado ni miden posicion fisica.
  return true;
}
void Outputs::led(bool red, bool green, bool blue) {
  digitalWrite(Config::LED_RED, red != Config::RGB_COMMON_ANODE);
  digitalWrite(Config::LED_GREEN, green != Config::RGB_COMMON_ANODE);
  digitalWrite(Config::LED_BLUE, blue != Config::RGB_COMMON_ANODE);
}
bool Outputs::emgSelected() { return digitalRead(Config::MODE_PIN) == HIGH; }
bool Outputs::buttonPressed() { return digitalRead(Config::BUTTON_PIN) == LOW; }

// ===== Storage.cpp =====
// Arduino IDE usa las definiciones integradas en Inmoov_EMG.ino.
#include <Arduino.h>
#include <EEPROM.h>
#include <avr/eeprom.h>

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

// ===== Entrada Arduino =====
namespace { Hand::Application application; }
void setup() { application.begin(millis()); }
void loop() { application.tick(millis()); }
