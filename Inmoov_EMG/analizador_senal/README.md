# Analizador y control por tres niveles

Ejecuta `iniciar.bat` o `python analizador.py --port COM3`. Usa Python 3.10+ con Tk, pyserial y matplotlib (`requirements.txt`). Cierra el monitor serie antes de iniciar.

FW8 controla sin calibracion. El promedio de 200 muestras de envolvente (~200 ms) ordena 0 grados si es menor que 5, 90 si es desde 5 y menor que 10, y 180 si es desde 10. Los cinco dedos se mueven juntos con rampa.

Iniciar graba CSV de senal y eventos; Detener genera PNG y resumen, pero no detiene motores. Pausar / abrir envia `x`; Activar control envia `c` y reanuda. La pantalla muestra el promedio y el objetivo ordenado. Los botones de control se habilitan al recibir `E,...,FW,8` durante una captura USB. Solo se admite control FW8, sin secuencia de calibracion.

`MEAN_X10` guarda la media en decimas en eventos.csv; `TARGET_DEG` es un destino, no una medida fisica. Las lineas D conservan envolvente instantanea y fronteras 5/10. Abrir CSV no vuelve a filtrar la captura.

El protocolo conserva `D,ms,raw,baseline,envelope,threshold_open,threshold_close,flags,adc_overruns,dropped_logs` y `E,ms,nombre,valor`. Al recibir datos se consulta el estado con `?`; si no hay flujo inicial, se envia `p` una sola vez para habilitarlo. Los eventos desconocidos se guardan sin interpretacion especial.

`senal.csv` conserva sus columnas actuales: `host_utc`, `session`, `device_ms`, `time_s`, `raw_adc`, `baseline_adc`, `envelope_adc`, `threshold_open_adc`, `threshold_close_adc`, `flags`, `adc_overruns`, `dropped_logs`, `adc_reference_v`, `raw_v`, `baseline_v`, `envelope_v`, `leads_off`, `clipped`, `not_ready`, `stale`, `data_loss`, `hardware_fault`. Los indicadores se derivan de `flags`; las dos ultimas columnas son opcionales al leer. `eventos.csv` conserva `host_utc,session,device_ms,time_s,name,value`. El resumen cuenta cierres por eventos `TARGET_DEG,180`.

La demo genera datos sinteticos de los tres niveles y fallas; no reproduce el filtro ni las 200 muestras internas, ni valida movimiento fisico. El bloque real de ~200 ms implica que R1 (latencia <=50 ms) no se cumple por diseno. El motivo y la decision estan documentados en [la bitacora](../../docs/bitacora.md).

CLI: `python analizador.py --headless --port COM3 --seconds 30 --output registros`. Demo: `--headless --demo --seconds 20`. Reabrir: `--headless --csv registros/registro/senal.csv`.
