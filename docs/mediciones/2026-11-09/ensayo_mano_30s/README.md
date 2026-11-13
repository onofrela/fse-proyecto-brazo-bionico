# Ensayo de reposo, contracción y movimiento de mano

Registro original de 305 muestras a 10 Hz, 30,4 s. El operador indica aproximadamente 10 s de reposo, 10 s apretando y 10 s abriendo/cerrando. Para el análisis se toma la primera muestra como t=0. El tiempo de dispositivo empieza en 1,368 s; los límites manuales tienen incertidumbre y no son marcas sincronizadas.

| Bloque relativo | Mediana envolvente | P95 | Máximo | Sin banderas |
| --- | --- | --- | --- | --- |
| 0–10 s, reposo | 0 | 3 | 6 | 0/100 |
| 10–20 s, contracción sostenida | 4 | 9 | 13 | 0/100 |
| 20–30 s, abrir y cerrar | 3,5 | 12 | 17 | 6/100 |

Las cinco muestras restantes quedan fuera de los tres bloques. El total contiene 299 muestras en estabilización y 295 con bit de pérdida, pero cero incrementos de desbordamiento ADC o descarte UART. Se conserva el archivo completo; no se ocultan ni corrigen sus banderas. Esta captura no permite aceptar una calibración ni recalcular los filtros de 1 kHz.

Se identificó una causa de software para pérdidas falsas: Application comparaba con resta sin signo la hora de entrada de tick frente a una muestra publicada después por la ISR. La prueba TIME-ISR falla con el código anterior y pasa con la comparación de edad firmada, incluso al desbordarse millis. Las muestras realmente antiguas y las pérdidas reales siguen rechazándose. La calidad física del sensor continúa pendiente.

Los umbrales 3/5 se usan solo para explorar sensibilidad en un replay que mantiene cada instantánea durante 100 ms y omite banderas exclusivamente en esa simulación. Da cero cierres en reposo y cuatro aperturas/cierres completos ordenados. No demuestra el filtrado original, exactitud por gesto ni recorrido mecánico. La calibración real debe repetirse con adquisición sana; no se precarga ese perfil en EEPROM.

El firmware conserva la arquitectura modular, adapta los umbrales sin pisos 8/12, usa subida de 120 ms para rechazar un pico aislado de 100 ms y permite terminar ambas rampas. Outputs envía 544/2400 us, equivalentes a Servo.write(0)/Servo.write(180) con Servo 1.2.2 por defecto. No existe realimentación de posición física.

`senal.csv`, `eventos.csv`, `resumen.json` y `grafica.png` son originales. `analisis.json` contiene los cálculos y `sha256.json` identifica sus bytes. El CSV previo no se ha filtrado nuevamente.
