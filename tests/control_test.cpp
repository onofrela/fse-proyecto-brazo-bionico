#include <cassert>
#include <cmath>
#include <iostream>
#include "../Inmoov_EMG/Control.h"
using namespace Hand;

void testSignalFaults() {
  Ad8232Signal s;
  for (unsigned i=0; i<2000; ++i) {
    assert(s.add(338,false)==0);
    assert(s.ready() == (i==1999));
  }
  assert(s.baseline()==338);
  assert(s.add(338,true)==0 && !s.ready());
  for (unsigned i=0; i<2000; ++i) s.add(338,false);
  assert(s.ready());
  for (uint16_t v : {uint16_t(0),uint16_t(669),uint16_t(1023)})
    assert(s.add(v,false)==0 && s.clipped() && !s.ready());
  for (unsigned i=0; i<2000; ++i) s.add(438,false);
  assert(s.ready() && s.add(438,false)<=1);
  assert(Config::CALIBRATION_SIGNATURE == 0xE706);
}

double response(double hz) {
  ActivityFilter f;
  double sum=0;
  for(int i=0; i<6000; ++i) {
    int32_t y=f.addQ6(int32_t(std::sin(2*3.141592653589793*hz*i/1000)*60*64));
    if(i>=2000) sum+=double(y)*double(y);
  }
  return std::sqrt(sum/4000)/64;
}

void testFrequencyResponse() {
  const double pass=response(100);
  assert(pass>35 && pass<45);
  assert(response(1)<pass*0.01);
  assert(response(10)<pass*0.15);
  assert(response(60)<pass*0.03);
  assert(response(300)<pass*0.35);
  Ad8232Signal a,b;
  for(int i=0; i<6000; ++i) {
    const int wave=int(50*std::sin(2*3.141592653589793*100*i/1000));
    assert(a.add(uint16_t(200+wave),false)==b.add(uint16_t(450+wave),false));
  }
  std::cout<<"RMS sintetico ADC: 1Hz="<<response(1)<<" 10Hz="<<response(10)
           <<" 60Hz="<<response(60)<<" 100Hz="<<pass<<" 300Hz="<<response(300)<<"\n";
}

void testDwellAndHysteresis() {
  const Thresholds t={8,12}; MuscleSwitch m; uint32_t now=0;
  auto hold = [&](uint16_t value, uint16_t duration) {
    bool closed=false;
    for(uint16_t i=0; i<duration; ++i) {
      closed=m.update(value,t,++now);
      assert(m.activity()<=1000);
    }
    return closed;
  };
  assert(!hold(30,2000)); // No cerrar sin haber relajado primero.
  assert(!hold(5,600));
  assert(!hold(50,20)); // Impulso breve descartado por ponderacion.
  assert(!hold(5,600));
  assert(!hold(9,1200)); // Reposo de una cifra no debe cerrar.
  assert(!hold(5,600));
  assert(hold(14,100));
  assert(m.activity()>=650);
  assert(hold(5,200)); // No invertir la rampa al primer descenso.
  assert(hold(5,600)); // Aun no se cumplen 900 ms desde la orden.
  assert(!hold(5,300)); // Reposo confirmado: apertura.
  assert(hold(32,150));
  now+=100;
  assert(!m.update(32,t,now)); // Hueco de control: apertura/rearme.
  assert(m.activity()==0);
  assert(!hold(32,200));
  assert(!hold(5,600));
  assert(hold(32,150));
  assert(!m.update(32,Thresholds{0,0},++now));
  MuscleSwitch wrap;
  for(uint32_t i=0; i<200; ++i) assert(!wrap.update(5,t,0xFFFFFF80UL+i));
  bool closed=false;
  for(uint32_t i=200; i<350; ++i) closed=wrap.update(32,t,0xFFFFFF80UL+i);
  assert(closed);
}

void testBurstyEnvelopeFullStroke() {
  // Reproduccion funcional a partir de instantaneas a 10 Hz; no reconstruye
  // la senal original de 1 kHz. Cada valor se mantiene 100 ms en el ensayo.
  const uint16_t values[]={15,13,11,13,5,4,6,6,18,21,35,5,8,7,9,12,4,7,9,4,19,4,12,23,28,7};
  MuscleSwitch detector; Motion hand; const Thresholds t={8,12}; uint32_t now=0;
  uint16_t maximum=0;
  auto tick=[&](uint16_t envelope) {
    ++now;
    hand.request(detector.update(envelope,t,now),now);
    hand.update(now);
    if(hand.position()>maximum) maximum=hand.position();
  };
  for(int i=0;i<600;++i) tick(5);
  for(uint16_t value:values) for(int i=0;i<100;++i) tick(value);
  assert(maximum==1000); // La orden permite completar el recorrido.
  for(int i=0;i<2000;++i) tick(5);
  assert(hand.isOpen());
  std::cout<<"Replay de envolventes intermitentes: maximo="<<maximum<<"/1000; final abierto\n";
}

void runCalibration(Calibration &c, int mode=0, uint32_t origin=0) {
  c.start(origin);
  for(uint32_t ms=1; ms<=22000; ++ms) {
    uint16_t value=5;
    if(c.stage()==Calibration::REST && mode==2 && ms%1000<20) value=150;
    if(c.stage()==Calibration::REST && mode==4) value=ms%200<100 ? 5 : 25;
    if(c.stage()==Calibration::ACTIVE) {
      value=(ms%2000<1000) ? 30 : 5;
      if(mode==1) value=5;
      if(mode==3) value=300;
    }
    c.update(origin+ms,value,true);
  }
}

void testRobustCalibration() {
  Calibration c;
  runCalibration(c);
  assert(c.stage()==Calibration::SUCCESS);
  assert(c.rest95()==5 && c.active70()==31);
  assert(c.thresholds().open==8 && c.thresholds().close==12);
  runCalibration(c,1);
  assert(c.stage()==Calibration::FAILED && c.reason()==Calibration::LOW_SEPARATION);
  runCalibration(c,2);
  assert(c.stage()==Calibration::SUCCESS);
  runCalibration(c,3);
  assert(c.stage()==Calibration::FAILED && c.reason()==Calibration::RANGE_EXCEEDED);
  runCalibration(c,4);
  assert(c.stage()==Calibration::FAILED && c.reason()==Calibration::LOW_SEPARATION);
  runCalibration(c,0,0xFFFFF000UL); assert(c.stage()==Calibration::SUCCESS);
  c.start(0); c.update(1,5,false);
  assert(c.stage()==Calibration::FAILED && c.reason()==Calibration::SIGNAL_LOST);
  c.start(0);
  for(uint32_t ms=100; ms<=9000; ms+=100) c.update(ms,5,true);
  assert(c.stage()==Calibration::FAILED && c.reason()==Calibration::TOO_FEW_SAMPLES);
  c.cancel(); assert(!c.running() && c.stage()==Calibration::IDLE);
}

void testWholeHandMotion() {
  Motion hand;
  assert(hand.isOpen() && hand.request(true,0));
  for(uint32_t ms=20; ms<=780; ms+=20) hand.update(ms);
  assert(hand.closed() && hand.position()==1000);
  for(uint8_t finger=0; finger<5; ++finger) {
    assert(pulse(hand.position(),Config::OPEN_US[finger],Config::CLOSED_US[finger])==Config::CLOSED_US[finger]);
  }
  hand.request(false,800);
  for(uint32_t ms=820; ms<=1580; ms+=20) hand.update(ms);
  assert(hand.isOpen() && hand.position()==0);
  assert(pulse(250,2000,1000)==1750);
  Motion wrap; wrap.request(true,0xFFFFFFF0UL);
  assert(wrap.update(4) && wrap.position()==50);
  DebouncedInput b; b.begin(false,0);
  assert(!b.update(true,5)); assert(b.update(true,15)); assert(!b.update(true,100));
}

void testSignalCalibrationControl() {
  Calibration c; Ad8232Signal signal; Motion hand; MuscleSwitch muscle;
  for(int i=0; i<2000; ++i) signal.add(338,false);
  c.start(0);
  for(uint32_t ms=1; ms<=22000; ++ms) {
    const bool active=c.stage()==Calibration::ACTIVE && ms%2000<1000;
    const int wave=int((active ? 70 : 2)*std::sin(2*3.141592653589793*100*ms/1000));
    const uint16_t envelope=signal.add(uint16_t(338+wave),false);
    c.update(ms,envelope,signal.ready());
  }
  assert(c.stage()==Calibration::SUCCESS);
  for(uint32_t ms=23001; ms<23150; ++ms) muscle.update(0,c.thresholds(),ms);
  for(uint32_t ms=23150; ms<23300; ++ms) hand.request(muscle.update(50,c.thresholds(),ms),ms);
  assert(hand.closed());
  signal.add(338,true);
  if(!signal.ready()) { muscle.reset(); hand.request(false,23301); }
  assert(!hand.closed() && !muscle.update(50,c.thresholds(),23302));
}

int main() {
  testSignalFaults(); testFrequencyResponse(); testDwellAndHysteresis();
  testRobustCalibration(); testWholeHandMotion(); testSignalCalibrationControl(); testBurstyEnvelopeFullStroke();
  std::cout<<"OK: filtro Q12/Q6, fallas, calibracion robusta, histeresis y movimiento simultaneo de mano completa\n";
}
