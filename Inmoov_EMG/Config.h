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
constexpr uint16_t MAX_CONTROL_GAP_MS = 20;
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
constexpr int EEPROM_REQUIRED_BYTES = 8;
constexpr uint32_t INVALID_BOOT_COUNT = UINT32_MAX;
// Asignacion de pines de los perifericos del proyecto.
constexpr uint8_t SERVO_PINS[SERVO_COUNT] = {3, 5, 6, 10, 11};
constexpr uint8_t BUTTON_PIN = 8;
constexpr uint8_t MODE_PIN = 9; // GND = manual; abierto = EMG.
// Salidas del AD8232, activas en alto. INPUT sin pull-up interno de 5 V.
constexpr uint8_t LEADS_OFF_PLUS_PIN = 15;  // A1 en UNO.
constexpr uint8_t LEADS_OFF_MINUS_PIN = 16; // A2 en UNO.

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
