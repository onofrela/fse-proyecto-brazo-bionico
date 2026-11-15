#include <cassert>
#include "TestSupport.h"
#include "../Inmoov_EMG/Decision.h"
#include "../Inmoov_EMG/Motion.h"
int main() {
  uint32_t now=0;
  const uint16_t boundaries[]={0,4,5,9,10,1023};
  for (uint16_t value : boundaries) {
    Hand::ThreeLevelControl levels;
    for(unsigned n=0;n<199;++n) { assert(levels.add(value,++now)==0); assert(!levels.ready()); }
    const uint16_t expected=value<5 ? 0 : value<10 ? 500 : 1000;
    assert(levels.add(value,++now)==expected && levels.ready());
    assert(levels.meanX10()==value*10);
  }
  result("LEVEL-BOUNDARY","0/4 -> 0; 5/9 -> 90; 10/1023 -> 180","seis fronteras con bloques de 200 muestras",true);
  Hand::ThreeLevelControl avg;
  for(unsigned n=0;n<100;++n) avg.add(0,++now);
  for(unsigned n=0;n<100;++n) avg.add(10,++now);
  assert(avg.position()==500 && avg.meanX10()==50);
  avg.reset();
  for(unsigned n=0;n<199;++n) avg.add(5,++now);
  assert(avg.add(4,++now)==0); // Media 4.995, sin redondear hacia 5.
  avg.reset();
  for(unsigned n=0;n<199;++n) avg.add(10,++now);
  assert(avg.add(9,++now)==500); // Media 9.995, sin redondear hacia 10.
  result("LEVEL-MEAN","clasificar la media exacta, no ultimo valor ni media redondeada","5.000 -> 90; 4.995 -> 0; 9.995 -> 90",true);
  avg.reset();
  for(unsigned n=0;n<200;++n) avg.add(n==99 ? 100 : 0,++now);
  assert(avg.position()==0);
  avg.add(10,now+21); now+=21;
  assert(!avg.ready() && avg.position()==0);
  for(unsigned n=0;n<199;++n) avg.add(10,++now);
  assert(avg.ready() && avg.position()==1000);
  avg.reset(); assert(!avg.ready() && avg.position()==0);
  for(uint32_t n=0;n<200;++n) avg.add(9,UINT32_MAX-99+n);
  assert(avg.position()==500);
  result("LEVEL-FAULT","pico diluido, reset por hueco y wrap de millis","promedio reiniciado y vuelta de reloj aceptada",true);
  Hand::Motion motion;
  const uint16_t positions[]={500,1000,0};
  for(uint16_t position : positions) {
    motion.requestPosition(position,now);
    for(unsigned n=0;n<40;++n) {now+=20; motion.update(now);}
    assert(motion.position()==position);
  }
  assert(!motion.requestPosition(1001,now));
  assert(Hand::pulse(500,544,2400)==1472);
  result("LEVEL-RAMP","0/90/180 completos mediante rampa","0/500/1000 y pulso central 1472 us",true);
}
