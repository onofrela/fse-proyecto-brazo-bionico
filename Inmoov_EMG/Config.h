#ifndef INMOOV_CONFIG_H
#define INMOOV_CONFIG_H

#include <stdint.h>

namespace Config {
// Asignacion de pines de los perifericos del proyecto.
constexpr uint8_t SERVO_PINS[5] = {3, 5, 6, 10, 11};
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
    (uint32_t(SENSOR_SUPPLY_MV) * 1023 + ADC_REFERENCE_MV / 2) / ADC_REFERENCE_MV;
constexpr uint16_t ADC_LOW_CLIP = 5;
constexpr uint16_t ADC_HIGH_CLIP = SENSOR_MAX_ADC - 6; // 669 con AVcc=5 V.
constexpr uint16_t SIGNAL_SETTLE_SAMPLES = 2000; // 2 s continuos sin falla.
constexpr uint16_t MIN_CALIBRATION_RANGE = 12; // Envolvente, cuentas ADC.
constexpr uint16_t REST_NOISE_MARGIN = 2;
constexpr uint16_t CALIBRATION_SIGNATURE = 0xE702; // Invalida formato anterior.

static_assert(ADC_REFERENCE_MV >= SENSOR_SUPPLY_MV, "Referencia ADC insuficiente");
static_assert(SENSOR_MAX_ADC > 12 && SENSOR_MAX_ADC <= 1023, "Rango ADC invalido");

// Calibrar con los tensores sueltos. 1000/2000 us son pulsos, NO grados.
// Se permite invertir individualmente open/closed si un servo gira al reves.
constexpr uint16_t OPEN_US[5] = {1000, 1000, 1000, 1000, 1000};
constexpr uint16_t CLOSED_US[5] = {2000, 2000, 2000, 2000, 2000};
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
