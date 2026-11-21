#include <cassert>
#include <cmath>
#include <iostream>
#include "../Inmoov_EMG/Signal.h"
#include "../Inmoov_EMG/Inputs.h"
#include "../Inmoov_EMG/Motion.h"
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
  for (uint8_t n=0; n<Config::SERVO_COUNT; ++n) {
    assert(Config::OPEN_US[n]==(n < 2 ? 544 : 2400));
    assert(Config::CLOSED_US[n]==(n < 2 ? 2400 : 544));
    assert(pulse(0,Config::OPEN_US[n],Config::CLOSED_US[n])==Config::OPEN_US[n]);
    assert(pulse(1000,Config::OPEN_US[n],Config::CLOSED_US[n])==Config::CLOSED_US[n]);
  }
  Motion wrap; wrap.request(true,0xFFFFFFF0UL);
  assert(wrap.update(4) && wrap.position()==50);
  DebouncedInput b; b.begin(false,0);
  assert(!b.update(true,5)); assert(b.update(true,15)); assert(!b.update(true,100));
}

int main() {
  testSignalFaults(); testFrequencyResponse(); testWholeHandMotion();
  std::cout<<"OK: senal, filtro Q12/Q6, fallas, movimiento de mano completa y antirrebote\n";
}
