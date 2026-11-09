# Analizador de mano completa

Desde esta carpeta: `python -m pip install -r requirements.txt` y `python analizador.py`, o abre `iniciar.bat`. Python 3.10+ con Tcl/Tk. Elige Sensor USB, el puerto del UNO y referencia ADC de 5 V por defecto. Cierra el monitor serie antes de iniciar.

El firmware modular informa `FW=6`. Sólo después se habilitan Calibrar y Cancelar/abrir. Calibrar guía 3 s de preparación, 6 s de reposo, 3 s de preparación y 10 s de contracciones/relajación. No empieza al abrir USB. Una calibración válida permite controlar toda la mano: actividad ponderada para cerrar, reposo para abrir. No identifica dedos ni mide fuerza.

Iniciar guarda CSV de señal y eventos. Detener genera PNG y estadísticas en `registros/registro_FECHA_HORA`; sólo termina el registro, sin desactivar motores. Cancelar/abrir envía `x`, solicita apertura y exige reposo para rearmar. Las marcas REST/HAND/END son locales. El porcentaje ACTIVITY proviene del módulo Decision; no es fuerza ni posición física.

Abrir CSV muestra la envolvente guardada sin aplicar filtros nuevos retrospectivamente. La adquisición interna es de 1 kHz y la telemetría es de 10 Hz. Los estados incluyen contacto (1), recorte (2), espera (4), muestra antigua (8), datos perdidos (16) y hardware (32). Los CSV antiguos siguen siendo legibles; los nuevos añaden `data_loss` y `hardware_fault`.

CLI: `python analizador.py --headless --port COM3 --seconds 60`; demo: `--headless --demo --seconds 20`; regenerar gráfica: `--headless --csv RUTA/senal.csv`. `--output` cambia el destino. Demo no transmite órdenes a hardware. Pruebas: `python -m unittest discover -s tests -v`.

Consulta el README general, `docs/control-senal.md` y `docs/mediciones/2026-11-09/README.md` para montaje, evidencia y límites.
