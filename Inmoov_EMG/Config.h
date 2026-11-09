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
constexpr uint16_t OBSERVED_REST_CEILING = 8; // Referencia inicial de las capturas, no regla universal.
constexpr uint16_t OBSERVED_ACTIVE_FLOOR = 12;
constexpr uint16_t ACTIVITY_ATTACK_MS = 50;
constexpr uint16_t ACTIVITY_DECAY_MS = 180;
constexpr uint16_t ACTIVITY_CLOSE_LEVEL = 650; // Ponderacion 0..1000.
constexpr uint16_t ACTIVITY_OPEN_LEVEL = 200;
constexpr uint16_t REST_ARM_MS = 100;
constexpr uint16_t RELAX_CONFIRM_MS = 250;
constexpr uint16_t MIN_CLOSED_MS = 900; // Permite completar la rampa de 780..800 ms.
constexpr uint16_t MAX_CONTROL_GAP_MS = 20;
constexpr uint16_t LED_BLINK_MS = 250;
constexpr uint16_t PLOT_PERIOD_MS = 100;
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint16_t SAMPLE_HZ = 1000;
constexpr uint8_t FIRMWARE_VERSION = 6;
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
constexpr uint16_t REST_NOISE_MARGIN = 2;
constexpr uint16_t CALIBRATION_SIGNATURE = 0xE706; // Cadena filtrada modular: recalibrar.

static_assert(ADC_REFERENCE_MV >= SENSOR_SUPPLY_MV, "Referencia ADC insuficiente");
static_assert(SENSOR_MAX_ADC > ADC_HIGH_CLIP_MARGIN + ADC_LOW_CLIP &&
              SENSOR_MAX_ADC <= ADC_FULL_SCALE, "Rango ADC invalido");

// Calibrar con los tensores sueltos. 1000/2000 us son pulsos, NO grados.
// Se permite invertir individualmente open/closed si un servo gira al reves.
constexpr uint16_t OPEN_US[SERVO_COUNT] = {1000, 1000, 1000, 1000, 1000};
constexpr uint16_t CLOSED_US[SERVO_COUNT] = {2000, 2000, 2000, 2000, 2000};
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
