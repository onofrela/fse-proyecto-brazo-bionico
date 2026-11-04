# Mano InMoov con AD8232 (ECG, 3.3 V)

Adaptación de `Inmoov_Robot_Hand.ino` de Kuldip.K Gami al proyecto de Fundamentos de Sistemas Embebidos. Los archivos originales `.ino` y `Robotic Hand.txt` contienen el mismo programa. Se retiraron `SoftwareSerial`, los comandos Bluetooth y los movimientos con `delay()`.

La implementación conserva la plataforma y los pines de actuadores del documento del avance del proyecto: Arduino UNO R3, ATmega328P a 16 MHz y cinco servos. El sensor del proyecto ahora es el **AD8232 de UNIT Electronics**, según la descripción de compra proporcionada por el usuario. Se añaden A1 y A2 para detectar pérdida de contacto. Un solo canal de actividad abre/cierra la mano completa; no identifica dedos individuales.

El documento original es una referencia de diseño: su conexión de alimentación de un EMG genérico al riel de 5 V **no corresponde a este AD8232**.

## Archivos

- `Inmoov_EMG/Inmoov_EMG.ino`: adquisición ADC, interrupciones, modos, servos, LED y EEPROM.
- `Inmoov_EMG/Config.h`: pines, polaridad del RGB y límites de pulso de cada servo.
- `Inmoov_EMG/Control.h`: eliminación de nivel DC, rectificación, envolvente, histéresis, antirrebote, calibración y movimiento.
- `tests/control_test.cpp`: pruebas de la lógica en una computadora, sin accionar motores.

## Compatibilidad del sensor y alimentación

**Alimenta el módulo AD8232 a 3.3 V; no a 5 V, 9 V ni a la LiPo directamente.** Conecta su `GND` a la tierra común y `OUTPUT` a A0. Su rango nominal de salida queda dentro de 0–3.3 V. El UNO conserva AVcc, nominalmente 5 V, como referencia ADC: la salida ocupará aproximadamente 0–675 cuentas de sus 10 bits. No hace falta conectar AREF a 3.3 V.

`LO+` y `LO−` se conectan a A1/A2 como **entradas sin pull-up de 5 V**. Un nivel alto en cualquiera inhibe el control por señal. `SDN` se puede mantener a 3.3 V para habilitar el módulo; no se maneja con una salida de 5 V del UNO. Los terminales RA/LA/RL corresponden a los cables de electrodos, no a alimentación ni GND.

**El AD8232 es un acondicionador ECG, no un sensor que entregue una envolvente EMG.** El [fabricante Analog Devices](https://www.analog.com/en/products/ad8232.html) explica que su banda y ganancia dependen de componentes externos. No se conoce el esquema ni la banda exacta de esta tarjeta UNIT; no se atribuyen a ella los valores de otros módulos. El control de contracciones con esta tarjeta es **experimental**: el firmware calcula una medida de actividad, pero no distingue actividad muscular voluntaria de ECG o artefactos de movimiento. El software no recupera frecuencias eliminadas por el filtro analógico. Si la calibración no logra separar reposo y contracción de forma repetible, hay que revisar el acondicionamiento físico o usar un módulo EMG apropiado; bajar el umbral sin medir no resuelve esa limitación.

Referencias eléctricas: [ficha AD8232, revisión D](https://www.analog.com/media/en/technical-documentation/data-sheets/ad8232.pdf) y [guía de conexión de SparkFun](https://learn.sparkfun.com/tutorials/ad8232-heart-rate-monitor-hookup-guide/). La ficha del circuito integrado no sustituye el esquema de la tarjeta UNIT. La página de Mercado Libre no pudo abrirse desde esta sesión; se usó la descripción de compra facilitada por el usuario.

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

RGB de cátodo común por defecto, común a GND. Para ánodo común cambia `RGB_COMMON_ANODE` en `Config.h` y conecta el común según el LED. La tabla sustituye los pines de servo D2–D6 del código original: el documento usa D3/D5/D6/D10/D11.

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

Una calibración correcta se guarda en EEPROM. **La calibración del sensor anterior se invalida automáticamente** al cargar esta versión, porque los umbrales ahora corresponden a otra magnitud. Al entrar al modo de señal, primero hay que relajar por debajo del umbral de apertura para habilitar el primer cierre. Después, una envolvente por encima del umbral de cierre cierra la mano; por debajo del umbral de apertura la abre. Entre ambos umbrales conserva el estado para evitar oscilaciones por ruido.

Para recalibrar, envía `c` por el monitor serie, o mantén D8 presionado al encender/reiniciar y suéltalo al comenzar la secuencia. Conviene recalibrar si cambian electrodos, colocación o ganancia. Durante una recalibración fallida se conserva la anterior en EEPROM, pero EMG queda inhibido en esa sesión hasta repetir con éxito o cambiar de modo. El control manual sigue disponible.

Indicadores de operación: rojo = manual abierto; amarillo = manual cerrando/cerrado; verde = control por señal habilitado. Magenta intermitente indica calibración fallida, pérdida de contacto, recorte o espera de estabilización en modo de señal/calibración. Las pulsaciones y cambios de modo aceptados actualizan el indicador en la misma vuelta del bucle.

## Temporización y diagnóstico

- **Timer2:** CTC, prescaler 64, OCR2A=249; solicita una conversión cada 1 ms.
- **ADC:** A0/AVcc, prescaler 128, reloj ADC de 125 kHz. `ADC_vect` lee además LO+/LO− y actualiza el procesamiento de señal. El bucle toma una copia atómica del resultado y decide el movimiento.
- **Procesamiento:** IIR del nivel DC (`base += (raw - base)/256`, con precisión Q8), magnitud `abs(raw-base)` y media móvil de 32 magnitudes. El seguimiento DC tiene constante aproximada de 256 ms a 1 kHz; la ventana de magnitud es de 32 ms. No se promedia primero la señal alterna, porque eso cancelaría sus semiondas.
- **Timer1:** reservado a la biblioteca Servo para cinco trenes de pulsos de aproximadamente 50 Hz. Se usa su implementación, sin sobrescribir sus registros.
- **Timer0:** permanece con la configuración del núcleo Arduino para `millis()`.
- **Movimiento:** avance simultáneo de 25/1000 del recorrido cada 20 ms, con un primer paso inmediato al aceptar la orden. Recorrido ordenado completo en 780–800 ms nominales, reversible en cualquier punto. El tiempo físico debe medirse.
- **Interfaz:** lectura cada 5 ms y antirrebote estable de 10 ms. Se ajusta el planteamiento de 10/20 ms del documento porque esa combinación puede superar su límite R9 de 20 ms.
- **D2:** conmuta en cada ISR ADC. El periodo entre flancos consecutivos debe ser 1 ms; la onda completa es de **500 Hz**, no 1 kHz. Desactiva con `EMG_SAMPLE_PROBE=0` si usas D2 para otra cosa.
- **EEPROM:** contador de arranques en bytes 0–7; umbrales con firma AD8232 versión 2 (`0xE702`) y comprobación en bytes 16–23. Se conserva el contador de arranques de la versión anterior, pero se rechazan sus umbrales. El contador cambia una vez por arranque, la calibración solo al completar una nueva medición. Una escritura interrumpida puede invalidar el registro, que se rechaza en el próximo arranque; no es un registro de reinicios tolerante a fallas ni tiene reparto de desgaste.

No uses `tone()`, `analogRead()` ni reconfigures Timer1/Timer2 mientras este firmware esté activo. No se usa `analogWrite()` para los servos; Servo genera sus señales por interrupciones incluso en los pines no marcados como PWM.

Monitor serie a **115200 baud**:

- `c`: iniciar/repetir calibración.
- `p`: activar/desactivar datos de diagnóstico a 10 Hz.
- `x`: cancelar calibración y ordenar apertura; en EMG exige reposo antes de volver a cerrar. No sustituye al interruptor físico de alimentación.

Eventos: `E,milisegundos,nombre,valor`, por ejemplo `E,12345,CLOSE,25`. `BOOT` informa el contador persistente; `MODE` usa 0 para manual y 1 para EMG. `TH_OPEN` y `TH_CLOSE` informan los umbrales calibrados. Los eventos `CLOSE` permiten contar las detecciones en R6, pero no son mediciones de movimiento real.

Datos: `D,milisegundos,ADC_crudo,base_DC,envolvente,umbral_abrir,umbral_cerrar,estado,overruns,logs_descartados`. `estado` combina bits: 1 = LO+/LO− alto; 2 = recorte/rango inválido; 4 = procesamiento aún no listo; 8 = ADC sin muestras recientes. Pueden sumarse varios bits. `estado=0` solo indica que estos controles de señal pasan; no certifica que la actividad sea una contracción voluntaria.

Los datos a 10 Hz sirven para ver niveles y estados, no para reconstruir la forma de onda ECG. Los overruns cuentan solicitudes de conversión que encontraron el ADC ocupado. La UART descarta líneas si no hay espacio para evitar bloquear el control; en una prueba de registro, verifica que no se descarten líneas. Puedes compilar con `EMG_SERIAL_DEBUG=0` para desactivar UART; seguirá disponible la calibración con D8 al arranque.

Un LO alto, falta de muestras durante más de 10 ms o una muestra próxima a los rieles del AD8232 ordenan apertura en modo de señal. Por defecto se considera recorte `raw <= 5` o `raw >= 669`, calculado para sensor de 3.3 V y AVcc=5 V. `ADC_REFERENCE_MV`, `SENSOR_SUPPLY_MV` y márgenes están en `Config.h`; si se ajustan según mediciones, recalibra. El recorte invalida la ventana y exige otra estabilización de 2 s. Una falla durante la calibración la rechaza y requiere repetirla; en operación normal la señal puede recuperarse automáticamente, exigiendo reposo antes del siguiente cierre. La apertura conserva la rampa del mecanismo; no es una desconexión de potencia.

LO+/LO− dependen de la configuración física de detección del módulo. No garantizan detectar un cable de OUTPUT roto, la desconexión de la referencia RL o todos los casos de mal contacto; tampoco detectan un dedo bloqueado. F1 sigue requiriendo ensayos reales.

## Verificación

Compilación con CLI (o usa Verificar en Arduino IDE):

```powershell
arduino-cli compile --fqbn arduino:avr:uno --warnings all --build-path build/uno Inmoov_EMG
```

Pruebas de lógica con un compilador C++11; crea `build` si aún no existe:

```powershell
New-Item -ItemType Directory -Force build
g++ -std=c++11 -Wall -Wextra -Werror tests/control_test.cpp -o build/control_test.exe
.\build\control_test.exe
```

Se verificaron eliminación DC, simetría frente a semiondas positivas/negativas, independencia del offset, recorte a ambos rieles, pérdida de contacto y espera de recuperación de 2 s. Se probó también una cadena de señal alterna sintética → envolvente → calibración → cierre/apertura por pérdida de contacto. Continúan las pruebas de histéresis/rearme, pulsaciones, inversiones de movimiento, duración del recorrido ordenado, calibraciones inválidas, adquisición incompleta y desbordamiento de `millis()`.

Compilación verificada con Arduino AVR 1.8.6 y Servo 1.2.2: 9784 bytes de Flash y 621 bytes de variables globales. Las pruebas C++11 pasaron sin advertencias del código del proyecto.

| Requisito del documento | Preparación en firmware | Verificación pendiente en el montaje |
| --- | --- | --- |
| R1: latencia <=50 ms | Sin `delay()`, envolvente de 32 muestras, primer paso inmediato | Con AD8232 hay latencia del filtro analógico y del procesamiento; OUTPUT ya no es una envolvente que se pueda comparar directamente con el umbral. Definir una referencia de inicio de contracción y repetir las mediciones; <=50 ms sigue sin demostrarse |
| R2: cierre <=1 s | Rampa simultánea nominal de 800 ms | Video de 10 cierres con batería y carga indicadas |
| R3: 0–150° | Extremos de pulso configurables por dedo | Calibración con goniómetro; no confundir ángulo mecánico con pulso |
| R4/R5: tensor y autonomía | Sin medición de fuerza, corriente o batería en este circuito | Ensayo de tensor y autonomía según el documento |
| R6: detección >=90% | Calibración de envolvente, histéresis, LO+/LO− y eventos con tiempo | Control muscular experimental: 50 contracciones y 5 min de reposo; comprobar que ECG/artefactos no disparen cierres |
| R7: PWM | Servo/Timer1; extremos iniciales 1–2 ms | Periodo y ancho en los cinco pines bajo carga |
| R8: ADC 1 kHz | Timer2/ADC con salida de prueba D2 y contador de overruns | Medir intervalos de 1 ms entre flancos con servos activos |
| R9: interfaz <=20 ms | Muestreo 5 ms, antirrebote 10 ms, LED inmediato | Medir entradas y LED, incluidos rebotes reales |
| R10: estabilidad | Contador persistente de arranques | Medir buck y comparar contador antes/después de 100 ciclos |

La compilación y las pruebas de lógica no comprueban los criterios eléctricos, mecánicos ni de exactitud EMG. No se cargó el programa en una placa ni se accionaron motores desde esta sesión.
