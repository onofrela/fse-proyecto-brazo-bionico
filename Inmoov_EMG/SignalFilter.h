#ifndef INMOOV_SIGNAL_FILTER_H
#define INMOOV_SIGNAL_FILTER_H
#include <stdint.h>
#include "Config.h"

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
