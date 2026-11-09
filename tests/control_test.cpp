#include <cassert>
#include <iostream>
#include "../Inmoov_EMG/Control.h"

using namespace Hand;

void testAverage() {
  MovingAverage f;
  for (int i = 0; i < 31; ++i) {
    f.add(100);
    assert(!f.ready());
  }
  assert(f.add(100) == 100 && f.ready());
  // Una muestra espuria no se convierte directamente en una orden de cierre.
  assert(f.add(1023) == 128);
  for (int i = 0; i < 32; ++i) f.add(1023);
  assert(f.add(1023) == 1023); // Sin overflow con ADC a fondo de escala.
  for (int i = 0; i < 32; ++i) f.add(0);
  assert(f.add(0) == 0);
}

void testMuscle() {
  const Thresholds t = {200, 300};
  MuscleSwitch m;
  assert(t.valid());
  assert(!m.update(500, t)); // Debe relajar antes del primer cierre.
  assert(!m.update(100, t));
  assert(!m.update(299, t));
  assert(m.update(300, t));
  assert(m.update(250, t)); // Histeresis: mantiene el estado entre umbrales.
  assert(!m.update(200, t));
  m.reset();
  assert(!m.update(500, t));
  assert(!(Thresholds{400, 200}).valid());
  assert(!(Thresholds{0, 300}).valid());
  assert(!(Thresholds{200, 1023}).valid());
  assert(!(Thresholds{200, Config::ADC_HIGH_CLIP}).valid());
}

void testMotion() {
  Motion m;
  assert(m.isOpen());
  assert(m.request(true, 100));
  assert(m.position() == 25); // Primera orden inmediata.
  assert(!m.request(true, 101)); // Orden repetida no acelera el movimiento.
  assert(!m.update(119));
  for (uint32_t now = 120; now <= 880; now += 20) m.update(now);
  assert(m.position() == 1000); // Recorrido completo <= 800 ms.
  assert(!m.update(900));
  assert(m.request(false, 901));
  assert(m.position() == 975);
  for (uint32_t now = 921; now <= 1681; now += 20) m.update(now);
  assert(m.isOpen());
  m.request(true, 1700);
  m.update(1720);
  assert(m.position() == 50);
  m.request(false, 1721); // Invertir a mitad de un cierre no salta al extremo.
  assert(m.position() == 25);
  m.update(1741);
  assert(m.isOpen());
  assert(pulse(0, 1000, 2000) == 1000);
  assert(pulse(1000, 1000, 2000) == 2000);
  assert(pulse(250, 2000, 1000) == 1750); // Servo invertido.

  Motion wrap;
  wrap.request(true, 0xFFFFFFF0UL);
  assert(wrap.update(4)); // millis() puede desbordarse durante el movimiento.
  assert(wrap.position() == 50);
}

void testDebounce() {
  DebouncedInput b;
  b.begin(false, 0);
  assert(!b.update(true, 5));
  assert(!b.update(false, 10));
  assert(!b.update(true, 15));
  assert(!b.update(true, 20));
  assert(b.update(true, 25));
  assert(b.value());
  for (uint32_t now = 30; now < 1000; now += 5) {
    assert(!b.update(true, now)); // Pulsacion sostenida: un solo evento.
  }
  b.begin(false, 0xFFFFFFF0UL);
  assert(!b.update(true, 0xFFFFFFF5UL));
  assert(b.update(true, 1));
}

void runCalibration(Calibration &c, uint16_t rest, uint16_t active,
                    bool noisy = false, uint32_t start = 0) {
  c.start(start);
  for (uint32_t ms = 1; ms <= 10000; ++ms) {
    const bool contracting = c.stage() == Calibration::ACTIVE ||
                             c.stage() == Calibration::PREPARE_ACTIVE;
    uint16_t value = contracting ? active : rest;
    if (noisy && c.stage() == Calibration::REST && ms % 500 == 0) value += 200;
    c.update(start + ms, value, true);
  }
}

void testCalibration() {
  Calibration c;
  runCalibration(c, 100, 500);
  assert(c.stage() == Calibration::SUCCESS);
  assert(c.thresholds().open == 200 && c.thresholds().close == 300);
  runCalibration(c, 100, 105);
  assert(c.stage() == Calibration::FAILED); // Contraccion insuficiente.
  runCalibration(c, 100, 500, true);
  assert(c.stage() == Calibration::FAILED); // Reposo demasiado ruidoso.
  runCalibration(c, 1023, 1023);
  assert(c.stage() == Calibration::FAILED);
  runCalibration(c, 100, 500, false, 0xFFFFF000UL);
  assert(c.stage() == Calibration::SUCCESS);
  c.start(0);
  c.update(1, 100, false);
  assert(c.stage() == Calibration::FAILED);
  c.start(0);
  for (uint32_t now = 10; now <= 5000; now += 10) c.update(now, 100, true);
  assert(c.stage() == Calibration::FAILED); // No aceptar adquisicion incompleta.
  c.cancel();
  assert(c.stage() == Calibration::IDLE && !c.running());
}

void testFilteredControl() {
  MovingAverage f;
  MuscleSwitch muscle;
  const Thresholds t = {200, 300};
  for (int i = 0; i < 32; ++i) f.add(100);
  muscle.update(100, t);
  assert(!muscle.update(f.add(1023), t));
  for (int i = 0; i < 32; ++i) muscle.update(f.add(100), t);
  int crossing = 0;
  for (int i = 1; i <= 32; ++i) {
    if (muscle.update(f.add(500), t) && !crossing) crossing = i;
  }
  assert(crossing == 16); // Escalon de 100 a 500, umbral de cierre 300.
  for (int i = 0; i < 32; ++i) muscle.update(f.add(100), t);
  assert(!muscle.update(100, t));
}

void testAd8232Signal() {
  assert(Config::SENSOR_MAX_ADC == 675);
  assert(Config::ADC_HIGH_CLIP == 669);
  assert(Config::CALIBRATION_SIGNATURE != 0xE701); // No reutilizar umbrales viejos.
  Ad8232Signal signal;
  for (uint16_t i = 0; i < Config::SIGNAL_SETTLE_SAMPLES - 1; ++i) {
    assert(signal.add(338, false) == 0);
    assert(!signal.ready());
  }
  assert(signal.add(338, false) == 0 && signal.ready());
  assert(signal.baseline() == 338); // ~1.65 V, no se interpreta como contraccion.

  Ad8232Signal lower, upper;
  uint16_t activity = 0;
  for (int i = 0; i < 5000; ++i) {
    const int wave = (i / 10) % 2 ? 80 : -80;
    const uint16_t a = lower.add(uint16_t(200 + wave), false);
    const uint16_t b = upper.add(uint16_t(450 + wave), false);
    assert(a == b); // Misma actividad aunque cambie el offset DC de la tarjeta.
    activity = a;
  }
  assert(lower.ready() && upper.ready());
  assert(activity > 30 && activity < 100); // Actividad conservada tras la nueva banda.

  assert(signal.add(338, true) == 0); // Electrodos desconectados.
  assert(!signal.ready());
  for (uint16_t i = 0; i < Config::SIGNAL_SETTLE_SAMPLES - 1; ++i) {
    signal.add(338, false);
    assert(!signal.ready());
  }
  signal.add(338, false);
  assert(signal.ready()); // Recuperacion requiere dos segundos continuos.
  signal.add(0, false);
  assert(signal.clipped() && !signal.ready());
  signal.add(Config::ADC_HIGH_CLIP, false);
  assert(signal.clipped() && !signal.ready());
  signal.add(1023, false); // Salida fuera del rango de 3.3 V.
  assert(signal.clipped() && !signal.ready());
  signal.add(338, false);
  assert(!signal.clipped() && !signal.ready());
  for (int i = 0; i < 5000; ++i) signal.add(438, false);
  assert(signal.ready());
  assert(signal.add(438, false) <= 1); // Un cambio DC no queda como cierre sostenido.
}

void testAd8232CalibrationAndContactLoss() {
  Ad8232Signal signal;
  Calibration c;
  for (int i = 0; i < 2000; ++i) signal.add(338, false);
  c.start(0);
  for (uint32_t now = 1; now <= 10000; ++now) {
    const bool active = c.stage() == Calibration::PREPARE_ACTIVE ||
                        c.stage() == Calibration::ACTIVE;
    const int amplitude = active ? 100 : 2;
    const int wave = (now / 10) % 2 ? amplitude : -amplitude;
    const uint16_t envelope = signal.add(uint16_t(338 + wave), false);
    assert(signal.ready());
    c.update(now, envelope, signal.ready());
  }
  assert(c.stage() == Calibration::SUCCESS);
  assert(c.thresholds().close > c.thresholds().open);
  assert(c.thresholds().close < 100);

  MuscleSwitch muscle;
  Motion hand;
  muscle.update(0, c.thresholds());
  hand.request(muscle.update(100, c.thresholds()), 10000);
  assert(hand.closed());
  signal.add(338, true);
  if (!signal.ready()) {
    muscle.reset();
    hand.request(false, 10001);
  }
  assert(!hand.closed()); // Perdida de contacto revierte la orden.
  assert(!muscle.update(100, c.thresholds())); // Debe relajar para rearmar.
  c.start(11000);
  c.update(11001, 0, signal.ready());
  assert(c.stage() == Calibration::FAILED);
}

int main() {
  testAverage();
  testMuscle();
  testMotion();
  testDebounce();
  testCalibration();
  testFilteredControl();
  testAd8232Signal();
  testAd8232CalibrationAndContactLoss();
  std::cout << "OK: AD8232, contacto, envolvente, EMG experimental, movimiento y calibracion\n";
}
