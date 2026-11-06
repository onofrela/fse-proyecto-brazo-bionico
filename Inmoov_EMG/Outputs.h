#ifndef INMOOV_OUTPUTS_H
#define INMOOV_OUTPUTS_H
#include <stdint.h>
namespace Outputs {
bool begin();
bool setPosition(uint16_t position);
void led(bool red, bool green, bool blue);
bool emgSelected();
bool buttonPressed();
}
#endif
