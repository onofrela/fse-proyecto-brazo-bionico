# Modularizacion paso a paso

Registro de la extraccion e integracion de los modulos desde el codigo base. Cada paso describe su responsabilidad y las verificaciones de software; las mediciones fisicas se documentan por separado.

Orden: apoyos de hardware, parametros, extraccion de logica, drivers, adquisicion, coordinador, pruebas y documentacion. Cada paso describe el codigo que incorpora.

## Apoyo de servomotor

Se incorpora el sketch existente y su guia: UART a 9600, angulos de 0 a 180 y posicion inicial de 90 grados. La confirmacion serial es una orden; no mide movimiento. Se verifico compilacion UNO.

## Apoyo de ADC

Se incorpora el sketch existente para entender el offset y las lecturas crudas del AD8232, junto con las senales de contacto. Muestreo nominal de 500 Hz, sin EEPROM. Se verifico compilacion UNO.
