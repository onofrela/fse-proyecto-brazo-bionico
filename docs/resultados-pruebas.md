# Resultados de pruebas automatizadas

Generado con `python scripts/test.py`, compilador `cl`, aserciones activas y advertencias como errores.

Todas las entradas son sinteticas. Los drivers GPIO/Servo/UART usan dobles; Application, cola, filtros y persistencia por bytes ejecutan el codigo del proyecto. La ISR AVR se verifica por compilacion; estas pruebas no simulan interrupciones de hardware ni demuestran ausencia de jitter o carreras en una placa.

R1-SW termina al ordenar el pulso; R2-SW mide el recorrido ordenado; R9-SW usa un bucle ideal de 1 ms. Son comparaciones de software contra los requisitos del proyecto, no mediciones de PWM, movimiento o consumo.

| Caso | Resultado esperado | Resultado obtenido | Estado |
| --- | --- | --- | --- |
| UNIT-01 | 8 grupos de regresion sin aserciones fallidas | 8 grupos completados (filtro, umbrales, rampa, UI, calibracion, AD8232 e integracion) | PASA |
| IPC-01 | FIFO sin sobrescritura y 1000 vueltas coherentes | FIFO y campos completos | PASA |
| R9-SW | LED y orden <=20 ms (fases 0..4, bucle 1 ms) | 14 ms maximo simulado | PASA |
| R2-SW | recorrido ordenado <=1000 ms | 780 ms simulados | PASA |
| E2E-01 | ADC sintetico -> cola -> filtro -> umbral -> servo y LED | cierre con primer paso y LED verde | PASA |
| R1-SW | primer paso <=50 ms desde escalon sintetico | 12 ms simulados (sin PWM fisico) | PASA |
| F1-SW | LO alto abre; 2000 muestras y reposo permiten nuevo cierre sin reset | apertura y recuperacion con nuevo cierre | PASA |
| F2-SW | ADC detenido abre; recuperacion sin reset tras estabilizar | timeout detectado y nuevo cierre | PASA |
| F3-SW | cola llena inhibe EMG y descarta pendientes | orden de apertura y alarma de perdida | PASA |
| F4-SW | muestra de 11 ms no renueva salud | muestra antigua rechazada | PASA |
| F9-SW | hueco >10 ms seguido de muestra nueva reinicia estabilizacion | nueva muestra aceptada sin habilitar EMG | PASA |
| BOOT-01 | arranque sin comandos -> calibracion -> guardado | calibracion automatica y EEPROM_SAVED | PASA |
| F5-SW | error de persistencia visible conservando control en RAM | EEPROM_SAVE_FAILED y storageFault | PASA |
| F6-SW | fallo attach impide ordenar motores | SERVO_INIT_FAILED y cero escrituras | PASA |
| F7-SW | fallo de arranque ADC inhibe EMG | ADC_INIT_FAILED sin cierre | PASA |
| F8-SW | recorte alto abre e invalida filtro | sin cierre ni salud y bit de recorte | PASA |
| TIME-01 | millis cruza UINT32_MAX sin falsa falla | salud conservada tras wrap | PASA |
| EEP-01 | firma/datos verificados y 10 bytes en pasos no bloqueantes | 10 escrituras y umbrales 20/40 | PASA |
| EEP-F1 | cortes tras escrituras 1..9 rechazan registro parcial | 9 cortes rechazados al reiniciar | PASA |
| EEP-F2 | checksum corrupto no habilita EMG | registro rechazado | PASA |
| EEP-F3 | fallo de escritura detectado por lectura | SaveResult::Failed | PASA |
| DRV-F1 | fallo del tercer attach deshace los anteriores | cero servos conectados | PASA |
| DRV-01 | 5 salidas con pulso de cierre y limites verificados | 5 pulsos de 2000 us ordenados | PASA |
| UART-F1 | UART llena descarta linea sin llamar write | una linea descartada y cero escrituras | PASA |
| UART-F2 | escritura corta se contabiliza como error | contador de descartes incrementado | PASA |
| UART-F3 | read=-1 y comando invalido no generan orden | ambos rechazados; c si inicia calibracion | PASA |
| UART-01 | linea de datos completa al recuperar capacidad | D,100,338,338,0,20,40,4,0,4 | PASA |
