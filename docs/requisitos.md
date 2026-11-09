# Matriz de requisitos y evidencia

Esta matriz evalúa los requisitos del proyecto sobre la base Arduino disponible. “Implementado” describe código existente; “simulado” describe entradas sintéticas; “pendiente físico” requiere montar y medir. No se declara cumplimiento de hardware por compilar.

| Criterio solicitado | Evidencia | Estado y límite |
| --- | --- | --- |
| Estructura modular, códigos separados, nombres claros | Tabla del README; drivers y clases de dominio independientes | Implementado |
| Constantes con nombre | `Config.h`; parámetros de memoria/temporización y límites centralizados | Implementado en firmware principal; 0/1 y estructura de bytes conservan su significado convencional |
| Errores en cada llamada al sistema | No hay syscalls en bare-metal. ADC con timeout/lectura de registros, Servo attach y estado comprobados, EEPROM verificada, UART read/write/snprintf comprobados | Implementado para APIs con estado. GPIO/Servo write son void y no pueden confirmar funcionamiento físico; scripts comprueban códigos de salida |
| Comentarios útiles en español | Contratos de cola, propiedad de datos, ISR y D01–D06 | Implementado |
| Pruebas con esperado/obtenido y >=2 fallas | `resultados-pruebas.md`, generado por `scripts/test.py` | Pruebas nativas ejecutadas; incluye recuperación de contacto y pérdida de ADC |
| Historial legible y etiquetas | Serie local en main, desde evidencia v0.5.1-mediciones hasta v0.6.0 | Etapas y etiquetas descritas en versiones.md; base modular conservada |
| Diagrama conectado y flujo completo | `arquitectura.md`, E2E-01 y BOOT-01 | Flujo software integrado y probado con dobles; demostración HW pendiente |
| Comunicación, IPC y sincronización | `SampleQueue.h`, `Acquisition.cpp`, IPC-01 | Cola compartida SPSC y secciones atómicas; sin hilos/procesos porque el diseño utiliza bare-metal. No cumple literalmente IPC de un SO |
| Inicio al encender | `setup()` → `Application::begin`, BOOT-01 | Implementado sin espera USB; confirmar desconectando PC en placa |
| Configuración repetible | `bootstrap.sh`, `build.py`, `verify.sh`, versiones del README | Compilación de firmware y variante sin UART/sonda verificadas; bootstrap con descarga pendiente en equipo limpio |
| Tiempo / consumo vs compromiso | LAT-SW, R2-SW, R9-SW y protocolo físico | Ponderación: 71 ms sintéticos; objetivo de 50 ms no demostrado; consumo pendiente |
| Tareas con prioridad | `Application::tick`, ISR y tabla de bitácora | Orden cooperativo explícito y prioridades de hardware fijas; no hay prioridades de RTOS |
| Robustez | Contacto, timeout, cola llena, muestras antiguas, EEPROM y UART | Recuperación de fallas de señal simulada; falla de arranque ADC requiere reinicio, falla mecánica no detectable |

## Objetivos de validación

| ID | Compromiso | Resultado disponible | Falta para aceptación |
| --- | --- | --- | --- |
| R1 | <=50 ms a primer PWM modificado, 20 eventos | 71 ms hasta orden en ensayo ponderado, sin PWM ni filtro analógico | No demostrado: revisar el compromiso de rechazo de ruido/latencia y medir en placa |
| R2 | Cierre <=1 s, promedio de 10 cierres | Recorrido ordenado 780 ms con bucle ideal de 1 ms | Video >=60 fps con batería >=12 V |
| R3 | 0–150 grados ±5 por dedo | Pulsos configurables 1000/2000 us | Goniómetro, 3 repeticiones por dedo |
| R4 | 2.5 kg durante 60 s; elongación <=2% | Sin prueba mecánica | Ensayo de tensor y nudos |
| R5 | >=90 min; promedio <=1 A a 5 V | Sin medida de corriente/autonomía | Ciclo cada 10 s, voltaje cada 10 min, fin a 10.5 V |
| R6 | >=90% detección; <=1 falsa/min | Control de actividad sintética y rearme verificados | 50 contracciones y 5 min de reposo |
| R7 | 20 ms ±1%; pulsos 1–2 ms ±20 us | Servo/Timer1 configurados; 5 órdenes verificadas con doble | Captura simultánea de 5 canales en movimiento |
| R8 | 1 kHz ±1% | Cálculo Timer2 =1000 Hz; cola con contador de pérdida | Medir intervalos entre flancos D2, servos activos |
| R9 | UI y LED <=20 ms | Máximo 14 ms de botón en 5 fases simuladas | Capturar botón y selector reales frente al LED |
| R10 | Buck 4.75–5.25 V y 0 reinicios en 100 ciclos | Contador de arranque persistente verificado por código | Osciloscopio + contador antes/después; validar suministro |

## Compilación verificada

Se conservan las dependencias fijadas: Arduino AVR 1.8.6 y Servo 1.2.2. Se compiló con advertencias completas; sólo aparecen parámetros no utilizados del core Arduino. Las pruebas nativas usan C++14, aserciones y advertencias como errores. Las pruebas del analizador y los hashes se incluyen en scripts/test.py.

| Variante | Flash | SRAM global | Remanente para pila/temporales |
| --- | --- | --- | --- |
| Normal, UART y D2 | 14720 / 32256 B | 1379 / 2048 B | 669 B |
| Sin UART ni D2 | 10024 / 32256 B | 806 / 2048 B | 1242 B |

Los logs permanecen en build/, ignorados por Git. Ese remanente no es una medición del máximo de pila ni de consumo eléctrico. No se cargó esta revisión ni se accionaron servos durante la integración. La evidencia real previa se conserva en mediciones/2026-11-09; no valida por sí sola el procesamiento posterior.

## Incidencia abierta del sensor

El AD8232 presenta variabilidad de señal cuya causa sigue sin determinarse. Soldaduras, módulo, jumpers, contactos, alimentación y condiciones de adquisición son hipótesis pendientes; las pruebas de software no las descartan. La estabilidad del sensor y la aceptación física de R6 siguen abiertas. Véase [el diagnóstico pendiente](diagnostico-sensor.md).
