# Ajuste del control de mano

Se analizó el registro original de 305 muestras y 30,4 s con el protocolo indicado: aproximadamente 10 s de reposo, 10 s apretando y 10 s abriendo/cerrando. Los límites se cuentan desde la primera muestra. La gráfica anotada conserva los valores originales.

| Intervalo | Mediana de envolvente | P95 | Máximo |
| --- | --- | --- | --- |
| Reposo | 0 | 3 | 6 |
| Apretando | 4 | 9 | 13 |
| Abrir y cerrar | 3,5 | 12 | 17 |

Los anteriores pisos de 8/12 eran altos para este ensayo. Además, 299/305 muestras estaban en estabilización y 295 indicaban pérdida, sin desbordamientos ADC. Se reprodujo y corrigió un error de tiempo: una muestra publicada por la ISR después de la hora de entrada del bucle parecía muy antigua por una resta sin signo. La prueba falla antes del cambio y pasa después, incluido el desbordamiento de millis; las muestras realmente antiguas siguen rechazándose.

## Cambios aplicados

- Firmware FW=7 y analizador compatible con FW=6/7. Arquitectura modular conservada; adquisición e interrupciones sin filtros ni procesamiento de gestos.
- Calibración sin pisos fijos 8/12: usa reposo y actividad medidos, exige separación y mantiene dos cuentas de histéresis mínima. No se acepta el registro con fallas como calibración ni se precargan sus valores en EEPROM. Firma nueva 0xE707.
- Subida de ponderación de 120 ms para rechazar picos aislados de 100 ms. Cierre por actividad y apertura por relajación; mínimo 900 ms en cada sentido normal para completar ambas rampas.
- Cinco servos con extremos 544/2400 us, equivalentes a órdenes 0°/180° en Servo 1.2.2 por defecto. El recorrido físico depende del servo y del montaje y no se mide con este circuito.

## Verificación

Pasaron 40 casos reportados y las 10 pruebas Python incluidas en ellos. El replay exploratorio del CSV, manteniendo cada instantánea 100 ms y omitiendo sus banderas exclusivamente en esa simulación, produce cero cierres en reposo y cuatro ciclos completos ordenados. No representa una reconstrucción de la señal de 1 kHz ni demuestra movimiento físico. El rechazo de picos, las dos rampas, fallas reales y recuperación se probaron también con entradas sintéticas.

Compilación UNO: 14598 bytes Flash y 1383 bytes SRAM global, con 665 disponibles para pila y temporales. Firmware cargado y verificado en COM3. Una captura real independiente posterior registró 119 muestras a 10 Hz, 112 válidas y 7 de estabilización inicial; cero pérdidas, desbordamientos ADC, descartes UART o fallas de hardware durante ese ensayo. Esta captura comprueba recepción y salud de adquisición, no es el protocolo de gestos del operador.

La inestabilidad física del sensor sigue en investigación; este hallazgo de software no confirma ni descarta soldaduras, módulo, jumpers u otras causas físicas.

## Uso

Inicia el analizador y pulsa Calibrar (22 s) cuando estés preparado. Sigue el reposo y las contracciones/relajaciones que indique. Al finalizar y aceptar la calibración, relaja para rearmar; después aprieta para cerrar y relaja para abrir. Si falla la separación, revisa señal y montaje y repite. Las órdenes son de mano completa, sin identificar dedos.
