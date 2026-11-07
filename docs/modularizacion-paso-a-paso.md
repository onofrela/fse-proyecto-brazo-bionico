# Modularizacion paso a paso

Registro de la extraccion e integracion de los modulos desde el codigo base. Cada paso describe su responsabilidad y las verificaciones de software; las mediciones fisicas se documentan por separado.

Orden: apoyos de hardware, parametros, extraccion de logica, drivers, adquisicion, coordinador, pruebas y documentacion. Cada paso describe el codigo que incorpora.

## Apoyo de servomotor

Se incorpora el sketch existente y su guia: UART a 9600, angulos de 0 a 180 y posicion inicial de 90 grados. La confirmacion serial es una orden; no mide movimiento. Se verifico compilacion UNO.

## Apoyo de ADC

Se incorpora el sketch existente para entender el offset y las lecturas crudas del AD8232, junto con las senales de contacto. Muestreo nominal de 500 Hz, sin EEPROM. Se verifico compilacion UNO.

## Parametros de operacion

Config.h centraliza pines, limites, tiempos, tamano de cola y direcciones EEPROM. Se conservan los pines y el formato de calibracion de la base.

## Extraccion de Signal.h

La responsabilidad de senal AD8232 y media movil sale de Control.h. Control.h incluye el modulo para mantener compatibles el sketch y las pruebas existentes. Este paso conserva el punto de entrada anterior y compila para UNO.

## Extraccion de Decision.h

La responsabilidad de umbrales e histeresis muscular sale de Control.h. Control.h incluye el modulo para mantener compatibles el sketch y las pruebas existentes. Este paso conserva el punto de entrada anterior y compila para UNO.

## Extraccion de Inputs.h

La responsabilidad de antirrebote de entradas sale de Control.h. Control.h incluye el modulo para mantener compatibles el sketch y las pruebas existentes. Este paso conserva el punto de entrada anterior y compila para UNO.

## Extraccion de Motion.h

La responsabilidad de rampa simultanea de dedos sale de Control.h. Control.h incluye el modulo para mantener compatibles el sketch y las pruebas existentes. Este paso conserva el punto de entrada anterior y compila para UNO.

## Extraccion de Calibration.h

La responsabilidad de maquina de estados de calibracion sale de Control.h. Control.h incluye el modulo para mantener compatibles el sketch y las pruebas existentes. Este paso conserva el punto de entrada anterior y compila para UNO.

## Regresion de logica separada

Se hacen explicitas las conversiones de las senales sinteticas para compilar tambien con MSVC y advertencias como errores. Se ejecutan los ocho grupos originales sobre los modulos separados.

## Validacion y rearme de senal

Se agrega reset del filtro para recuperarse de huecos de adquisicion, se rechazan umbrales invalidos y valores fuera de rango al calibrar. La adquisicion aun reside en el sketch.

## Driver Outputs conectado

Se extraen GPIO, RGB y cinco canales Servo. El sketch llama al driver y comprueba errores de inicio/escritura. El driver revierte un attach parcial. Se conserva la coordinacion anterior.

## Driver Diagnostics conectado

Se retira el formato UART del sketch. Diagnostics valida read/write y capacidad de salida, limita comandos por vuelta y descarta telemetria antes de bloquear el control.

## Transaccion de calibracion

CalibrationStore separa el formato persistente de la EEPROM concreta. La transaccion invalida firma, escribe datos/checksum y confirma firma; cada byte se verifica antes del siguiente.

## Driver Storage conectado

El sketch carga umbrales/contador a traves de Storage. Las escrituras de calibracion se reparten entre vueltas mediante pollStorage y se comprueban sus resultados. Se elimina EEPROM.put masivo durante la operacion.

## Cola de comunicacion

SampleQueue incorpora muestras con tiempo, valor ADC y contacto en memoria estatica. Productor y consumidor tienen indices separados y la cola llena no sobrescribe pendientes. La conexion con ISR se realiza en el siguiente paso.

## Driver Acquisition conectado

Timer2 y ADC salen del sketch. La ISR publica en la cola y Acquisition protege copias/contadores con ATOMIC_RESTORESTATE. El puente readSample consume y filtra en el bucle; se comprueban timeout, antiguedad y perdida. La coordinacion por muestra se completa con Application.

## Coordinador Application conectado

Se trasladan modo, UI, calibracion y movimiento al propietario unico Application. Se elimina el puente temporal y se procesa cada muestra una vez con su hora de captura. El .ino queda reducido a setup/loop; seguridad, UI, rampa, EEPROM y UART tienen orden cooperativo explicito.

## Pruebas de persistencia

Se incorpora soporte de resultados esperado/obtenido y se prueban EEPROM ocupada, transaccion completa, cortes tras los pasos 1..9, checksum corrupto y byte que no se escribe.

## Pruebas de drivers

Los dobles Arduino/Servo permiten comprobar rollback de attach y los cinco pulsos ordenados; UART llena, escritura corta, lectura fallida y recuperacion de salida se prueban sin hardware.

## Pruebas de extremo a extremo

Se verifica Application con cola/filtros reales y drivers simulados: arranque, UI, tiempos logicos, contacto, timeout ADC, cola llena, datos antiguos y recuperacion. No son mediciones electricas ni mecanicas.
