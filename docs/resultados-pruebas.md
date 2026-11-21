# Resultados de pruebas automatizadas

Generado con `python scripts/test.py`, compilador `cl`, aserciones activas y advertencias como errores.

Las pruebas C++ usan entradas sinteticas. Los drivers GPIO/Servo/UART usan dobles; Application, cola, filtros y persistencia por bytes ejecutan el codigo del proyecto. La ISR AVR se verifica por compilacion; estas pruebas no simulan interrupciones de hardware ni demuestran ausencia de jitter o carreras en una placa. FW8 usa ThreeLevelControl, sin calibracion. Los casos de Calibration y MuscleSwitch son regresiones de utilidades anteriores, no el control activo.

LAT-SW termina al ordenar el pulso (ver LAT-SW; no demuestra R1 de 50 ms); R2-SW mide el recorrido ordenado; R9-SW usa un bucle ideal de 1 ms. Son comparaciones de software contra los requisitos del proyecto, no mediciones de PWM, movimiento o consumo. REPLAY-30S mantiene cada instantanea 100 ms y omite sus fallas solo en esa simulacion exploratoria; no valida la adquisicion ni recalcula el filtro. Los CSV reales se leen sin recalcular sus filtros; los hashes verifican la evidencia original y no validan fisicamente el firmware posterior.

| Caso | Resultado esperado | Resultado obtenido | Estado |
| --- | --- | --- | --- |
| UNIT-01 | regresion con aserciones activas | utilidades historicas de filtro/calibracion/ponderacion y rampa | PASA |
| LEVEL-BOUNDARY | 0/4 -> 0; 5/9 -> 90; 10/1023 -> 180 | seis fronteras con bloques de 200 muestras | PASA |
| LEVEL-MEAN | clasificar la media exacta, no ultimo valor ni media redondeada | 5.000 -> 90; 4.995 -> 0; 9.995 -> 90 | PASA |
| LEVEL-FAULT | pico diluido, reset por hueco y wrap de millis | promedio reiniciado y vuelta de reloj aceptada | PASA |
| LEVEL-RAMP | 0/90/180 completos mediante rampa | 0/500/1000 y pulso central 1472 us | PASA |
| FILTER-01 | 100 Hz conservados; deriva de 1 Hz atenuada | 0.168701 / 41.437363 RMS ADC | PASA |
| FILTER-02 | 60 Hz atenuados respecto a 100 Hz | 0.164501 RMS ADC | PASA |
| FILTER-03 | atenuacion fuera de banda | 10 Hz y 300 Hz bajo limites sinteticos | PASA |
| FILTER-04 | estado acotado y reinicio limpio | limites Q6 y salida cero tras clear | PASA |
| REPLAY-30S | instantaneas con fallas omitidas SOLO en simulacion | cero cierres en reposo y 4 recorridos completos | PASA |
| E2E-LEVEL | sin calibracion: ADC -> filtro -> media -> 0/90/180 | tres destinos completos; EEPROM de calibracion intacta | PASA |
| LEVEL-PAUSE | x abre y pausa; c reactiva sin calibrar | pausa persistente incluso con actividad alta | PASA |
| LEVEL-SAFETY | contacto/timeout/cola abren y vacian promedio | fallas respetadas y recuperacion sin calibrar | PASA |
| TIME-ISR | muestra posterior a tick no parece antigua, tambien al wrap | sin falsa perdida; 11 ms antiguos siguen rechazados | PASA |
| IPC-TIME | cada muestra contada una vez en lotes 1/5 | misma media y objetivo 90 grados | PASA |
| HW-MANUAL | fallos de arranque inhiben salida y D8 conserva modo manual | servos/ADC fallidos y pulsaciones manuales verificados | PASA |
| EEP-01 | firma/datos verificados y 10 bytes en pasos no bloqueantes | 10 escrituras y umbrales 20/40 | PASA |
| EEP-F1 | cortes tras escrituras 1..9 rechazan registro parcial | 9 cortes rechazados al reiniciar | PASA |
| EEP-F2 | checksum corrupto no habilita EMG | registro rechazado | PASA |
| EEP-F3 | fallo de escritura detectado por lectura | SaveResult::Failed | PASA |
| DRV-F1 | fallo del tercer attach deshace los anteriores | cero servos conectados | PASA |
| DRV-01 | cierre con medio, anular y menique invertidos | 2400 us en pulgar/indice; 544 us en medio/anular/menique | PASA |
| DRV-MID | 90 grados conservados y retorno a apertura por dedo | 1472 us en cinco salidas; apertura 544/544/2400/2400/2400 us | PASA |
| DRV-REVERSE | rampa con sentidos opuestos entre los dos grupos | D3/D5 suben; D6/D10/D11 bajan en los 40 pasos | PASA |
| UART-F1 | UART llena descarta linea sin llamar write | una linea descartada y cero escrituras | PASA |
| UART-F2 | escritura corta se contabiliza como error | contador de descartes incrementado | PASA |
| UART-F3 | read=-1 y comando invalido no generan orden | ambos rechazados; c reactiva control fijo | PASA |
| UART-01 | linea de datos completa al recuperar capacidad | D,100,338,338,0,20,40,4,0,4 | PASA |
| UART-STATUS | consulta FW sin orden motora y sin bloquear TX | E,102,FW,8 | PASA |
| UART-REVERSE | consulta identifica los tres servos invertidos | SERVO_REVERSE_MASK=28 | PASA |
| UART-MEAN | D seguido de promedio en decimas | E,400,MEAN_X10,64 | PASA |
| PC-01 | protocolo, comandos, CSV antiguo y fallas 16/32 | 11 pruebas Python sin fallos | PASA |
| PC-DATOS | leer CSV reales sin reinterpretar su filtro | 305, 469 y 709 instantaneas conservadas | PASA |
| EVIDENCIA | SHA256 original de mediciones y capturas | 12 archivos con integridad verificada | PASA |
