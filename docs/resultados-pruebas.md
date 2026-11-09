# Resultados de pruebas automatizadas

Generado con `python scripts/test.py`, compilador `cl`, aserciones activas y advertencias como errores.

Las pruebas C++ usan entradas sinteticas. Los drivers GPIO/Servo/UART usan dobles; Application, cola, filtros y persistencia por bytes ejecutan el codigo del proyecto. La ISR AVR se verifica por compilacion; estas pruebas no simulan interrupciones de hardware ni demuestran ausencia de jitter o carreras en una placa.

LAT-SW termina al ordenar el pulso (71 ms en este ensayo; no demuestra R1 de 50 ms); R2-SW mide el recorrido ordenado; R9-SW usa un bucle ideal de 1 ms. Son comparaciones de software contra los requisitos del proyecto, no mediciones de PWM, movimiento o consumo. Los CSV reales se leen sin recalcular sus filtros; los hashes verifican la evidencia original y no validan fisicamente el firmware posterior.

| Caso | Resultado esperado | Resultado obtenido | Estado |
| --- | --- | --- | --- |
| UNIT-01 | regresion con aserciones activas | filtro, calibracion, ponderacion, rampa, fallas y replay completados | PASA |
| FILTER-01 | 100 Hz conservados; deriva de 1 Hz atenuada | 0.168701 / 41.437363 RMS ADC | PASA |
| FILTER-02 | 60 Hz atenuados respecto a 100 Hz | 0.164501 RMS ADC | PASA |
| FILTER-03 | atenuacion fuera de banda | 10 Hz y 300 Hz bajo limites sinteticos | PASA |
| FILTER-04 | estado acotado y reinicio limpio | limites Q6 y salida cero tras clear | PASA |
| IPC-01 | FIFO sin sobrescritura y 1000 vueltas coherentes | FIFO y campos completos | PASA |
| R9-SW | LED y orden <=20 ms (fases 0..4, bucle 1 ms) | 14 ms maximo simulado | PASA |
| R2-SW | recorrido ordenado <=1000 ms | 780 ms simulados | PASA |
| E2E-01 | ADC -> cola -> filtro -> ponderacion -> servo | cierre con primer paso y LED verde | PASA |
| LAT-SW | primer paso <200 ms sinteticos; R1 de 50 ms no demostrado | 71 ms simulados | PASA |
| F1-SW | falla interrumpe retencion; estabilizacion y reposo rearman | nuevo cierre sin reset | PASA |
| F2-SW | ADC detenido abre y permite recuperacion | timeout y nuevo cierre | PASA |
| F3-SW | cola llena inhibe EMG y descarta pendientes | apertura y bit de perdida | PASA |
| F4-SW | muestra de 11 ms no renueva salud | muestra antigua rechazada | PASA |
| F9-SW | hueco seguido de muestra nueva reinicia estabilizacion | no habilita EMG | PASA |
| IPC-TIME | ponderacion identica al consumir 1 o 5 muestras por vuelta | 701 / 701 | PASA |
| BOOT-01 | D8 al encender -> calibracion guiada -> guardado | calibracion solicitada y EEPROM_SAVED | PASA |
| F5-SW | error de persistencia visible conservando control en RAM | EEPROM_SAVE_FAILED y storageFault | PASA |
| CAL-START | sin umbrales espera; c inicia sin reset | sin medicion inesperada y CAL_PREP_REST | PASA |
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
| UART-STATUS | consulta FW sin orden motora y sin bloquear TX | E,102,FW,6 | PASA |
| UART-ACTIVITY | D seguido de ponderacion 0..1000 | E,400,ACTIVITY,650 | PASA |
| PC-01 | protocolo, comandos, CSV antiguo y fallas 16/32 | 10 pruebas Python sin fallos | PASA |
| PC-DATOS | leer CSV reales sin reinterpretar su filtro | 469 y 709 instantaneas conservadas | PASA |
| EVIDENCIA | SHA256 original de mediciones y capturas | 12 archivos con integridad verificada | PASA |
