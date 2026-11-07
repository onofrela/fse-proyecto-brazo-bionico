#ifndef INMOOV_ACQUISITION_H
#define INMOOV_ACQUISITION_H
#include "SampleQueue.h"
namespace Acquisition {
bool begin();
bool take(Hand::RawSample &sample);
bool takeDataLoss(); // Lee y limpia la alarma de forma atomica.
void discardPending();
uint16_t overruns();
}
#endif
