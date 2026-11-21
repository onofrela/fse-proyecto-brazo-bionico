# Resultados de pruebas automatizadas

Generado con `python scripts/test.py`, compilador `cl`, aserciones activas y advertencias como errores.

Las pruebas C++ usan entradas sinteticas. Los drivers GPIO/Servo/UART usan dobles; Application, cola, filtros y persistencia por bytes ejecutan el codigo del proyecto. La ISR AVR se verifica por compilacion; estas pruebas no simulan interrupciones de hardware ni demuestran ausencia de jitter o carreras en una placa. FW8 usa ThreeLevelControl, sin calibracion.

LAT-FW8 mide desde la primera muestra ADC de una contraccion sintetica hasta el primer cambio de destino y el primer paso escrito, recorriendo las 200 fases del bloque con un bucle de 1 ms. Es simulacion de software, no medicion en la mano ni cumplimiento de R1 (<=50 ms). R2-SW mide el recorrido ordenado; R9-SW usa un bucle ideal de 1 ms. Son comparaciones de software contra los requisitos del proyecto, no mediciones de PWM, movimiento o consumo. REPLAY-30S retiene cada fila sintetica 500 ms en un bucle ideal de 1 ms, sin reconstruir adquisicion ni filtro. PC-DATOS lee ese mismo CSV y EVIDENCIA verifica su manifiesto; no validan fisicamente el firmware.

| Caso | Resultado esperado | Resultado obtenido | Estado |
| --- | --- | --- | --- |
| UNIT-01 | regresion con aserciones activas | senal, respuesta del filtro, rampa y antirrebote | PASA |
| LEVEL-BOUNDARY | 0/4 -> 0; 5/9 -> 90; 10/1023 -> 180 | seis fronteras con bloques de 200 muestras | PASA |
| LEVEL-MEAN | clasificar la media exacta, no ultimo valor ni media redondeada | 5.000 -> 90; 4.995 -> 0; 9.995 -> 90 | PASA |
| LEVEL-FAULT | pico diluido, reset por hueco y wrap de millis | promedio reiniciado y vuelta de reloj aceptada | PASA |
| LEVEL-RAMP | 0/90/180 completos mediante rampa | 0/500/1000 y pulso central 1472 us | PASA |
| REPLAY-30S | CSV sintetico retenido 500 ms por fila, sin adquisicion ni filtro | 30 s simulados; destinos y rampas 0/90/180/0/180/0 completos | PASA |
| FILTER-01 | 100 Hz conservados; deriva de 1 Hz atenuada | 0.168701 / 41.437363 RMS ADC | PASA |
| FILTER-02 | 60 Hz atenuados respecto a 100 Hz | 0.164501 RMS ADC | PASA |
| FILTER-03 | atenuacion fuera de banda | 10 Hz y 300 Hz bajo limites sinteticos | PASA |
| FILTER-04 | estado acotado y reinicio limpio | limites Q6 y salida cero tras clear | PASA |
| E2E-LEVEL | sin calibracion: ADC -> filtro -> media -> 0/90/180 | tres destinos completos sin calibracion | PASA |
| LEVEL-PAUSE | x abre y pausa; c reactiva sin calibrar | pausa persistente incluso con actividad alta | PASA |
| LEVEL-SAFETY | contacto/timeout/cola abren y vacian promedio | fallas respetadas y recuperacion sin calibrar | PASA |
| TIME-ISR | muestra posterior a tick no parece antigua, tambien al wrap | sin falsa perdida; 11 ms antiguos siguen rechazados | PASA |
| IPC-TIME | cada muestra contada una vez en lotes 1/5 | misma media y objetivo 90 grados | PASA |
| HW-UART | fallas siguen visibles sin RGB | eventos de senal, servos, ADC y EEPROM conservados | PASA |
| HW-MANUAL | fallos de arranque inhiben salida y D8 conserva modo manual | servos/ADC fallidos y pulsaciones manuales verificados | PASA |
| LAT-FW8 | simulacion de software, bucle 1 ms, 200 fases por amplitud; ADC -> filtro -> bloque -> rampa; no acredita R1 | amplitud 12 ADC: destino mejor 161 ms (fase 38), peor 360 ms (fase 39), primer paso escrito mejor 161 ms (fase 38), peor 360 ms (fase 39); amplitud 40 ADC: destino mejor 58 ms (fase 141), peor 257 ms (fase 142), primer paso escrito mejor 58 ms (fase 141), peor 257 ms (fase 142); reposo ADC 338, seno 100 Hz; fase=0..199 muestras previas; retardo desde primera muestra de contraccion (0 ms si misma vuelta); sin ISR/PWM ni medicion en la mano | PASA |
| EEP-01 | arranques 1 y 2 con contador/inverso y escritura verificada | 8 bytes suficientes; contador 2 persistido | PASA |
| EEP-F1 | registro parcial rechazado al reiniciar | reinicio en 1 verificado | PASA |
| EEP-F2 | inverso corrupto rechazado | reinicio en 1 verificado | PASA |
| EEP-F3 | fallo de escritura detectado por lectura de ambas palabras | bootVerified=false para fallo en contador o inverso | PASA |
| EEP-02 | desborde saturado sin volver a 1 ni a 0 | UINT32_MAX-1 conservado en arranques sucesivos | PASA |
| EEP-F4 | valor reservado rechazado aunque el inverso coincida | reinicio en 1 verificado | PASA |
| EEP-F5 | EEPROM insuficiente no se escribe ni se verifica | contador 0 y bootVerified=false sin escrituras | PASA |
| DRV-F1 | fallo del tercer attach deshace los anteriores | cero servos conectados | PASA |
| DRV-01 | cierre con medio, anular y menique invertidos | 2400 us en pulgar/indice; 544 us en medio/anular/menique | PASA |
| DRV-MID | 90 grados conservados y retorno a apertura por dedo | 1472 us en cinco salidas; apertura 544/544/2400/2400/2400 us | PASA |
| DRV-REVERSE | rampa con sentidos opuestos entre los dos grupos | D3/D5 suben; D6/D10/D11 bajan en los 40 pasos | PASA |
| DRV-FREE | D7/D12/D13 libres durante inicio y movimiento | sin pinMode ni digitalWrite en los tres pines | PASA |
| UART-OFF | telemetria periodica apagada al arrancar | sin D ni eventos auxiliares antes de p | PASA |
| UART-F1 | UART llena descarta linea sin llamar write | una linea descartada y cero escrituras | PASA |
| UART-F2 | escritura corta se contabiliza como error | contador de descartes incrementado | PASA |
| UART-F3 | read=-1 y comando invalido no generan orden | ambos rechazados; c reactiva control fijo | PASA |
| UART-01 | linea de datos completa al recuperar capacidad | D,100,338,338,0,20,40,4,0,4 | PASA |
| UART-STATUS | consulta FW sin orden motora y sin bloquear TX | E,102,FW,8 | PASA |
| UART-REVERSE | consulta identifica los tres servos invertidos | SERVO_REVERSE_MASK=28 | PASA |
| UART-MEAN | D seguido de promedio en decimas | E,400,MEAN_X10,64 | PASA |
| UART-STOP | p vuelve a apagar telemetria periodica | sin nuevas lineas D ni auxiliares | PASA |
| UART-NORMAL | version normal sin inicio, lectura ni escritura UART | sin baud, comando sin consumir y cero escrituras/descartes | PASA |
| PC-01 | protocolo, comandos, CSV antiguo y fallas 16/32 | suite Python sin fallos | PASA |
| PC-SEG | ida y vuelta AES-256-GCM, rechazo de contrasena incorrecta, archivos y manifiesto alterados | suite de privacidad, consentimiento y retencion sin fallos | PASA |
| PC-DATOS | leer CSV sintetico sin reinterpretar valores | 60 filas y 22 columnas conservadas | PASA |
| EVIDENCIA | manifiesto del CSV sintetico, sin datos biometricos | origen, 60 filas, columnas, periodo y SHA256 verificados | PASA |
