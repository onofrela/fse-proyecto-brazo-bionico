# Bitácora técnica de modularización

Fecha de actualización: 2026-11-21 (America/Mexico_City). Registro de decisiones técnicas y verificaciones de software. Las mediciones físicas y revisiones del equipo se documentan por separado.

Plataforma: Arduino UNO R3, arquitectura bare-metal e interrupciones. La revisión considera modularidad, comunicación, temporización, persistencia y evidencia de pruebas.

## Retiro de calibración y control ponderado en FW8

**D11: retirar la calibración y usar el promedio de la tercera señal.** Esta decisión sustituye D03/D04/D08/D09 en lo relativo a calibración, umbrales persistidos y control ponderado; esas entradas se conservan como historial.

La calibración guiada solía fallar porque la señal se mezclaba con los latidos del corazón. El equipo encontró una referencia más clara en la tercera señal del analizador, la envolvente, que sigue el movimiento de la mano: valores de 1 a 4 en reposo, de 5 a 9 con el músculo medio tenso y de 10 en adelante con la mano tensada. Estas observaciones motivan la decisión; este cambio no aporta nuevas mediciones físicas.

FW8 usa esa referencia con bandas fijas: `ThreeLevelControl` promedia bloques de 200 muestras y ordena 0°/90°/180° con fronteras en 5 y 10, sin calibración ni ponderación temporal. Se retiraron `Calibration.h`, `MuscleSwitch`, la fachada `Control.h`, `CalibrationStore.h`, `Storage::save/poll`, sus constantes y pruebas históricas. El filtrado conserva `BASELINE_*`; EEPROM solo guarda y verifica el contador de arranque y su inverso, antes de habilitar ADC. Los umbrales antiguos se ignoran.

El costo aceptado es la latencia: a 1 kHz, cada bloque de 200 muestras representa aproximadamente 200 ms. **R1 (≤ 50 ms) ya no se cumple por diseño**. LAT-FW8 recorre 200 fases por amplitud con ADC sintético, filtro, promedio y rampa: primer destino y primer paso escrito en 161–360 ms con amplitud 12 ADC y 58–257 ms con 40 ADC. Usa reposo ADC 338, seno de 100 Hz y bucle de 1 ms; las pruebas de software no sustituyen una medición física de ISR, PWM o respuesta de la mano.

Se retiró el LED RGB y su código; D7/D12/D13 quedan libres. El montaje final usa 4 celdas 18650 para los servos MG995 (dos portapilas de dos celdas en serie (7.4 V nominales cada uno), conectados entre sí en paralelo: 7.4 V nominales y el doble de capacidad) y una pila de 9 V para el Arduino; la LiPo 3S propuesta no se consiguió y la fuente anterior (5 V fijos, 0.1 a 0.5 A) no movía bien los servos. La variante `normal` no inicia, lee ni escribe UART; `depuracion` conserva diagnóstico y comandos, con telemetría periódica apagada hasta `p`.

Las tablas D01–D10 documentan etapas anteriores; sus referencias a calibración y control ponderado son históricas. La planificación y las cifras siguientes se sincronizan con FW8 cuando se indica.

## Retiro de capturas biométricas del repositorio público

Las capturas reales del AD8232 son datos biométricos/de salud. Por decisión del equipo se elimina todo `docs/mediciones/` del árbol actual, sin reescribir el historial: las revisiones anteriores siguen conteniendo esos datos. Sus resultados se resumen en el informe del Avance 4. En el paso 8, las nuevas sesiones pasan a `Path.home() / "registros-emg"`, fuera del repositorio; se conserva la exclusión de `registros/`, las rutas anteriores y los nombres de CSV/JSON/PNG que produce el analizador.

El ECG puede identificar a la persona y el repositorio es público. El analizador requiere consentimiento explícito por captura (`--acepto-guardar` en headless), usa identificadores aleatorios y omite hora absoluta salvo `--incluir-hora`. Cifra señal, eventos y resumen con AES-256-GCM y scrypt a partir de una contraseña por sesión que no persiste. Cada archivo tiene sal/nonce propios; un manifiesto HMAC-SHA256 autentica los tres archivos y su lista. Los comandos de verificación y descifrado rechazan contraseñas incorrectas y alteraciones. La excepción `--sin-cifrar` advierte y mantiene integridad autenticada. Se retira el PNG automático para evitar copias en claro. Se ofrece borrar sesiones sin cambios por más de 30 días, siempre tras confirmar y mostrando las rutas. Las pruebas de este cambio usan telemetría sintética; no añaden mediciones físicas ni actualizan el informe histórico de pruebas.

Las pruebas usan ahora `tests/datos/senal_sintetica.csv`: 60 filas generadas por una fórmula determinista, sin capturas como entrada. REPLAY-30S ejercita decisión y rampa con retención artificial durante 30 s simulados; PC-DATOS verifica la lectura de las 22 columnas. EVIDENCIA se conserva para comprobar el origen sintético, filas, columnas, periodo y SHA256 del manifiesto. Se eliminó `scripts/verificar_mediciones.py` porque su verificación de originales ya no tiene entradas versionadas. Ninguno de estos casos representa una medición física ni acredita R1 o R6. En el paso 12 se regeneró [resultados-pruebas.md](resultados-pruebas.md) con `python scripts/test.py`: 44 casos reportados y 30 pruebas Python sin fallos.

## Decisiones de modularización anteriores

| Decisión y evidencia en código | Planeado / base | Construido y motivo |
| --- | --- | --- |
| D01: `Config.h`, clases de lógica | UNO, 32 KB Flash y 2048 B SRAM; memoria estática | Se conserva bare-metal. Se separan señal, calibración, decisión, entradas y movimiento. Cada parámetro de operación tiene nombre. No se introduce RTOS ni memoria dinámica |
| D02: `SampleQueue.h`, `Acquisition.cpp` | El diseño utiliza un búfer circular con índices. La base filtraba dentro de la ISR y publicaba solo el último resultado | Cola SPSC de 32 posiciones, 31 útiles; 7 bytes por muestra en AVR (tiempo 4, ADC 2, contacto 1), 224 B de entradas. Filtrado en el bucle. Acceso consumidor atómico; alarma al llenarse. Se recupera el mecanismo elegido y se reduce trabajo de ISR |
| D03: `CalibrationStore.h`, `Storage.cpp` | EEPROM sin sistema de archivos; la base escribía la calibración completa sin verificación | Guardado en 10 pasos de un byte: invalidar firma (2), datos/checksum (6), confirmar firma (2). Se verifica cada byte antes de avanzar; solo escribir si EEPROM está lista. El contador se verifica antes de arrancar ADC. Un registro parcial se rechaza |
| D04: `Application::begin` | Arranque autónomo sin Linux | Se inicia sin esperar USB. Una calibración guardada permite operar; sin ella se mantiene abierto hasta c o D8 al encender. La medición deja de comenzar por sorpresa |
| D05: `Application::consume` | ADC 1 kHz, filtro de 32 muestras y calibración | Se usa la hora de adquisición, no la de extracción. Datos con edad >10 ms, huecos >10 ms y pérdidas invalidan la ventana; se exigen 2000 muestras estables. Se evita que una lectura vieja parezca reciente |
| D06: `Application::tick`, ISR comentadas | Prioridades relativas, muestreo/PWM por hardware e interfaz no bloqueante | Seguridad y muestras primero; luego UI/comandos, rampa/LED, EEPROM incremental y UART. Máximo 31 muestras y un comando por vuelta. ISR sin filtrado, texto ni EEPROM. Se conservan Timer0 y Timer1 |

## Prioridad y concurrencia reales

La tabla describe la planificación vigente de FW8.

| Tarea | Periodicidad o disparo | Planificación implementada |
| --- | --- | --- |
| Solicitud ADC | Timer2, 1000 Hz nominales | ISR de hardware; comprueba ADC ocupado/pendiente |
| Captura ADC | Fin de conversión, 1000 Hz nominales | ISR de hardware; publica una muestra o alarma de pérdida |
| Pulsos de cinco servos | Tramas nominales de 20 ms | ISR Timer1 de Servo 1.2.2 |
| Procesamiento / seguridad | Hasta 31 muestras por vuelta | Primera tarea cooperativa; fallas ordenan apertura en EMG |
| UI | Lectura cada 5 ms, debounce 10 ms | Después de adquisición; sin tarea de LED |
| Rampa | Paso de 25/1000 cada 20 ms | Primer paso inmediato, sin ráfagas para recuperar retrasos |
| Contador de arranque | Solo al encender | Escritura y verificación de contador/inverso antes de habilitar ADC; sin tarea EEPROM en el bucle |
| UART, solo en depuración | Un comando por vuelta; datos cada 100 ms si se habilitan con `p` | Última prioridad, descarta líneas si no hay capacidad; deshabilitada en `normal` |

El ATmega328P tiene orden fijo de vectores; las ISR usadas no habilitan anidamiento. Una ISR en ejecución no es interrumpida por otra. El orden de Timer2/Timer1/ADC en eventos pendientes no equivale a la prioridad ideal de las tareas del diseño. La garantía temporal debe contrastarse con R7/R8 en hardware; no se implementa un planificador de prioridades preemptivas ni se afirma que exista.

No hay procesos, hilos, syscalls ni IPC de SO. La comunicación que reemplaza el IPC es memoria compartida mediante cola circular y alarmas atómicas. El mecanismo permite comunicación y sincronización dentro del firmware; no implementa procesos, hilos ni servicios de un sistema operativo. Añadirlos requeriría revisar la plataforma y el alcance.

## Cifras y desviaciones

- UI pasa de la propuesta 10/20 ms a muestreo 5 ms y debounce 10 ms para R9 <=20 ms. El ensayo histórico de botón obtuvo un máximo de 14 ms en cinco fases; el informe actual prueba antirrebote y órdenes manuales, sin repetir aquella medición por fases. R9 ya no incluye LED; falta medir botón y selector físicos frente a la orden/salida de servo.
- El recorrido ordenado de R2 resulta 780 ms frente a <=1000 ms en el modelo (UNIT-01); no mide las falanges ni incluye la espera de decisión. La referencia de latencia vigente es LAT-FW8, descrita en D11; sustituye las cifras del control ponderado.
- El AD8232 entrega señal ECG acondicionada, no una envolvente muscular lista. Se incorpora SignalFilter entre centrado y rectificación, manteniendo la cola y las ISR breves. El ensayo R1 original debe redefinir la referencia de disparo para esta tarjeta y R6 debe demostrar separación de actividad muscular/ECG/artefactos. Alimentación corregida a 3.3 V.
- El muestreo nominal se calcula como 16 MHz / 64 / 250 = 1000 Hz. D2 da 1000 flancos/s y una onda de 500 Hz. El periodo y jitter no se han medido.
- La compilación AVR de ambas variantes, ejecutada el 2026-11-21 con `scripts/build.py`, usa 7128 B Flash / 508 B SRAM en `normal` y 11404 B Flash / 1002 B SRAM en `depuracion`, con D2 habilitado. Quedan 1540 y 1046 B para pila/temporales; no es una medición de máximo uso de pila ni de consumo. Solo hubo advertencias por parámetros no utilizados del core Arduino. Véase la matriz de requisitos.
- LEVEL-SAFETY recupera control sin reset en simulación ante contacto, timeout y cola llena. El F2 original (dedo bloqueado) requiere carga mecánica y medición del riel; no se detecta bloqueo porque no existe sensor de corriente, fuerza o posición.
- R5 exige >=90 min y presupuesto <=1 A promedio a 5 V. No se dispone de medición de consumo o voltaje de batería: queda pendiente; no se deduce autonomía de la Flash ni del tiempo de CPU.
- Los commits y etiquetas documentan la separación de responsabilidades y las etapas de integración.

## Trabajo pendiente y validación física

La implementación incluye pruebas de sincronización y comunicación (cola y alarmas), orden cooperativo de tareas y arranque autónomo. Las verificaciones del equipo deben registrarse con fecha, autor, revisión y evidencia.

Pendientes de validación: cargar y probar la versión etiquetada; ejecutar el protocolo físico; capturar R1/R7/R8/R9 con servos activos; medir R2/R3/R4/R5/R6/R10; completar cableado y tensores; registrar consumo, voltaje, reinicios y fallas reales; preparar el informe técnico con la evidencia de validación. Véase `pruebas-hardware.md`.

## Historial detallado y apoyos

El historial de `main` separa los cambios por responsabilidad. Los sketches existentes de servo y ADC se versionan como herramientas independientes de apoyo; su compilacion se verifico, pero no se agregan resultados fisicos inventados. La evolucion de cada modulo y su documentacion se registra en `modularizacion-paso-a-paso.md`.

## Ajuste con evidencia del sensor

| Decisión | Evidencia y resultado |
| --- | --- |
| D07: SignalFilter separado | HP30, rechazo 60 Hz y LP200; enteros Q12/Q6; respuesta sintética y reset probados. Sin validación de eliminación completa de ECG en placa |
| D08: Calibración explícita y robusta | Las capturas iniciales muestran desajuste de ventanas. Se guía 3/6/3/10 s, con histogramas y diagnóstico; Storage conserva su transacción incremental |
| D09: Mano completa ponderada | Las amplitudes por dedo se superponen. Se usa actividad/reposo con suavizado, cierre mínimo de 900 ms y apertura prioritaria por falla |
| D10: Evidencia original separada (histórica) | Se conservaron CSV reales, eventos, gráficas y capturas con SHA256. Ya no se versionan por ser datos biométricos/de salud; sus resultados se resumen en el informe del Avance 4. Los replays actuales usan datos sintéticos y no validan físicamente el firmware |

En aquella revisión, la ponderación de un replay de envolventes llegó al recorrido ordenado completo y se requería recalibrar con firma 0xE706. Esa evidencia no mide fuerza, movimiento físico, consumo ni respuesta real del filtro analógico. FW8 sustituyó ese método y no usa calibración ni su firma.

## Incidencia abierta: volatilidad del sensor

El módulo presenta señal muy variable y falta identificar la causa. Se investigan soldaduras, posibles daños del módulo, jumpers/contactos, alimentación y condiciones de adquisición. Son hipótesis, no fallas confirmadas. Se documentará cada cambio y su efecto sin considerar que un ajuste de filtros o umbrales repare el montaje. Véase [el plan de diagnóstico](diagnostico-sensor.md).
