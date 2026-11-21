# Analizador y control por tres niveles

Ejecuta `iniciar.bat` o `python analizador.py --port COM3`. Usa Python 3.10+ con Tk, pyserial, matplotlib y cryptography (`python -m pip install -r requirements.txt`). Cierra el monitor serie antes de iniciar.

La captura USB requiere cargar la variante **depuracion** (`EMG_SERIAL_DEBUG=1`); la versión normal no inicia ni emite por UART. Desde la raíz del repo, `python scripts/build.py` genera `build/normal` y `build/depuracion`; carga esta última siguiendo [las instrucciones del firmware](../../README.md#carga-y-primera-prueba). En Arduino IDE, añade `#define EMG_SERIAL_DEBUG 1` al principio del `.ino`, antes de `Config.h`, y sube. La UART del UNO no ofrece cifrado ni autenticación; vuelve a cargar la versión normal al terminar la depuración. El cifrado de archivos del analizador protege el almacenamiento, no este enlace.

FW8 controla sin calibracion. El promedio de 200 muestras de envolvente (~200 ms) ordena 0 grados si es menor que 5, 90 si es desde 5 y menor que 10, y 180 si es desde 10. Los cinco dedos se mueven juntos con rampa.

Iniciar solicita consentimiento y contraseña antes de grabar señal y eventos; Detener cierra el registro, genera resumen y manifiesto, pero no detiene motores. No se genera PNG automáticamente: Exportar gráfica requiere aceptar una copia sin cifrar en la ruta elegida. Pausar / abrir envia `x`; Activar control envia `c` y reanuda. La pantalla muestra el promedio y el objetivo ordenado. Los botones de control se habilitan al recibir `E,...,FW,8` durante una captura USB. Solo se admite control FW8, sin secuencia de calibracion.

`MEAN_X10` guarda la media en decimas en eventos.csv; `TARGET_DEG` es un destino, no una medida fisica. Las lineas D conservan envolvente instantanea y fronteras 5/10. Abrir CSV no vuelve a filtrar la captura.

El protocolo conserva `D,ms,raw,baseline,envelope,threshold_open,threshold_close,flags,adc_overruns,dropped_logs` y `E,ms,nombre,valor`. Al recibir datos se consulta el estado con `?`; si no hay flujo inicial, se envia `p` una sola vez para habilitarlo. Los eventos desconocidos se guardan sin interpretacion especial.

El contenido de `senal.csv` tiene las columnas `session`, `device_ms`, `time_s`, `raw_adc`, `baseline_adc`, `envelope_adc`, `threshold_open_adc`, `threshold_close_adc`, `flags`, `adc_overruns`, `dropped_logs`, `adc_reference_v`, `raw_v`, `baseline_v`, `envelope_v`, `leads_off`, `clipped`, `not_ready`, `stale`, `data_loss`, `hardware_fault`. Los indicadores se derivan de `flags`; las dos ultimas columnas son opcionales al leer CSV antiguos. `eventos.csv` tiene `session,device_ms,time_s,name,value`. `session` cuenta reinicios de placa, no identifica a la persona. El resumen cuenta cierres por eventos `TARGET_DEG,180`. `--incluir-hora` agrega `host_utc` a ambos CSV; por defecto esa columna no se guarda. La lectura admite CSV históricos con ella y nuevos sin ella, sin volver a filtrar los valores.

La demo genera datos sinteticos de los tres niveles y fallas; no reproduce el filtro ni las 200 muestras internas, ni valida movimiento fisico. El bloque real de ~200 ms implica que R1 (latencia <=50 ms) no se cumple por diseno. El motivo y la decision estan documentados en [la bitacora](../../docs/bitacora.md).

## Datos biométricos, consentimiento y ubicación

El AD8232 es un módulo de ECG. La señal cardíaca puede identificar a la persona; señal, eventos, etiquetas y estadísticas se tratan como datos biométricos/de salud. El repositorio es público: no subas capturas, gráficas ni contraseñas. Las capturas reales de `docs/mediciones/` se retiraron del árbol actual sin reescribir el historial; las versiones antiguas todavía las contienen. Las pruebas actuales usan exclusivamente datos sintéticos.

La salida predeterminada es `Path.home() / "registros-emg"`, fuera del repositorio, con subcarpetas `registro_<UUID aleatorio>`, sin nombre de persona ni fecha en el identificador. `--output RUTA` permite elegir otra base; usa una carpeta privada fuera del repo y revisa si se sincroniza con terceros. Los tiempos `device_ms`/`time_s` son relativos; el sistema de archivos conserva sus propias fechas aunque no se incluya `host_utc`. No se guarda el nombre del puerto en el resumen.

La ventana explica qué se guarda, dónde y con qué protección, y pide aceptar cada sesión (también la demo). Cancelar el aviso o la contraseña no inicia captura. En `--headless` se muestra el aviso y se exige `--acepto-guardar`; la bandera no sustituye la autorización de la persona de la que se obtiene la señal.

```powershell
python analizador.py --headless --port COM3 --seconds 30 --acepto-guardar
python analizador.py --headless --demo --seconds 20 --acepto-guardar
```

## Cifrado e integridad

`senal.csv`, `eventos.csv` y `resumen.json` conservan esos nombres, pero sus bytes están cifrados por defecto. No se pueden abrir directamente en Excel. Se usa AES-256-GCM con etiqueta de 128 bits; cada archivo tiene sal aleatoria de 16 bytes y nonce aleatorio de 12 bytes en una cabecera `EMG-AES256-GCM-1`. Scrypt deriva claves de 32 bytes con N=16384, r=8, p=1 y separación de dominio entre AES y HMAC. La cabecera y el nombre del archivo son datos autenticados de GCM. El CSV se cifra conforme se escribe, sin temporales en claro; la etiqueta se completa al cerrar.

La contraseña se pide por sesión, con confirmación al crearla, mediante `getpass` en terminal o un diálogo oculto. Usa una frase larga y única por sesión y consérvala fuera de estos registros. La contraseña y las claves no se guardan en disco ni en el manifiesto. Si se pierden, no se recupera el contenido. Para pruebas automatizadas se admite `EMG_PASSPHRASE` en el entorno del proceso; no la pongas en archivos `.env`, scripts ni argumentos de línea de comandos.

`manifiesto.hmac.json` contiene una sal independiente, HMAC-SHA256 de los bytes de cada uno de los tres archivos y HMAC del propio contenido del manifiesto. Su nombre lo distingue del manifiesto de origen de los datos sintéticos de prueba. No contiene estadísticas ni señal. Se verifica una lista fija de archivos: borrar, truncar o modificar uno, alterar el manifiesto o usar otra contraseña produce un error y código de salida distinto de cero. No demuestra autoría ni detecta sustituir una sesión completa por una copia antigua válida. Una captura que termina sin cierre normal puede quedar incompleta: sin etiqueta/manifiesto válidos se rechaza, nunca se entrega texto parcial como verificado.

Desde la carpeta del analizador, sustituye las rutas por las de tu sesión:

```powershell
python analizador.py --verificar "$HOME/registros-emg/registro_ID"
python analizador.py --descifrar "$HOME/registros-emg/registro_ID/senal.csv"
python analizador.py --headless --csv "$HOME/registros-emg/registro_ID/senal.csv"
```

`--descifrar ARCHIVO` verifica primero toda la sesión y emite el contenido autenticado por la salida estándar; no crea una copia en disco. Una redirección crea una copia en claro bajo tu responsabilidad; el shell puede cambiar su codificación. La ventana abre los registros cifrados directamente y pide contraseña. `--headless --csv` imprime estadísticas y no crea archivos nuevos. Los CSV antiguos sin manifiesto se pueden leer, pero carecen de autenticación.

`--sin-cifrar` es una excepción explícita: muestra una advertencia y guarda los tres archivos en claro. Aun así exige consentimiento y contraseña para el manifiesto HMAC; no ofrece confidencialidad. Las gráficas exportadas explícitamente también quedan en claro. Detener y verificar no exportan gráficas.

Referencia del formato: [AES/GCM en cryptography](https://cryptography.io/en/latest/hazmat/primitives/symmetric-encryption/) y [derivación scrypt](https://cryptography.io/en/latest/hazmat/primitives/key-derivation-functions/).

## Conservación y borrado

Al iniciar el analizador se revisa la base de salida elegida. Se avisa de carpetas de sesión reconocidas sin cambios desde hace más de 30 días y se ofrece borrarlas completas, mostrando las rutas. Sólo se borran tras confirmar en la ventana o escribir `BORRAR` en una terminal interactiva; cancelar, EOF o ejecución sin terminal conserva todo. Se omiten enlaces, carpetas anidadas y archivos ajenos. La revisión no afecta ubicaciones de otras bases: inicia con `--output RUTA` para revisarlas.

También puedes borrar manualmente la carpeta completa de una sesión desde el explorador. Revisa además exportaciones, copias de seguridad, sincronización y papelera. Este borrado no garantiza sobreescritura física del disco ni retira las capturas del historial público anterior. No se ha reescrito ese historial por decisión del equipo.
