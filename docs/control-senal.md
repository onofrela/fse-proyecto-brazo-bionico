# Control de mano completa a partir de la envolvente

Esta revisión integra las [mediciones originales](mediciones/2026-11-09/README.md) en la arquitectura modular. No restaura el procesamiento monolítico ni devuelve filtros a las interrupciones. El sketch sigue delegando en Application.

## Incidencia física pendiente

El sensor presenta variabilidad de causa desconocida. La ponderación puede mejorar la respuesta lógica, pero no corrige una soldadura defectuosa, un módulo averiado ni un jumper intermitente. Esas posibilidades y otras condiciones del montaje siguen [en investigación](diagnostico-sensor.md). No se afirma que ninguna de ellas esté confirmada.

## Responsabilidades

- Acquisition y SampleQueue capturan/publican muestras completas con su hora; permanecen sin filtrado ni escritura serie/EEPROM.
- Signal administra la salud y el centrado DC; SignalFilter ejecuta HP30, rechazo de 60 Hz Q20 y LP200, con estados Q6 y coeficientes Q12; Signal rectifica y promedia 32 muestras antes de redondear.
- Calibration mide reposo y actividad sin decidir movimientos ni escribir EEPROM.
- Decision pondera la envolvente y produce una orden binaria común.
- Motion define el recorrido ordenado; Outputs aplica los cinco pulsos y sus límites individuales.
- Application coordina estados y fallas, con Storage incremental y Diagnostics de baja prioridad. Ningún driver decide qué gesto realizar.

## Interpretación y calibración

La trama `D,tiempo,ADC,base,envolvente,abrir,cerrar,estado,perdidas,descartes` conserva su formato. En las capturas hay envolventes de 4–9 y actividad que sube a dos cifras, pero no se usa la cantidad de dígitos como clasificador universal. Los umbrales `0,0` indican ausencia de calibración válida en esas tramas.

La calibración se solicita con `c`, desde la aplicación o con D8 presionado al encender. Sin registro válido el control EMG permanece abierto y espera la solicitud. No comienza por sorpresa al abrir USB. Con una calibración guardada, el firmware inicia sin PC; la variante sin UART sigue permitiendo calibrar con D8.

Secuencia: preparación 3 s, reposo 6 s, preparación 3 s y actividad repetida 10 s. Se toman histogramas a 50 Hz de la envolvente procesada a 1 kHz. P95 de reposo, su dispersión frente a la mediana y P70 de actividad determinan si hay separación. Se rechazan señal inválida, adquisición insuficiente, rango excedido o superposición.

La apertura parte de P95 + 10% de la separación y el cierre de P95 + 25%, respetando márgenes y referencias mínimas de 8/12 cuentas. Si el reposo es mayor, suben ambos; la actividad debe superar el umbral final. No se copian esos números como calibración válida ni se fuerza éxito. La firma `0xE706` invalida los registros anteriores y exige recalibrar. Storage conserva el guardado verificado, por pasos, en bytes 16–23 y el contador en 0–7.

## Ponderación y movimiento

La envolvente se normaliza entre umbrales a 0–100%. Suavizado asimétrico: subida 50 ms y caída 180 ms. Tras 100 ms de reposo para rearmar, un nivel de 65% ordena cierre completo. Para abrir se exige 20% o menos, envolvente bajo apertura y 250 ms de relajación. El cierre se mantiene al menos 900 ms para permitir completar la rampa nominal de 780–800 ms.

Esa retención sólo aplica a relajación normal: contacto perdido, recorte, datos antiguos o cola llena solicitan apertura inmediatamente, aunque la rampa física de apertura conserva sus límites. No es un corte de potencia ni detección de bloqueo mecánico.

La ponderación usa `RawSample.capturedMs`; la rampa usa la hora del bucle. La prueba IPC-TIME comprueba igual ponderación al extraer una o cinco muestras por vuelta, sin acelerar los servos al vaciar la cola.

Los cinco dedos comparten la misma orden. Relajar significa abrir: no se clasifican extensión y flexión como señales independientes, ni se identifica el dedo. La amplitud decide si activar, no limita el recorrido a una fracción. Se prioriza completar un movimiento frente a seguir cada fluctuación rápida.

## Telemetría y programa de escritorio

Firmware `FW=6`. `?` consulta versión, banda, validez de calibración, modo de mano completa y percentiles. ACTIVITY informa el nivel interno 0–1000; no es fuerza ni posición medida. Diagnostics intercala esos eventos sin esperar UART y mantiene las tramas D a 10 Hz. El software de escritorio acepta los seis bits de falla, añade columnas de pérdida/hardware y sigue abriendo los CSV antiguos.

Desde `Inmoov_EMG/analizador_senal`: instala `requirements.txt` y ejecuta `iniciar.bat` o `python analizador.py`. Selecciona Sensor USB y el puerto correcto, con referencia ADC coincidente con Config.h (5 V por defecto). Cierra el monitor serie, inicia, calibra cuando estés preparado y relaja antes de probar cierres. Las marcas REST/HAND/END se guardan localmente en eventos.csv. Detener sólo termina el registro; Cancelar/abrir envía x.

## Límites y pruebas pendientes

Los filtros atenúan ciertos componentes de interferencia y parte del ECG; no garantizan eliminarlo todo. La banda analógica del AD8232 depende de su tarjeta. El CSV de 10 Hz no permite demostrar el rechazo de 60 Hz ni reconstruir la onda de 1 kHz. No se recalculan los archivos originales como si ya hubieran pasado por el filtro nuevo.

El ensayo sintético actual ordena el primer paso a los 71 ms y el recorrido en 780 ms. El primero no cumple ni demuestra el objetivo R1 de 50 ms. El replay de instantáneas llega a 1000/1000 ordenados, pero no mide movimiento real. Faltan ensayos de filtros, calibración, PWM, consumo y mecánica con la placa. No se cargó esta revisión ni se accionaron servos durante la integración.
