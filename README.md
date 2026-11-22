# Mano InMoov con AD8232 (ECG, 3.3 V)

## Inicio rápido

Desde la raíz del repositorio, con **Python 3.10+**, pip, **Arduino CLI 1.1.1** en PATH y un compilador C++ (g++, clang++ o Visual Studio C++ con Windows SDK). La interfaz gráfica del analizador también requiere Tk.

1. **Instala las dependencias.** En Bash: `bash scripts/bootstrap.sh` (usa `python3`; puedes seleccionar otro intérprete con `PYTHON`). En PowerShell:

   ```powershell
   arduino-cli core update-index
   if ($LASTEXITCODE -ne 0) { throw "No se pudo actualizar el indice" }
   arduino-cli core install arduino:avr@1.8.6
   if ($LASTEXITCODE -ne 0) { throw "No se pudo instalar AVR" }
   arduino-cli lib install Servo@1.2.2
   if ($LASTEXITCODE -ne 0) { throw "No se pudo instalar Servo" }
   python -m pip install -r Inmoov_EMG/analizador_senal/requirements.txt
   if ($LASTEXITCODE -ne 0) { throw "No se pudieron instalar las dependencias Python" }
   ```

2. **Ejecuta las pruebas**, guardando el informe en `build/` para conservar el informe histórico:

   ```powershell
   python scripts/test.py --report build/resultados-pruebas.md
   ```

   Debe terminar con código 0. Si faltan `pyserial` o `cryptography`, avisa al inicio e indica cómo instalarlos; `--native-only` omite las pruebas Python y esa comprobación.

3. **Compila FW8** para Arduino Uno:

   ```powershell
   python scripts/build.py
   ```

   Debe terminar con código 0 y generar `build/normal` (sin UART) y `build/depuracion` (para el analizador), además de sus logs. No carga la placa.

4. **Prepara el cableado mínimo**, sin energizar: AD8232 a **3.3 V**, GND común, OUTPUT→A0, LO+→A1, LO−→A2 y SDN→3.3 V; señales de servos pulgar/índice/medio/anular/meñique→D3/D5/D6/D10/D11. Los servos son MG995: aliméntalos con **4 celdas 18650 (dos portapilas de dos celdas en serie (7.4 V nominales cada uno), conectados entre sí en paralelo: 7.4 V nominales y el doble de capacidad) y un regulador a 5 V** dimensionado para su corriente, y alimenta el Arduino aparte con una **pila cuadrada de 9 V** si no lo conectas a la computadora; nunca los servos desde el pin 5V ni el USB del UNO. Pulsador D8→GND, selector D9→GND para manual (abierto para EMG). **Sin LED RGB**; D7/D12/D13 quedan libres. Consulta [el detalle eléctrico](#compatibilidad-del-sensor-y-alimentación) y [la tabla de señales](#cableado-de-señales).

5. **Carga y comprueba.** Sustituye COM3 por el puerto real; durante la carga USB mantén los electrodos desconectados del cuerpo:

   ```powershell
   arduino-cli upload --fqbn arduino:avr:uno --port COM3 --input-dir build/normal Inmoov_EMG
   if ($LASTEXITCODE -ne 0) { throw "No se pudo cargar el firmware" }
   ```

   Si `build.py` generó `build/arduino-local.yaml`, añade `--config-file build/arduino-local.yaml` a `upload`; con una configuración propia, usa el mismo YAML que al compilar. También puedes abrir `Inmoov_EMG/Inmoov_EMG.ino` en Arduino IDE, seleccionar Arduino Uno y subir. Prueba primero con tensores sueltos en manual: cada pulsación D8 alterna cierre/apertura. En EMG espera ~2 s de señal estable y otras 200 muestras (~200 ms); FW8 inicia automáticamente, **sin calibración**. Véase [carga y primera prueba](#carga-y-primera-prueba).

## Proyecto y estado de FW8

Adaptación de `Inmoov_Robot_Hand.ino` de Kuldip.K Gami al proyecto de Fundamentos de Sistemas Embebidos. Los archivos originales `.ino` y `Robotic Hand.txt` contienen el mismo programa. Se retiraron `SoftwareSerial`, los comandos Bluetooth y los movimientos con `delay()`.

La implementación conserva la plataforma y los pines de actuadores del proyecto: Arduino UNO R3, ATmega328P a 16 MHz y cinco servos. El sensor del proyecto ahora es el **AD8232 de UNIT Electronics**. Se añaden A1 y A2 para detectar pérdida de contacto. Un solo canal de actividad abre/cierra la mano completa; no identifica dedos individuales.

La conexión de alimentación de un EMG genérico al riel de 5 V **no corresponde a este AD8232**.

Versión actual: **FW8**, entrega final con control fijo por tres niveles, sin calibración. Se etiquetará **v1.0.0** al terminar la rama `limpieza-fw8`. [Control por promedios](docs/control-senal.md) · [Datos sintéticos de prueba](tests/datos/README.md).

Las capturas reales del AD8232 ya no se versionan porque contienen datos biométricos/de salud: el ECG puede identificar a la persona y este repositorio es público. Sus resultados se resumen en el informe del Avance 4. Se retiran del árbol actual sin reescribir el historial público; las revisiones anteriores aún las contienen. El analizador guarda las sesiones nuevas fuera del repo, en `Path.home() / "registros-emg"`, con identificador aleatorio.

El analizador solicita consentimiento y contraseña por sesión, cifra los registros con AES-256-GCM y verifica su integridad con HMAC-SHA256. No guarda contraseñas, omite la hora absoluta por defecto y no exporta PNG automáticamente. Consulta [instalación, formato, verificación, descifrado y conservación](Inmoov_EMG/analizador_senal/README.md). No publiques registros ni contraseñas.

## Estado actual del sensor

El módulo AD8232 presenta una señal muy variable y todavía se investiga la causa. Entre las hipótesis están una soldadura deficiente, un módulo deteriorado, jumpers o conectores con mal contacto, alimentación/referencia inestable y condiciones de los electrodos. Ninguna se ha confirmado. El filtrado y las pruebas automatizadas no demuestran que el problema físico esté resuelto. Consulta [el diagnóstico y las comprobaciones pendientes](docs/diagnostico-sensor.md).

## Código modular

[`Inmoov_EMG/Inmoov_EMG.ino`](Inmoov_EMG/Inmoov_EMG.ino) es el único sketch generado del firmware y contiene el programa completo, incluida la configuración de los servos, para abrirlo directamente en Arduino IDE. Se genera desde los módulos `.h` y `.cpp` con `python scripts/export_ino.py`; edita esos módulos y regenera el sketch para conservar los cambios. Los `.cpp` se compilan por separado solo en las pruebas nativas para evitar definiciones duplicadas en Arduino.
Los comentarios `D01` a `D06` enlazan decisiones concretas con [la bitácora técnica](docs/bitacora.md).

| Módulo | Responsabilidad |
| --- | --- |
| `Config.h` | Pines, tiempos, límites y direcciones del contador EEPROM con nombre |
| `Acquisition.h/.cpp` | Timer2/ADC, ISR breve, cola circular y secciones atómicas |
| `SampleQueue.h` | Comunicación SPSC de muestras completas con marca de tiempo |
| `Signal.h` | Salud, centrado DC, rectificación y media Q6 |
| `SignalFilter.h` | HP30, rechazo de 60 Hz y LP200 a 1 kHz, fuera de ISR |
| `Decision.h` | Promedio de 200 muestras de envolvente y control fijo por tres niveles |
| `Motion.h` | Rampa simultánea y conversión a pulsos |
| `Inputs.h` | Antirrebote de botón y selector |
| `Outputs.h/.cpp` | GPIO y cinco servos; deshace un arranque parcial |
| `Storage.h/.cpp` | Contador de arranque EEPROM con lectura de verificación |
| `Diagnostics.h/.cpp` | Comandos y telemetría UART de baja prioridad |
| `Application.h/.cpp` | Coordinación de módulos y recuperación de fallas |
| `tests/`, `scripts/` | Pruebas con fallas inducidas y compilación reproducible |

[Diagrama conectado](docs/arquitectura.md) · [Matriz de requisitos](docs/requisitos.md) · [Resultados esperados y obtenidos](docs/resultados-pruebas.md) · [Protocolo físico](docs/pruebas-hardware.md).

## Compatibilidad del sensor y alimentación

**Alimenta el módulo AD8232 a 3.3 V; nunca directamente desde el paquete de baterías ni desde el riel de servos de 5 V.** Conecta su `GND` a la tierra común y `OUTPUT` a A0. Su rango nominal de salida queda dentro de 0–3.3 V. El UNO conserva AVcc, nominalmente 5 V, como referencia ADC: la salida ocupará aproximadamente 0–675 cuentas de sus 10 bits. No hace falta conectar AREF a 3.3 V.

`LO+` y `LO−` se conectan a A1/A2 como **entradas sin pull-up de 5 V**. Un nivel alto en cualquiera inhibe el control por señal. `SDN` se puede mantener a 3.3 V para habilitar el módulo; no se maneja con una salida de 5 V del UNO. Los terminales RA/LA/RL corresponden a los cables de electrodos, no a alimentación ni GND.

**El AD8232 es un acondicionador ECG, no un sensor que entregue una envolvente EMG.** El [fabricante Analog Devices](https://www.analog.com/en/products/ad8232.html) explica que su banda y ganancia dependen de componentes externos. No se conoce el esquema ni la banda exacta de esta tarjeta UNIT; no se atribuyen a ella los valores de otros módulos. El control de contracciones con esta tarjeta es **experimental**: el firmware calcula una medida de actividad, pero no distingue actividad muscular voluntaria de ECG o artefactos de movimiento. El software no recupera frecuencias eliminadas por el filtro analógico. La separación entre reposo y contracción requiere validación física; FW8 usa fronteras fijas, sin calibración.

Referencias eléctricas: [ficha AD8232, revisión D](https://www.analog.com/media/en/technical-documentation/data-sheets/ad8232.pdf) y [guía de conexión de SparkFun](https://learn.sparkfun.com/tutorials/ad8232-heart-rate-monitor-hookup-guide/). La ficha del circuito integrado no sustituye el esquema de la tarjeta UNIT.

La alimentación final usa **4 celdas 18650 para los servos MG995 (dos portapilas de dos celdas en serie (7.4 V nominales cada uno), conectados entre sí en paralelo: 7.4 V nominales y el doble de capacidad, con un regulador a 5 V) y una pila cuadrada de 9 V para el Arduino**; la LiPo 3S propuesta no se consiguió. Con la fuente anterior, de 5 V fijos, se midieron con multímetro 0.1 a 0.5 A (6 y 13 de noviembre de 2026) y los servos no se movían, tardaban o no giraban completo; además el módulo se quedaba sin alimentación, por eso el Arduino va aparte. Alimenta los servos desde el regulador, dimensionado para sus corrientes, con GND común (también unido a la pila de 9 V); no desde el pin 5V ni el USB del Arduino. Corriente, regulación y autonomía siguen pendientes de validación en el montaje. Con electrodos en el cuerpo, usa alimentación por batería sin conexiones a la red, o aislamiento apropiado para la interfaz de depuración e instrumentos. La [guía del fabricante MyoWare](https://cdn.sparkfun.com/assets/learn_tutorials/1/9/5/6/MyoWare_v2_QuickStartGuide.pdf) explica esta precaución para sistemas de electrodos conectados a USB.

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
| Pines libres | D7/D12/D13 | Sin LED RGB ni asignación en el firmware |
| Prueba de muestreo | D2 | Salida opcional; un flanco por conversión ADC |
| Depuración USB | D0/D1 | UART a 115200 baud solo en la variante de depuración |

La tabla sustituye los pines de servo D2–D6 del código original: el proyecto usa D3/D5/D6/D10/D11.

La guía genérica de SparkFun conecta LO+/LO− a D10/D11; aquí esos pines ya controlan servos y **se usan A1/A2 para LO+/LO−**. No conectes ambas funciones a los mismos pines.

## Carga y primera prueba

Abre [Inmoov_EMG.ino](Inmoov_EMG/Inmoov_EMG.ino) para cargar el firmware completo y la inversión de medio, anular y meñique. Conserva el nombre de la carpeta `Inmoov_EMG` y del archivo `Inmoov_EMG.ino`. No requiere copiar los módulos `.h` y `.cpp`.

1. Abre `Inmoov_EMG/Inmoov_EMG.ino` en Arduino IDE. Instala la biblioteca **Servo** y selecciona **Arduino Uno** y el puerto correcto. EEPROM viene con el núcleo AVR.
2. Los valores `OPEN_US` y `CLOSED_US` están incluidos en el propio `.ino`. Medio (D6), anular (D10) y meñique (D11) tienen los extremos intercambiados para invertir su giro: apertura a 2400 us y cierre a 544 us. Si mantienes los módulos como fuente, ajusta `Config.h` y regenera con `python scripts/export_ino.py`. Ajusta los extremos con los tensores sueltos.
3. Carga el programa y verifica primero el modo manual: D9 a GND y una pulsación en D8 para cerrar; otra para abrir. Mantener pulsado no repite la orden. Un cambio de modo ordena abrir la mano.
4. Conecta OUTPUT y ambas salidas LO. Espera 2 s de señal estable. El control fijo comienza tras estabilizar la señal y completar 200 muestras nuevas; no requiere calibración.

La compilación predeterminada es **normal**, con `EMG_SERIAL_DEBUG=0`: no inicia, lee ni emite por UART. La UART del UNO no ofrece cifrado ni autenticación; la variante de uso normal evita transmitir datos de señal y estado.

Para usar el analizador, carga la variante **depuracion** según [Inicio rápido](#inicio-rápido), cambiando `--input-dir` a `build/depuracion`. En Arduino IDE puedes añadir `#define EMG_SERIAL_DEBUG 1` al principio del `.ino`, antes de la sección `Config.h`, y subirlo; la regeneración elimina esa edición local. Con CLI se habilita mediante `-DEMG_SERIAL_DEBUG=1`, que `build.py` aplica a esa variante. Para volver al uso normal, carga `build/normal` o elimina el define del IDE y vuelve a subir.

Solo en depuración, abre el Monitor Serie a **115200 baudios** y envía `?` para identificar el firmware. Debe aparecer `E,...,SERVO_REVERSE_MASK,28`: indica que medio, anular y meñique están invertidos. Cierra el monitor antes de abrir el analizador. El flujo periódico arranca apagado y se activa con `p`; el analizador lo envía automáticamente si no recibe flujo inicial. Los eventos puntuales de arranque, fallas y consultas pueden emitirse sin `p`. Si el movimiento físico no corresponde, comprueba que sus señales estén en D6, D10 y D11.

Los pulsos de 544/2400 us representan **0°/180° ordenados**, sin garantizar esos ángulos físicos. El ángulo real depende del servo, polea y tensores; debe medirse y ajustarse mecánicamente como indica R3. Se usan `writeMicroseconds()` y límites por dedo para no confundir grados con ancho de pulso.

## Control de mano por tres niveles

FW8 promedia bloques de 200 muestras de envolvente (~200 ms) y ordena a los cinco dedos: promedio menor que 5 -> 0°, de 5 a menos de 10 -> 90°, desde 10 -> 180°. No requiere calibración ni utiliza sus umbrales anteriores de EEPROM. Los promedios fraccionarios se clasifican sin redondear hacia arriba.

Se abandonó la calibración porque solía fallar al mezclarse la señal con los latidos. La tercera señal del analizador (envolvente) ofreció la referencia elegida por el equipo: 1–4 en reposo, 5–9 con tensión media y ≥10 con la mano tensada. Promediar 200 muestras introduce ~200 ms por bloque: **R1 (latencia ≤50 ms) no se cumple por diseño**. Estas observaciones previas motivan el método; no son nuevas mediciones de este paso. Consulta [la decisión en la bitácora](docs/bitacora.md#retiro-de-calibración-y-control-ponderado-en-fw8).

La rampa usa 544/1472/2400 us para los objetivos 0°/90°/180° en pulgar e índice, y 2400/1472/544 us en medio, anular y meñique (0° → 180°, 90° → 90°, 180° → 0°). Una señal inválida vacía el promedio y abre en modo EMG. El ángulo real debe verificarse en el montaje. Consulta [el funcionamiento](docs/control-senal.md).

En el analizador, Iniciar graba y Detener termina el registro. Pausar / abrir (`x`) pausa el control y solicita apertura. Activar control (`c`) lo reactiva, sin calibrar. La pantalla muestra media y destino. D9/D8 conservan modo manual. La variabilidad física sigue en investigación.

En la variante de depuración, las fallas se reportan por UART a 115200 baudios mediante eventos `E,...`, incluidos `SIGNAL_FAULT`, `SERVO_INIT_FAILED`, `SERVO_WRITE_FAILED`, `ADC_INIT_FAILED` y `EEPROM_BOOT_FAILED`.

## Temporización y diagnóstico

- **Timer2:** CTC, prescaler 64, OCR2A=249; solicita una conversión cada 1 ms. Si el ADC sigue ocupado o su resultado no fue recogido, registra pérdida.
- **ADC:** A0/AVcc, prescaler 128, reloj ADC de 125 kHz. La ISR publica ADC, LO+/LO− y `millis()` en una cola circular estática de 32 posiciones (31 útiles). La primera conversión tiene timeout de 1000 us; se comprueba la configuración por lectura de registros.
- **Sincronización:** productor ISR y consumidor `loop`; extracción y alarmas protegidas con `ATOMIC_RESTORESTATE`. `volatile` da visibilidad, la sección crítica da atomicidad. No existen procesos ni hilos de SO en este diseño bare-metal.
- **Procesamiento en el bucle:** Signal centra, SignalFilter aplica HP30/notch60/LP200 y Signal rectifica/promedia 32 muestras con precisión Q6. Cada muestra se procesa una sola vez, hasta 31 por vuelta. Las ISR no filtran ni escriben UART/EEPROM.
- **Timer1:** reservado a Servo para cinco trenes de pulsos de aproximadamente 50 Hz. Timer0 conserva `millis()`/`micros()`. Las ISR AVR no se anidan ni tienen prioridades de RTOS configurables; véase la diferencia entre prioridad relativa y orden de vectores en la bitácora.
- **Movimiento:** avance de 25/1000 cada 20 ms y primer paso inmediato: 780 ms en la simulación con bucle de 1 ms. La duración real requiere video; la rampa no recupera pasos atrasados de golpe.
- **Interfaz:** muestreo cada 5 ms y antirrebote de 10 ms. Máximo observado de 14 ms en las cinco fases del ensayo de botón con bucle simulado de 1 ms; no garantiza la latencia eléctrica del montaje.
- **Prioridades del bucle:** seguridad/muestras, UI/comandos, rampa y telemetría. Trabajo acotado; UART descarta sin esperar espacio. No hay tarea de LED RGB.
- **D2:** un flanco por conversión: 1 ms entre flancos, onda completa de 500 Hz. `EMG_SAMPLE_PROBE=0` libera D2.
- **EEPROM:** solo se conserva el contador de arranque verificado, escrito antes de habilitar el ADC. Se retiraron las utilidades de calibración y su guardado; los bytes de umbrales antiguos se ignoran.

No uses `tone()`, `analogRead()` ni reconfigures Timer1/Timer2 mientras este firmware esté activo. No se usa `analogWrite()` para los servos; Servo genera sus señales por interrupciones incluso en los pines no marcados como PWM.

Monitor serie a **115200 baud**, solo con la variante de depuración:

- `c`: reanudar el control fijo y comenzar un bloque nuevo de 200 muestras válidas, sin calibración.
- `?`: consultar versión, banda, fronteras fijas, estado del control e inversión de servos.
- `p`: activar/desactivar datos de diagnóstico a 10 Hz, apagados al arrancar, y eventos auxiliares periódicos.
- `x`: pausar el control y ordenar apertura. No sustituye al interruptor físico de alimentación.

Eventos: `E,milisegundos,nombre,valor`. `FW=8`, `FIXED_CONTROL=1`, `AVERAGE_SAMPLES=200`, `LEVEL_MID=5` y `LEVEL_HIGH=10` describen el modo fijo. `MEAN_X10` informa el promedio en décimas, `TARGET_DEG` el destino 0/90/180 y `CONTROL_PAUSED` la pausa. Son órdenes, no movimiento físico medido.

Datos: `D,milisegundos,ADC_crudo,base_DC,envolvente,umbral_abrir,umbral_cerrar,estado,overruns,logs_descartados`. `estado` combina bits: 1 = LO+/LO− alto; 2 = recorte/rango inválido; 4 = procesamiento aún no listo; 8 = ADC sin muestras recientes; 16 = pérdida de datos sin recuperación completa; 32 = falla de inicialización ADC o de salida a servos. Pueden sumarse varios bits. `estado=0` solo indica que estos controles de señal pasan; no certifica que la actividad sea una contracción voluntaria.

Los datos a 10 Hz sirven para ver niveles y estados, no para reconstruir la forma de onda ECG. Los overruns cuentan pérdidas detectadas por ADC ocupado/pendiente o cola llena; el contador satura en 65535. No cuentan muestras omitidas si Timer2 deja de funcionar. La UART descarta líneas si no hay espacio para evitar bloquear el control; en una prueba de registro, verifica que no se descarten líneas. El analizador requiere la variante de depuración; el firmware normal conserva el control fijo autónomo y el modo manual con D9/D8, sin comandos UART.

Un LO alto, falta de muestras o hueco de adquisición de más de 10 ms, cola desbordada o recorte ordenan apertura en modo de señal. Una muestra cuya edad supera 10 ms se descarta: leer datos viejos no renueva la salud del sensor. Por defecto se considera recorte `raw <= 5` o `raw >= 669`, calculado para sensor de 3.3 V y AVcc=5 V. `ADC_REFERENCE_MV`, `SENSOR_SUPPLY_MV` y márgenes están en `Config.h`; si se ajustan según mediciones, regenera y recompila. El recorte, el hueco de adquisición o la pérdida de datos invalidan la ventana y exigen otras 2000 muestras estables (~2 s). Tras recuperarse, el control EMG necesita un bloque nuevo de 200 muestras válidas; no requiere recalibrar ni pasar por reposo. La apertura conserva la rampa del mecanismo; no es una desconexión de potencia.

LO+/LO− dependen de la configuración física de detección del módulo. No garantizan detectar un cable de OUTPUT roto, la desconexión de la referencia RL o todos los casos de mal contacto; tampoco detectan un dedo bloqueado. F1 sigue requiriendo ensayos reales.

## Archivos de apoyo para hardware

Estos sketches independientes se conservan tal como estaban en el directorio del proyecto y ahora estan versionados. Sirven para revisar cada componente antes de integrarlo; cada uno reemplaza temporalmente al programa principal en la placa.

| Apoyo | Para que se usa | Instrucciones |
| --- | --- | --- |
| `Prueba_Servo_Serial/Prueba_Servo_Serial.ino` | Enviar angulos 0..180 a un servo en D3, 9600 baudios; posicion inicial 90 grados | [Conexion y uso](Prueba_Servo_Serial/README.md) |
| `Prueba_AD8232_Serial/Prueba_AD8232_Serial.ino` | Observar ADC crudo y LO+/LO- por Monitor Serie/Plotter, 115200 baudios; 500 Hz nominales | [Lecturas y escala](Prueba_AD8232_Serial/README.md) |

Ambos compilan para UNO. Las confirmaciones del servo y las lecturas del ADC son herramientas de diagnostico; los resultados fisicos deben registrarse durante el ensayo. Las pruebas automatizadas de `tests/` verifican logica con datos sinteticos y cumplen otra funcion.

## Configuración y verificación reproducibles

Dependencias fijadas: Arduino CLI 1.1.1, Arduino AVR Boards **1.8.6**, Servo **1.2.2**, EEPROM **2.0** incluida en AVR; dependencias Python en [requirements.txt](Inmoov_EMG/analizador_senal/requirements.txt). Firmware Arduino C++11; pruebas nativas C++14. Se conservan Arduino y Servo, por lo que no se presenta como C puro. Los comandos de instalación, pruebas, compilación y carga están en [Inicio rápido](#inicio-rápido).

`build.py` busca CLI en PATH o en Arduino IDE de Windows, comprueba AVR/Servo y selecciona explícitamente la biblioteca. Antes de compilar, regenera el único sketch de firmware, `Inmoov_EMG/Inmoov_EMG.ino`. `--variant normal` o `--variant depuracion` selecciona solo una variante; `--no-debug` es alias de la normal. `--no-probe` desactiva D2 y añade `-sin-sonda` a la carpeta y al log (por ejemplo, `python scripts/build.py --variant normal --no-probe`). Acepta `--cli RUTA`, `--config RUTA_YAML` y `--servo-library CARPETA_SERVO` para otras instalaciones. No descarga ni carga firmware. Los scripts propagan errores y generan logs en `build/` (ignorado por Git).

Memoria obtenida al ejecutar `python scripts/build.py` el **2026-11-21**, para UNO con AVR 1.8.6, Servo 1.2.2 y sonda D2 habilitada (opción predeterminada):

| Variante | Flash usada / disponible | SRAM global usada / total | SRAM restante para pila y temporales |
| --- | --- | --- | --- |
| `normal` (UART deshabilitada) | 7128 / 32256 B (22 %) | 508 / 2048 B (24 %) | 1540 B |
| `depuracion` (UART habilitada) | 11404 / 32256 B (35 %) | 1002 / 2048 B (48 %) | 1046 B |

Son cifras del compilador, conservadas en `build/normal.txt` y `build/depuracion.txt`; el margen restante no mide el máximo uso de pila en ejecución.

`test.py` descubre el entorno de MSVC automáticamente. Prueba el driver UART con `EMG_SERIAL_DEBUG=1` y comprueba por separado que la configuración normal no inicia, lee ni escribe UART. Por defecto regenera `docs/resultados-pruebas.md`; durante esta limpieza se usa siempre `--report build/resultados-pruebas.md` hasta el paso 12. `--native-only` omite el analizador Python. REPLAY-30S, PC-DATOS y EVIDENCIA usan exclusivamente el [dato sintético y su manifiesto](tests/datos/README.md); no validan señales físicas.

Para cargar la variante que necesita el analizador, cambia el comando de Inicio rápido a `--input-dir build/depuracion`. Después de cargar, cada encendido ejecuta `setup()` automáticamente; no se espera conexión USB ni se requiere servicio Linux. Ambas variantes ejecutan FW8 sin calibración y conservan el modo manual con D9/D8.

Resultados y límites de validación están en [la tabla de pruebas](docs/resultados-pruebas.md) y [la matriz de requisitos](docs/requisitos.md). No se cargó firmware en una placa ni se accionaron motores durante esta modificación. El movimiento, PWM, consumo, exactitud muscular, autonomía y estabilidad eléctrica requieren ensayos físicos.

## Historial de versiones

La base modular recibida es `a37d208`. Los cambios posteriores se separan en mediciones, filtro, señal, calibración, decisión, telemetría, integridad de evidencia y consolidación de `v0.6.0`. Las etiquetas y fechas se detallan en [versiones](docs/versiones.md). Las capturas y sus hashes pertenecen al historial anterior; ya no se versionan datos biométricos en el árbol actual. Sus resultados se resumen en el informe del Avance 4.
