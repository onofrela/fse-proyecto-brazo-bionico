/* Mano InMoov con AD8232 a 3.3 V y Arduino UNO R3.
 * Adaptacion del ejemplo de Kuldip.K Gami. Control experimental de actividad.
 * Modularizacion: aqui solo se conectan Arduino y Application.
 * Ver README.md y docs/bitacora.md para decisiones y limites medidos.
 */
#include <Arduino.h>
#include "Application.h"
namespace { Hand::Application application; }
void setup() { application.begin(millis()); }
void loop() { application.tick(millis()); }
