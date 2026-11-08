# Bitácora técnica de modularización

Fecha de actualización: 2026-11-08 (America/Mexico_City). Registro de decisiones técnicas y verificaciones de software. Las mediciones físicas y revisiones del equipo se documentan por separado.

Plataforma: Arduino UNO R3, arquitectura bare-metal e interrupciones. La revisión considera modularidad, comunicación, temporización, persistencia y evidencia de pruebas.

| Decisión y evidencia en código | Planeado / base | Construido y motivo |
| --- | --- | --- |
| D01: `Config.h`, clases de lógica | UNO, 32 KB Flash y 2048 B SRAM; memoria estática | Se conserva bare-metal. Se separan señal, calibración, decisión, entradas y movimiento. Cada parámetro de operación tiene nombre. No se introduce RTOS ni memoria dinámica |
| D02: `SampleQueue.h`, `Acquisition.cpp` | El diseño utiliza un búfer circular con índices. La base filtraba dentro de la ISR y publicaba solo el último resultado | Cola SPSC de 32 posiciones, 31 útiles; 7 bytes por muestra en AVR (tiempo 4, ADC 2, contacto 1), 224 B de entradas. Filtrado en el bucle. Acceso consumidor atómico; alarma al llenarse. Se recupera el mecanismo elegido y se reduce trabajo de ISR |
| D03: `CalibrationStore.h`, `Storage.cpp` | EEPROM sin sistema de archivos; la base escribía la calibración completa sin verificación | Guardado en 10 pasos de un byte: invalidar firma (2), datos/checksum (6), confirmar firma (2). Se verifica cada byte antes de avanzar; solo escribir si EEPROM está lista. El contador se verifica antes de arrancar ADC. Un registro parcial se rechaza |
| D04: `Application::begin` | Arranque autónomo sin Linux | `setup()` inicia módulos al encender, sin `while (!Serial)`. EEPROM válida evita recalibrar; sin ella se solicita la secuencia con la mano abierta. La interacción para calibrar es necesaria, pero el programa inicia sin PC |
| D05: `Application::consume` | ADC 1 kHz, filtro de 32 muestras y calibración | Se usa la hora de adquisición, no la de extracción. Datos con edad >10 ms, huecos >10 ms y pérdidas invalidan la ventana; se exigen 2000 muestras estables. Se evita que una lectura vieja parezca reciente |
| D06: `Application::tick`, ISR comentadas | Prioridades relativas, muestreo/PWM por hardware e interfaz no bloqueante | Seguridad y muestras primero; luego UI/comandos, rampa/LED, EEPROM incremental y UART. Máximo 31 muestras y un comando por vuelta. ISR sin filtrado, texto ni EEPROM. Se conservan Timer0 y Timer1 |

## Prioridad y concurrencia reales

| Tarea | Periodicidad o disparo | Planificación implementada |
| --- | --- | --- |
| Solicitud ADC | Timer2, 1000 Hz nominales | ISR de hardware; comprueba ADC ocupado/pendiente |
| Captura ADC | Fin de conversión, 1000 Hz nominales | ISR de hardware; publica una muestra o alarma de pérdida |
| Pulsos de cinco servos | Tramas nominales de 20 ms | ISR Timer1 de Servo 1.2.2 |
| Procesamiento / seguridad | Hasta 31 muestras por vuelta | Primera tarea cooperativa; fallas ordenan apertura en EMG |
| UI / LED | Lectura cada 5 ms, debounce 10 ms; LED cada vuelta | Después de adquisición; pruebas con bucle ideal de 1 ms |
| Rampa | Paso de 25/1000 cada 20 ms | Primer paso inmediato, sin ráfagas para recuperar retrasos |
| Persistencia de calibración | A petición tras calibrar | Un paso si EEPROM lista, con verificación posterior |
| UART | Un comando por vuelta; datos cada 100 ms | Última prioridad, descarta líneas si no hay capacidad |

El ATmega328P tiene orden fijo de vectores; las ISR usadas no habilitan anidamiento. Una ISR en ejecución no es interrumpida por otra. El orden de Timer2/Timer1/ADC en eventos pendientes no equivale a la prioridad ideal de las tareas del diseño. La garantía temporal debe contrastarse con R7/R8 en hardware; no se implementa un planificador de prioridades preemptivas ni se afirma que exista.

No hay procesos, hilos, syscalls ni IPC de SO. La comunicación que reemplaza el IPC es memoria compartida mediante cola circular y alarmas atómicas. El mecanismo permite comunicación y sincronización dentro del firmware; no implementa procesos, hilos ni servicios de un sistema operativo. Añadirlos requeriría revisar la plataforma y el alcance.

## Cifras y desviaciones

- UI pasa de la propuesta 10/20 ms a muestreo 5 ms y debounce 10 ms para ajustarse a R9 <=20 ms. En el ensayo simulado de botón, las cinco fases producen como máximo 14 ms; falta osciloscopio y selector físico.
- El recorrido ordenado de R2 resulta 780 ms frente a <=1000 ms en el modelo; no mide las falanges. El primer paso frente a un escalón sintético resulta 12 ms; no es la latencia física de R1.
- El AD8232 entrega señal ECG acondicionada, no una envolvente muscular lista. Se conserva la adaptación de la base: extracción DC/rectificación/ventana. El ensayo R1 original debe redefinir la referencia de disparo para esta tarjeta y R6 debe demostrar separación de actividad muscular/ECG/artefactos. Alimentación corregida a 3.3 V.
- El muestreo nominal se calcula como 16 MHz / 64 / 250 = 1000 Hz. D2 da 1000 flancos/s y una onda de 500 Hz. El periodo y jitter no se han medido.
- La memoria se verifica con compilación AVR en la matriz de requisitos. El margen de SRAM publicado es para pila/temporales; no es una medición de máximo uso de pila.
- Los casos F1-SW/F2-SW recuperan control sin reset en simulación. El F2 original (dedo bloqueado) requiere carga mecánica y medición del riel; no se detecta bloqueo porque no existe sensor de corriente, fuerza o posición.
- R5 exige >=90 min y presupuesto <=1 A promedio a 5 V. No se dispone de medición de consumo o voltaje de batería: queda pendiente; no se deduce autonomía de la Flash ni del tiempo de CPU.
- Los commits y etiquetas documentan la separación de responsabilidades y las etapas de integración.

## Trabajo pendiente y validación física

La implementación incluye pruebas de sincronización y comunicación (cola y alarmas), orden cooperativo de tareas y arranque autónomo. Las verificaciones del equipo deben registrarse con fecha, autor, revisión y evidencia.

Pendientes de validación: cargar y probar la versión etiquetada; ejecutar el protocolo físico; capturar R1/R7/R8/R9 con servos activos; medir R2/R3/R4/R5/R6/R10; completar cableado y tensores; registrar consumo, voltaje, reinicios y fallas reales; preparar el informe técnico con la evidencia de validación. Véase `pruebas-hardware.md`.

## Historial detallado y apoyos

El historial de `main` separa los cambios por responsabilidad. Los sketches existentes de servo y ADC se versionan como herramientas independientes de apoyo; su compilacion se verifico, pero no se agregan resultados fisicos inventados. La evolucion de cada modulo y su documentacion se registra en `modularizacion-paso-a-paso.md`.
