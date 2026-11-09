# Mano InMoov con AD8232 (ECG, 3.3 V)

Adaptación de `Inmoov_Robot_Hand.ino` de Kuldip.K Gami al proyecto de Fundamentos de Sistemas Embebidos. Los archivos originales `.ino` y `Robotic Hand.txt` contienen el mismo programa. Se retiraron `SoftwareSerial`, los comandos Bluetooth y los movimientos con `delay()`.

La implementación conserva la plataforma y los pines de actuadores del proyecto: Arduino UNO R3, ATmega328P a 16 MHz y cinco servos. El sensor del proyecto ahora es el **AD8232 de UNIT Electronics**. Se añaden A1 y A2 para detectar pérdida de contacto. Un solo canal de actividad abre/cierra la mano completa; no identifica dedos individuales.

La conexión de alimentación de un EMG genérico al riel de 5 V **no corresponde a este AD8232**.

## Código modular

El punto de entrada solo llama a `Application::begin()` y `Application::tick()`.
Los comentarios `D01` a `D06` enlazan decisiones concretas con [la bitácora técnica](docs/bitacora.md).

| Módulo | Responsabilidad |
| --- | --- |
| `Config.h` | Pines, tiempos, límites y direcciones EEPROM con nombre |
| `Acquisition.h/.cpp` | Timer2/ADC, ISR breve, cola circular y secciones atómicas |
| `SampleQueue.h` | Comunicación SPSC de muestras completas con marca de tiempo |
| `Signal.h` | Nivel DC, rectificación, media móvil y estabilización |
| `Decision.h` | Umbrales, histéresis y rearme en reposo |
| `Calibration.h` | Máquina de estados para medir reposo y contracción |
| `Motion.h` | Rampa simultánea y conversión a pulsos |
| `Inputs.h` | Antirrebote de botón y selector |
| `Outputs.h/.cpp` | GPIO, RGB y cinco servos; deshace un arranque parcial |
| `Storage.h/.cpp`, `CalibrationStore.h` | EEPROM verificada y guardado incremental |
| `Diagnostics.h/.cpp` | Comandos y telemetría UART de baja prioridad |
| `Application.h/.cpp` | Coordinación de módulos y recuperación de fallas |
| `Control.h` | Fachada de compatibilidad de la lógica portable |
| `tests/`, `scripts/` | Pruebas con fallas inducidas y compilación reproducible |

[Diagrama conectado](docs/arquitectura.md) · [Matriz de requisitos](docs/requisitos.md) · [Resultados esperados y obtenidos](docs/resultados-pruebas.md) · [Protocolo físico](docs/pruebas-hardware.md).

## Compatibilidad del sensor y alimentación

**Alimenta el módulo AD8232 a 3.3 V; no a 5 V, 9 V ni a la LiPo directamente.** Conecta su `GND` a la tierra común y `OUTPUT` a A0. Su rango nominal de salida queda dentro de 0–3.3 V. El UNO conserva AVcc, nominalmente 5 V, como referencia ADC: la salida ocupará aproximadamente 0–675 cuentas de sus 10 bits. No hace falta conectar AREF a 3.3 V.

`LO+` y `LO−` se conectan a A1/A2 como **entradas sin pull-up de 5 V**. Un nivel alto en cualquiera inhibe el control por señal. `SDN` se puede mantener a 3.3 V para habilitar el módulo; no se maneja con una salida de 5 V del UNO. Los terminales RA/LA/RL corresponden a los cables de electrodos, no a alimentación ni GND.

**El AD8232 es un acondicionador ECG, no un sensor que entregue una envolvente EMG.** El [fabricante Analog Devices](https://www.analog.com/en/products/ad8232.html) explica que su banda y ganancia dependen de componentes externos. No se conoce el esquema ni la banda exacta de esta tarjeta UNIT; no se atribuyen a ella los valores de otros módulos. El control de contracciones con esta tarjeta es **experimental**: el firmware calcula una medida de actividad, pero no distingue actividad muscular voluntaria de ECG o artefactos de movimiento. El software no recupera frecuencias eliminadas por el filtro analógico. Si la calibración no logra separar reposo y contracción de forma repetible, hay que revisar el acondicionamiento físico o usar un módulo EMG apropiado; bajar el umbral sin medir no resuelve esa limitación.

Referencias eléctricas: [ficha AD8232, revisión D](https://www.analog.com/media/en/technical-documentation/data-sheets/ad8232.pdf) y [guía de conexión de SparkFun](https://learn.sparkfun.com/tutorials/ad8232-heart-rate-monitor-hookup-guide/). La ficha del circuito integrado no sustituye el esquema de la tarjeta UNIT.

Alimenta los servos desde una fuente/buck dimensionada para sus corrientes, con GND común; no desde el pin 5V ni el USB del Arduino. La propuesta de LiPo 3S y buck requiere validar corriente, regulación y autonomía en el montaje. Con electrodos en el cuerpo, usa alimentación por batería sin conexiones a la red, o aislamiento apropiado para la interfaz de depuración e instrumentos. La [guía del fabricante MyoWare](https://cdn.sparkfun.com/assets/learn_tutorials/1/9/5/6/MyoWare_v2_QuickStartGuide.pdf) explica esta precaución para sistemas de electrodos conectados a USB.

## Cableado de señales

| Componente | Pin UNO | Conexión/configuración |
| --- | --- | --- |
| AD8232 `3.3V` | 3.3V | Alimentación del módulo, nunca riel de servos de 5 V |
| AD8232 `GND` | GND | Tierra común de señal |
| AD8232 `OUTPUT` | A0 | Señal con nivel DC; ADC0/AVcc, 10 bits |
| AD8232 `LO+` | A1 | Pérdida de contacto, activo en alto, `INPUT` |
| AD8232 `LO−` | A2 | Pérdida de contacto, activo en alto, `INPUT` |
| AD8232 `SDN` | 3.3V | Mantener habilitado, sin GPIO de 5 V |
| Servo pulgar | D3 | Señal del servo |
| Servo índice | D5 | Señal del servo |
| Servo medio | D6 | Señal del servo |
| Servo anular | D10 | Señal del servo |
| Servo meñique | D11 | Señal del servo |
| Pulsador | D8 | Entre D8 y GND, `INPUT_PULLUP` |
| Selector | D9 | GND = manual; abierto = EMG, `INPUT_PULLUP` |
| RGB rojo | D7 | Resistencia individual de 220 ohm |
| RGB verde | D12 | Resistencia individual de 220 ohm |
| RGB azul | D13 | Resistencia individual de 220 ohm |
| Prueba de muestreo | D2 | Salida opcional; un flanco por conversión ADC |
| Depuración USB | D0/D1 | UART a 115200 baud; sin módulo Bluetooth |

RGB de cátodo común por defecto, común a GND. Para ánodo común cambia `RGB_COMMON_ANODE` en `Config.h` y conecta el común según el LED. La tabla sustituye los pines de servo D2–D6 del código original: el proyecto usa D3/D5/D6/D10/D11.

La guía genérica de SparkFun conecta LO+/LO− a D10/D11; aquí esos pines ya controlan servos y **se usan A1/A2 para LO+/LO−**. No conectes ambas funciones a los mismos pines.

## Carga y primera prueba

1. Abre `Inmoov_EMG/Inmoov_EMG.ino` en Arduino IDE. Instala la biblioteca **Servo** y selecciona **Arduino Uno** y el puerto correcto. EEPROM viene con el núcleo AVR.
2. Ajusta `OPEN_US` y `CLOSED_US` en `Config.h` con los tensores sueltos. Los valores iniciales son 1000 y 2000 microsegundos para cada servo. Puedes invertir los extremos individualmente si un dedo gira al revés.
3. Carga el programa y verifica primero el modo manual: D9 a GND y una pulsación en D8 para cerrar; otra para abrir. Mantener pulsado no repite la orden. Un cambio de modo ordena abrir la mano.
4. Conecta OUTPUT y ambas salidas LO. En el primer arranque, si no hay calibración válida, se solicita automáticamente la secuencia siguiente. Espera primero al menos 2 s continuos de contacto válido y señal sin recorte. Puedes cancelar la solicitud con una pulsación o cambiando el selector para probar en manual.

Los pulsos de 1000/2000 us **no equivalen automáticamente a 0°/150°**. El ángulo real depende del servo, polea y tensores; debe medirse y ajustarse mecánicamente como indica R3. Se usan `writeMicroseconds()` y límites por dedo para no confundir grados con ancho de pulso.

## Calibración EMG

La mano se abre antes de medir y permanece abierta durante la calibración. Se calibra la **envolvente calculada**, no el nivel medio de OUTPUT. Una señal quieta alrededor de 1.65 V no se interpreta por sí sola como contracción.

Al reconectar electrodos o salir de un recorte, el procesamiento espera 2000 muestras consecutivas sin falla antes de habilitar mediciones. Es un tiempo de espera inicial configurable, no una garantía de que el filtro analógico de la tarjeta ya haya terminado de estabilizarse.

| LED | Duración | Acción |
| --- | --- | --- |
| Azul intermitente | 2 s | Prepararse y relajar el músculo |
| Azul fijo | 3 s | Mantener reposo: se mide el nivel base y su máximo |
| Cian intermitente | 2 s | Preparar una contracción sostenida |
| Cian fijo | 3 s | Mantener una contracción cómoda y estable |

Si la diferencia media de envolvente entre contracción y reposo es menor que `MIN_CALIBRATION_RANGE` (12 cuentas por defecto, unos 59 mV con AVcc=5 V), hay pérdida de contacto, recorte, ruido excesivo o faltan muestras, se rechaza la calibración. Los umbrales de apertura y cierre quedan al 25% y 50% de la diferencia medida sobre la envolvente media de reposo. La apertura debe superar también el máximo de reposo en más de `REST_NOISE_MARGIN` (2 cuentas por defecto). Estos valores son parámetros iniciales de software; deben validarse con la tarjeta real.

Una calibración correcta se guarda en EEPROM; `EEPROM_SAVED` confirma que se verificó. Se conserva el formato AD8232 `0xE702` de la base: las calibraciones válidas de esa base siguen siendo compatibles. Los registros del sensor anterior (`0xE701`) se rechazan. Al entrar al modo de señal, primero hay que relajar por debajo del umbral de apertura para habilitar el primer cierre. Después, una envolvente por encima del umbral de cierre cierra la mano; por debajo del umbral de apertura la abre. Entre ambos umbrales conserva el estado para evitar oscilaciones por ruido.

Para recalibrar, envía `c` por el monitor serie, o mantén D8 presionado al encender/reiniciar y suéltalo al comenzar la secuencia. Conviene recalibrar si cambian electrodos, colocación o ganancia. Durante una recalibración fallida se conserva la anterior en EEPROM, pero EMG queda inhibido en esa sesión hasta repetir con éxito o cambiar de modo. El control manual sigue disponible.

Indicadores de operación: rojo = manual abierto; amarillo = manual cerrando/cerrado; verde = control por señal habilitado. Magenta intermitente indica calibración fallida, pérdida de contacto, recorte o espera de estabilización en modo de señal/calibración. Las pulsaciones y cambios de modo aceptados actualizan el indicador en la misma vuelta del bucle.

## Temporización y diagnóstico

- **Timer2:** CTC, prescaler 64, OCR2A=249; solicita una conversión cada 1 ms. Si el ADC sigue ocupado o su resultado no fue recogido, registra pérdida.
- **ADC:** A0/AVcc, prescaler 128, reloj ADC de 125 kHz. La ISR publica ADC, LO+/LO− y `millis()` en una cola circular estática de 32 posiciones (31 útiles). La primera conversión tiene timeout de 1000 us; se comprueba la configuración por lectura de registros.
- **Sincronización:** productor ISR y consumidor `loop`; extracción y alarmas protegidas con `ATOMIC_RESTORESTATE`. `volatile` da visibilidad, la sección crítica da atomicidad. No existen procesos ni hilos de SO en este diseño bare-metal.
- **Procesamiento en el bucle:** IIR del nivel DC (`base += (raw - base)/256`, precisión Q8), magnitud `abs(raw-base)` y media móvil de 32 magnitudes. Se procesa cada muestra una sola vez, hasta 31 por vuelta. La ISR no filtra ni escribe UART/EEPROM.
- **Timer1:** reservado a Servo para cinco trenes de pulsos de aproximadamente 50 Hz. Timer0 conserva `millis()`/`micros()`. Las ISR AVR no se anidan ni tienen prioridades de RTOS configurables; véase la diferencia entre prioridad relativa y orden de vectores en la bitácora.
- **Movimiento:** avance de 25/1000 cada 20 ms y primer paso inmediato: 780 ms en la simulación con bucle de 1 ms. La duración real requiere video; la rampa no recupera pasos atrasados de golpe.
- **Interfaz:** muestreo cada 5 ms y antirrebote de 10 ms. Máximo observado de 14 ms en las cinco fases del ensayo de botón con bucle simulado de 1 ms; no garantiza la latencia eléctrica del montaje.
- **Prioridades del bucle:** seguridad/muestras, UI/comandos, rampa/LED, un paso EEPROM y telemetría. Trabajo acotado; UART descarta sin esperar espacio.
- **D2:** un flanco por conversión: 1 ms entre flancos, onda completa de 500 Hz. `EMG_SAMPLE_PROBE=0` libera D2.
- **EEPROM:** contador y complemento en 0–7, calibración AD8232 (`0xE702`) en 16–23. Contador escrito y releído al arrancar, antes de ADC. Calibración: invalidar firma, escribir datos y checksum, confirmar firma al final; como máximo un byte por vuelta si EEPROM está lista y lectura posterior de cada byte. `EEPROM_SAVED` confirma persistencia; `EEPROM_SAVE_FAILED` deja la calibración nueva solo en RAM. Un corte durante escritura puede perder la calibración persistida: se rechaza el registro parcial y se solicita calibrar al próximo arranque. No hay sistema de archivos, reparto de desgaste ni recuperación garantizada del contador tras un corte durante su escritura.

No uses `tone()`, `analogRead()` ni reconfigures Timer1/Timer2 mientras este firmware esté activo. No se usa `analogWrite()` para los servos; Servo genera sus señales por interrupciones incluso en los pines no marcados como PWM.

Monitor serie a **115200 baud**:

- `c`: iniciar/repetir calibración.
- `p`: activar/desactivar datos de diagnóstico a 10 Hz.
- `x`: cancelar calibración y ordenar apertura; en EMG exige reposo antes de volver a cerrar. No sustituye al interruptor físico de alimentación.

Eventos: `E,milisegundos,nombre,valor`, por ejemplo `E,12345,CLOSE,25`. `BOOT` informa el contador persistente; `MODE` usa 0 para manual y 1 para EMG. `TH_OPEN` y `TH_CLOSE` informan los umbrales calibrados. Los eventos `CLOSE` permiten contar las detecciones en R6, pero no son mediciones de movimiento real.

Datos: `D,milisegundos,ADC_crudo,base_DC,envolvente,umbral_abrir,umbral_cerrar,estado,overruns,logs_descartados`. `estado` combina bits: 1 = LO+/LO− alto; 2 = recorte/rango inválido; 4 = procesamiento aún no listo; 8 = ADC sin muestras recientes; 16 = pérdida de datos sin recuperación completa; 32 = falla de inicialización ADC o de salida a servos. Pueden sumarse varios bits. `estado=0` solo indica que estos controles de señal pasan; no certifica que la actividad sea una contracción voluntaria.

Los datos a 10 Hz sirven para ver niveles y estados, no para reconstruir la forma de onda ECG. Los overruns cuentan pérdidas detectadas por ADC ocupado/pendiente o cola llena; el contador satura en 65535. No cuentan muestras omitidas si Timer2 deja de funcionar. La UART descarta líneas si no hay espacio para evitar bloquear el control; en una prueba de registro, verifica que no se descarten líneas. Puedes compilar con `EMG_SERIAL_DEBUG=0` para desactivar UART; seguirá disponible la calibración con D8 al arranque.

Un LO alto, falta de muestras o hueco de adquisición de más de 10 ms, cola desbordada o recorte ordenan apertura en modo de señal. Una muestra cuya edad supera 10 ms se descarta: leer datos viejos no renueva la salud del sensor. Por defecto se considera recorte `raw <= 5` o `raw >= 669`, calculado para sensor de 3.3 V y AVcc=5 V. `ADC_REFERENCE_MV`, `SENSOR_SUPPLY_MV` y márgenes están en `Config.h`; si se ajustan según mediciones, recalibra. El recorte, el hueco de adquisición o la pérdida de datos invalidan la ventana y exigen otras 2000 muestras estables (~2 s). Una falla durante la calibración la rechaza y requiere repetirla; en operación normal la señal puede recuperarse automáticamente, exigiendo reposo antes del siguiente cierre. La apertura conserva la rampa del mecanismo; no es una desconexión de potencia.

LO+/LO− dependen de la configuración física de detección del módulo. No garantizan detectar un cable de OUTPUT roto, la desconexión de la referencia RL o todos los casos de mal contacto; tampoco detectan un dedo bloqueado. F1 sigue requiriendo ensayos reales.

## Archivos de apoyo para hardware

Estos sketches independientes se conservan tal como estaban en el directorio del proyecto y ahora estan versionados. Sirven para revisar cada componente antes de integrarlo; cada uno reemplaza temporalmente al programa principal en la placa.

| Apoyo | Para que se usa | Instrucciones |
| --- | --- | --- |
| `Prueba_Servo_Serial/Prueba_Servo_Serial.ino` | Enviar angulos 0..180 a un servo en D3, 9600 baudios; posicion inicial 90 grados | [Conexion y uso](Prueba_Servo_Serial/README.md) |
| `Prueba_AD8232_Serial/Prueba_AD8232_Serial.ino` | Observar ADC crudo y LO+/LO- por Monitor Serie/Plotter, 115200 baudios; 500 Hz nominales | [Lecturas y escala](Prueba_AD8232_Serial/README.md) |

Ambos compilan para UNO. Las confirmaciones del servo y las lecturas del ADC son herramientas de diagnostico; los resultados fisicos deben registrarse durante el ensayo. Las pruebas automatizadas de `tests/` verifican logica con datos sinteticos y cumplen otra funcion.

## Configuración y verificación reproducibles

Versiones verificadas: Arduino CLI 1.1.1, Arduino AVR Boards **1.8.6**, Servo **1.2.2**, EEPROM **2.0** incluida en AVR. Firmware Arduino C++11; pruebas nativas C++11 (MSVC usa C++14). Se conservan Arduino y Servo, por lo que no se presenta como C puro.

Preparación en un equipo nuevo con Arduino CLI en PATH, usando Bash:

```bash
bash scripts/bootstrap.sh
```

En PowerShell, instalar las mismas dependencias y comprobar cada salida:

```powershell
arduino-cli core update-index
if ($LASTEXITCODE -ne 0) { throw "No se pudo actualizar el índice" }
arduino-cli core install arduino:avr@1.8.6
if ($LASTEXITCODE -ne 0) { throw "No se pudo instalar AVR" }
arduino-cli lib install Servo@1.2.2
if ($LASTEXITCODE -ne 0) { throw "No se pudo instalar Servo" }
```

Desde la raíz del repositorio:

```powershell
python scripts/test.py
python scripts/build.py
python scripts/build.py --no-debug --no-probe
```

`build.py` busca CLI en PATH o en Arduino IDE de Windows, comprueba AVR/Servo y selecciona explícitamente la biblioteca. Acepta `--cli RUTA`, `--config RUTA_YAML` y `--servo-library CARPETA_SERVO` para otras instalaciones. No descarga ni carga firmware. Los scripts propagan errores y generan logs en `build/` (ignorado por Git). `test.py` requiere Python 3 y g++, clang++ o Visual Studio C++ con Windows SDK; descubre el entorno de MSVC automáticamente. Regenera `docs/resultados-pruebas.md`. En Bash también puede usarse `bash scripts/verify.sh`.

Para cargar: Arduino IDE → abrir `Inmoov_EMG/Inmoov_EMG.ino` → Arduino Uno → puerto de la placa → Subir. O, después de compilar con CLI:

```powershell
arduino-cli upload --fqbn arduino:avr:uno --port COM3 --input-dir build/uno Inmoov_EMG
if ($LASTEXITCODE -ne 0) { throw "No se pudo cargar el firmware" }
```

Sustituye COM3 por el puerto real. Si usaste un YAML personalizado, pásalo también a `upload`. Después de cargar, cada encendido ejecuta `setup()` automáticamente; no se espera conexión USB ni se requiere servicio Linux. La calibración válida se recupera de EEPROM; sin ella, la mano permanece abierta mientras solicita la secuencia de calibración. La variante sin UART conserva el pulsador D8 al arrancar.

Resultados y límites de validación están en [la tabla de pruebas](docs/resultados-pruebas.md) y [la matriz de requisitos](docs/requisitos.md). No se cargó firmware en una placa ni se accionaron motores durante esta modificación. El movimiento, PWM, consumo, exactitud muscular, autonomía y estabilidad eléctrica requieren ensayos físicos.

## Historial de versiones

El historial de `main` parte del codigo base `v0.1.0` y separa cada modulo, conexion de drivers, pruebas y documentacion en commits propios. El [registro paso a paso](docs/modularizacion-paso-a-paso.md) muestra la evolucion y [versiones](docs/versiones.md) relaciona las etiquetas.

Etapas: `v0.5.0-modulos`, `v0.5.0-integracion`, `v0.5.0-pruebas` y `v0.5.0` final. Las etiquetas v0.2.0 a v0.4.1 permiten consultar las versiones preliminares.

Los commits separan los cambios por responsabilidad. Los sketches de apoyo se incorporan como herramientas independientes. El firmware y las pruebas automatizadas conservan su funcionamiento; las mediciones fisicas se registran por separado.
