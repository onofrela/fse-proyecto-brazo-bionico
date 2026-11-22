# Matriz de requisitos y evidencia

Esta matriz evalúa los requisitos del proyecto sobre la base Arduino disponible. “Implementado” describe código existente; “simulado” describe entradas sintéticas; “pendiente físico” requiere montar y medir. No se declara cumplimiento de hardware por compilar.

| Criterio solicitado | Evidencia | Estado y límite |
| --- | --- | --- |
| Estructura modular, códigos separados, nombres claros | Tabla del README; drivers y clases de dominio independientes | Implementado |
| Constantes con nombre | `Config.h`; parámetros de memoria/temporización y límites centralizados | Implementado en firmware principal; 0/1 y estructura de bytes conservan su significado convencional |
| Errores en cada llamada al sistema | No hay syscalls en bare-metal. ADC con timeout/lectura de registros, Servo attach y estado comprobados, EEPROM verificada, UART read/write/snprintf comprobados | Implementado para APIs con estado. GPIO/Servo write son void y no pueden confirmar funcionamiento físico; scripts comprueban códigos de salida |
| Comentarios útiles en español | Contratos de cola, propiedad de datos, ISR y D01–D11 | Implementado |
| Pruebas con esperado/obtenido y >=2 fallas | `resultados-pruebas.md`, generado por `scripts/test.py` | Pruebas nativas ejecutadas; incluye recuperación de contacto y pérdida de ADC |
| Historial legible y etiquetas | Serie histórica hasta v0.6.0 y limpieza FW8 en `limpieza-fw8` | Etapas descritas en versiones.md; v1.0.0 pendiente del commit final y etiquetado por el orquestador |
| Diagrama conectado y flujo completo | `arquitectura.md`, E2E-LEVEL y HW-MANUAL | Flujo software integrado y probado con dobles; demostración HW pendiente |
| Comunicación, IPC y sincronización | `SampleQueue.h`, `Acquisition.cpp`, IPC-TIME y LEVEL-SAFETY | Cola compartida SPSC y secciones atómicas; sin hilos/procesos porque el diseño utiliza bare-metal. No cumple literalmente IPC de un SO |
| Inicio al encender | `setup()` → `Application::begin`, E2E-LEVEL | Implementado sin espera USB ni calibración; confirmar desconectando PC en placa |
| Configuración repetible | `bootstrap.sh`, `build.py`, `verify.sh`, versiones del README | Variantes `normal` y `depuracion` compiladas; bootstrap con descarga pendiente en equipo limpio |
| Tiempo / consumo vs compromiso | LAT-FW8, UNIT-01 y protocolo físico | R1 no cumple por diseño: bloques de 200 muestras (~200 ms); latencia simulada detallada abajo; consumo pendiente |
| Tareas con prioridad | `Application::tick`, ISR y tabla de bitácora | Orden cooperativo explícito y prioridades de hardware fijas; no hay prioridades de RTOS |
| Robustez | Contacto, timeout, cola llena, muestras antiguas, EEPROM y UART | Recuperación de fallas de señal simulada; falla de arranque ADC requiere reinicio, falla mecánica no detectable |

## Objetivos de validación

| ID | Compromiso | Resultado disponible | Falta para aceptación |
| --- | --- | --- | --- |
| R1 | <=50 ms a primer PWM modificado, 20 eventos | LAT-FW8: primer destino y primer paso escrito en 161–360 ms con amplitud 12 ADC; 58–257 ms con 40 ADC | **No cumple por diseño**: bloque de 200 muestras (~200 ms). Medir PWM en placa para caracterizar la respuesta, sin declarar cumplimiento |
| R2 | Cierre <=1 s, promedio de 10 cierres | Recorrido ordenado 780 ms en UNIT-01, desde la orden de cierre; no incluye la espera de decisión | Video >=60 fps (las cámaras del equipo solo graban a 30 fps; en video a 30 fps la apertura tarda ≈ 0.8 s) con 4 celdas 18650 y regulador a 5 V; registrar tensión del montaje |
| R3 | 0–150 grados ±5 por dedo | Pulsos configurables 544/2400 us (0°/180° ordenados por solicitud posterior) | Goniómetro, 3 repeticiones por dedo |
| R4 | 2.5 kg durante 60 s; elongación <=2% | Sin prueba mecánica | Ensayo de tensor y nudos |
| R5 | >=90 min; promedio <=1 A a 5 V | Multímetro el 6 y 13 de noviembre de 2026 con la fuente anterior: 0.1 a 0.5 A con 5 V fijos y servos sin respuesta adecuada; sin medida de autonomía para 4 celdas 18650 (2 portapilas en serie, conectados en paralelo: 7.4 V y el doble de capacidad) + regulador | Ciclo cada 10 s, voltaje cada 10 min; definir corte según celdas y protección del montaje; 10.5 V era de la propuesta LiPo 3S |
| R6 | >=90% detección; <=1 falsa/min | Prueba física de aproximadamente 60 contracciones: la mayoría detectadas y 4 fallos por el electrodo (≈ 93 % de detección). Falsas activaciones por minuto sin contar. Tres niveles y recuperación de fallas verificados con entradas sintéticas | Contar falsas activaciones en 5 min de reposo y repetir con 50 contracciones contadas |
| R7 | 20 ms ±1%; pulsos 1–2 ms ±20 us | Servo/Timer1 configurados; extremos actuales 544–2400 us, fuera del intervalo 1–2 ms de este requisito; 5 órdenes verificadas con doble | Captura simultánea de 5 canales en movimiento |
| R8 | 1 kHz ±1% | Cálculo Timer2 =1000 Hz; cola con contador de pérdida | Medir intervalos entre flancos D2, servos activos |
| R9 | Respuesta de botón D8 y selector D9 <=20 ms | Lectura cada 5 ms y antirrebote de 10 ms; UNIT-01 y HW-MANUAL prueban entradas y órdenes; LED RGB retirado | Medir entrada física frente a cambio de orden/salida de servo; caracterizar botón y selector en placa |
| R10 | Buck 4.75–5.25 V y 0 reinicios en 100 ciclos | Riel medido con multímetro: 5.1 V (dentro del intervalo). Contador de arranque persistente verificado por código | Osciloscopio (mínimo al arrancar los servos) + contador antes/después en 100 arranques |

## Compilación verificada

El 2026-11-21 se ejecutaron `python scripts/test.py` (44 casos reportados y 30 pruebas Python sin fallos) y `python scripts/build.py` con opciones locales `--config` y `--servo-library` para ambas variantes. El YAML local mantiene las salidas y cachés dentro del repositorio; las rutas de instalación son propias del equipo. Se conservan Arduino AVR 1.8.6 y Servo 1.2.2. Se compiló con advertencias completas; sólo aparecen parámetros no utilizados del core Arduino. Las pruebas nativas usan C++14, aserciones y advertencias como errores; el informe [resultados-pruebas.md](resultados-pruebas.md) se regeneró con datos sintéticos.

LAT-FW8 recorre las 200 fases del bloque por amplitud con un bucle ideal de 1 ms, reposo ADC 338 y seno de 100 Hz. El retardo parte de la primera muestra ADC de contracción e incluye filtro, promedio y primer paso escrito. Para 12 ADC, los extremos son 161 ms (fase 38) y 360 ms (fase 39); para 40 ADC, 58 ms (fase 141) y 257 ms (fase 142). El destino y el primer paso coinciden en esta simulación. No incluye ISR, PWM ni movimiento en la mano; que LAT-FW8 pase significa que verifica el comportamiento simulado, no que R1 se cumpla.

| Variante | Flash | SRAM global | Remanente para pila/temporales |
| --- | --- | --- | --- |
| `normal`, sin UART y con D2 | 7128 / 32256 B | 508 / 2048 B | 1540 B |
| `depuracion`, con UART y D2 | 11404 / 32256 B | 1002 / 2048 B | 1046 B |

Los logs permanecen en build/, ignorados por Git. Ese remanente no es una medición del máximo de pila ni de consumo eléctrico. No se cargó esta revisión ni se accionaron servos durante la integración. Las capturas reales previas ya no se versionan por tratarse de datos biométricos/de salud; sus resultados se resumen en el informe del Avance 4 y no validan por sí solos el procesamiento posterior. Las pruebas actuales usan el [dato sintético](../tests/datos/README.md).

## Incidencia abierta del sensor

El AD8232 presenta variabilidad de señal cuya causa sigue sin determinarse. Soldaduras, módulo, jumpers, contactos, alimentación y condiciones de adquisición son hipótesis pendientes; las pruebas de software no las descartan. La estabilidad del sensor y la aceptación física de R6 siguen abiertas. Véase [el diagnóstico pendiente](diagnostico-sensor.md).
