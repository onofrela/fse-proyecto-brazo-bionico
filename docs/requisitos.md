# Matriz de requisitos y evidencia

Esta matriz evalúa los requisitos del proyecto sobre la base Arduino disponible. “Implementado” describe código existente; “simulado” describe entradas sintéticas; “pendiente físico” requiere montar y medir. No se declara cumplimiento de hardware por compilar.

| Criterio solicitado | Evidencia | Estado y límite |
| --- | --- | --- |
| Estructura modular, códigos separados, nombres claros | Tabla del README; drivers y clases de dominio independientes | Implementado |
| Constantes con nombre | `Config.h`; parámetros de memoria/temporización y límites centralizados | Implementado en firmware principal; 0/1 y estructura de bytes conservan su significado convencional |
| Errores en cada llamada al sistema | No hay syscalls en bare-metal. ADC con timeout/lectura de registros, Servo attach y estado comprobados, EEPROM verificada, UART read/write/snprintf comprobados | Implementado para APIs con estado. GPIO/Servo write son void y no pueden confirmar funcionamiento físico; scripts comprueban códigos de salida |
| Comentarios útiles en español | Contratos de cola, propiedad de datos, ISR y D01–D06 | Implementado |
| Pruebas con esperado/obtenido y >=2 fallas | `resultados-pruebas.md`, generado por `scripts/test.py` | Pruebas nativas ejecutadas; incluye recuperación de contacto y pérdida de ADC |
| Historial legible y etiquetas | `git log --oneline --decorate`, base v0.1.0 y etapas v0.5.0-modulos/integracion/pruebas/final | Commits separados por modulo en main y versiones preliminares conservadas mediante etiquetas |
| Diagrama conectado y flujo completo | `arquitectura.md`, E2E-01 y BOOT-01 | Flujo software integrado y probado con dobles; demostración HW pendiente |
| Comunicación, IPC y sincronización | `SampleQueue.h`, `Acquisition.cpp`, IPC-01 | Cola compartida SPSC y secciones atómicas; sin hilos/procesos porque el diseño utiliza bare-metal. No cumple literalmente IPC de un SO |
| Inicio al encender | `setup()` → `Application::begin`, BOOT-01 | Implementado sin espera USB; confirmar desconectando PC en placa |
| Configuración repetible | `bootstrap.sh`, `build.py`, `verify.sh`, versiones del README | Compilación de firmware y variante sin UART/sonda verificadas; bootstrap con descarga pendiente en equipo limpio |
| Tiempo / consumo vs compromiso | R1-SW, R2-SW y R9-SW; protocolo de R1–R10 | Tiempos lógicos contrastados; medición física y consumo pendientes |
| Tareas con prioridad | `Application::tick`, ISR y tabla de bitácora | Orden cooperativo explícito y prioridades de hardware fijas; no hay prioridades de RTOS |
| Robustez | Contacto, timeout, cola llena, muestras antiguas, EEPROM y UART | Recuperación de fallas de señal simulada; falla de arranque ADC requiere reinicio, falla mecánica no detectable |

## Objetivos de validación

| ID | Compromiso | Resultado disponible | Falta para aceptación |
| --- | --- | --- | --- |
| R1 | <=50 ms a primer PWM modificado, 20 eventos | 12 ms de escalón sintético a orden, sin tren PWM ni filtro analógico | Definir disparo para AD8232 y medir los 20 eventos |
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

Herramientas locales: Arduino CLI 1.1.1, AVR 1.8.6, Servo 1.2.2 y EEPROM 2.0. `scripts/build.py` comprueba las versiones del core y Servo. La compilación con `--warnings all` muestra advertencias de parámetros no usados en `new.cpp` del core Arduino; no se observaron advertencias del código del proyecto. Las pruebas nativas MSVC usan `/W4 /WX` y aserciones activas.

| Variante verificada | Flash | Globales SRAM | Remanente para pila/temporales |
| --- | --- | --- | --- |
| Normal (UART y D2) | 11622 B / 32256 B | 966 B / 2048 B | 1082 B |
| Sin UART ni sonda D2 | 7534 B / 32256 B | 478 B / 2048 B | 1570 B |

Los logs se generan en `build/uno.txt` y `build/uno-sin-uart-sin-sonda.txt` (ignorados por Git). Flash y SRAM no son medidas de consumo eléctrico; el remanente de SRAM no garantiza margen de pila bajo todas las condiciones.
