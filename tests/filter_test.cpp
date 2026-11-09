#include <cassert>
#include <cmath>
#include <string>
#include "TestSupport.h"
#include "../Inmoov_EMG/SignalFilter.h"

double response(double hz) {
  Hand::ActivityFilter filter;
  double sum = 0;
  for (int i=0; i<6000; ++i) {
    const int32_t y = filter.addQ6(int32_t(std::sin(2*3.141592653589793*hz*i/1000)*60*64));
    if (i>=2000) sum += double(y)*double(y);
  }
  return std::sqrt(sum/4000)/64;
}
int main() {
  const double pass=response(100), slow=response(1), mains=response(60);
  result("FILTER-01", "100 Hz conservados; deriva de 1 Hz atenuada", std::to_string(slow)+" / "+std::to_string(pass)+" RMS ADC", pass>35 && pass<45 && slow<pass*.01);
  result("FILTER-02", "60 Hz atenuados respecto a 100 Hz", std::to_string(mains)+" RMS ADC", mains<pass*.03);
  result("FILTER-03", "atenuacion fuera de banda", "10 Hz y 300 Hz bajo limites sinteticos", response(10)<pass*.15 && response(300)<pass*.35);
  Hand::ActivityFilter filter;
  for(int i=0; i<10000; ++i) {
    const int32_t y=filter.addQ6(i%2 ? 65472 : -65472);
    assert(y>=-65472 && y<=65472);
  }
  filter.clear();
  for(int i=0; i<2000; ++i) assert(filter.addQ6(0)==0);
  result("FILTER-04", "estado acotado y reinicio limpio", "limites Q6 y salida cero tras clear", true);
}
