#ifndef TEST_AVR_EEPROM_H
#define TEST_AVR_EEPROM_H
#include <EEPROM.h>
inline void eeprom_busy_wait() { ++EEPROM.waits; EEPROM.finishWrite(); }
#endif
