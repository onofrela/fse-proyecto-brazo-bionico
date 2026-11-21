# Analizador y control por tres niveles

Ejecuta `iniciar.bat` o `python analizador.py --port COM3`. Usa Python 3.10+ con Tk, pyserial y matplotlib (`requirements.txt`). Cierra el monitor serie antes de iniciar.

FW8 controla sin calibracion. El promedio de 200 muestras de envolvente (~200 ms) ordena 0 grados si es menor que 5, 90 si es desde 5 y menor que 10, y 180 si es desde 10. Los cinco dedos se mueven juntos con rampa.

Iniciar graba CSV de senal y eventos; Detener genera PNG y resumen, pero no detiene motores. Pausar / abrir envia x; Activar control envia c y reanuda. La pantalla muestra el promedio y el objetivo ordenado. La secuencia de calibracion solo se usa al conectar firmware anterior FW6/7.

`MEAN_X10` guarda la media en decimas en eventos.csv; `TARGET_DEG` es un destino, no una medida fisica. Las lineas D conservan envolvente instantanea y fronteras 5/10. Abrir CSV no vuelve a filtrar la captura.

CLI: `python analizador.py --headless --port COM3 --seconds 30 --output registros`. Demo: `--headless --demo --seconds 20`. Reabrir: `--headless --csv registros/registro/senal.csv`.
