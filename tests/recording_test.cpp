#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "TestSupport.h"
#include "../Inmoov_EMG/Decision.h"
#include "../Inmoov_EMG/Motion.h"
int main() {
  // Simulacion por retencion de instantaneas 10 Hz. Se omiten fallas SOLO
  // aqui para explorar sensibilidad; no valida el registro ni el filtro 1 kHz.
  std::ifstream file("docs/mediciones/2026-11-09/ensayo_mano_30s/senal.csv");
  assert(file.is_open()); std::string line; std::getline(file,line);
  std::vector<uint16_t> values;
  while(std::getline(file,line)) {
    std::istringstream fields(line); std::string field;
    for(unsigned col=0; col<=6; ++col) assert(bool(std::getline(fields,field,',')));
    values.push_back(uint16_t(std::stoul(field)));
  }
  assert(values.size()==305);
  Hand::MuscleSwitch detector; Hand::Motion motion; uint32_t now=0;
  unsigned restClosures=0, activeClosures=0, fullClosures=0, fullOpenings=0;
  bool previous=false; uint16_t previousPosition=0;
  auto tick=[&](uint16_t value) {
    const bool closed=detector.update(value,{3,5},++now);
    if(closed && !previous) { if(now<10000) ++restClosures; else ++activeClosures; }
    previous=closed; motion.request(closed,now); motion.update(now);
    if(motion.position()==1000 && previousPosition!=1000) ++fullClosures;
    if(motion.position()==0 && previousPosition!=0) ++fullOpenings;
    previousPosition=motion.position();
    if(now<10000) assert(!closed);
  };
  for(uint16_t value:values) for(unsigned n=0;n<100;++n) tick(value);
  for(unsigned n=0;n<2500;++n) tick(0);
  assert(!restClosures && activeClosures && fullClosures && fullOpenings && motion.isOpen());
  result("REPLAY-30S", "instantaneas con fallas omitidas SOLO en simulacion", "cero cierres en reposo y 4 recorridos completos", fullClosures==4 && fullOpenings==4);
  std::cout<<"Replay exploratorio: reposo="<<restClosures<<" cierres; actividad="<<activeClosures
           <<"; cierres completos="<<fullClosures<<"; aperturas completas="<<fullOpenings<<"\n";
}
