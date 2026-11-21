# Control fijo por promedio de envolvente

FW=8 reemplaza el control calibrado en el programa principal. No solicita calibracion ni utiliza los umbrales guardados en EEPROM. Los cinco dedos comparten el mismo destino.

En `D,tiempo,ADC,base,envolvente,...` se usa **envolvente**: el tercer valor del sensor despues del tiempo. No se clasifican el tiempo, ADC crudo, base ni las lineas E.

| Promedio de envolvente | Orden |
| --- | --- |
| Menor que 5 (lecturas enteras 0 a 4) | 0 grados |
| Desde 5 y menor que 10 (5 a 9) | 90 grados |
| Desde 10 | 180 grados |

Decision acumula bloques de 200 muestras nuevas y sanas de envolvente. A 1 kHz son aproximadamente 200 ms. Cada bloque se clasifica por su media aritmetica exacta, comparando sumas; no se redondea 4.995 a 5 ni 9.995 a 10 antes de decidir. Entre bloques conserva el ultimo destino. Antes del primer bloque completo permanece en 0. No se promedian muestras invalidas ni se rellenan huecos con ceros.

Signal mantiene el filtro digital, la rectificacion y su media de 32 muestras. El promedio de Decision se aplica sobre esa envolvente ya procesada. No introduce filtros en la ISR ni reconstruye la senal original a partir del CSV de 10 Hz.

Motion recibe posiciones normalizadas 0, 500 y 1000 y conserva la rampa de 25 unidades cada 20 ms. Outputs las convierte a 544, 1472 y 2400 us, equivalentes a las ordenes 0, 90 y 180 grados con Servo por defecto. No hay realimentacion del angulo fisico. Un nuevo bloque puede cambiar el destino durante la rampa; no existe la retencion de 900 ms de la revision anterior.

Una falla de contacto, recorte, muestra antigua, perdida de adquisicion o fallo de arranque del ADC vacia el promedio y solicita apertura en modo EMG. Se conserva la estabilizacion de 2 s de Signal y luego se exige un bloque nuevo completo. La comparacion firmada de edad evita falsas perdidas si la ISR publica despues de la hora de entrada de tick.

El control fijo esta activo al arrancar. `x` pausa y abre; `c` reactiva el control, sin calibrar; `p` conmuta telemetria y `?` consulta estado. D9/D8 conservan modo manual y alternancia entre extremos. Detener el registro no pausa motores: usa Pausar / abrir.

El analizador muestra promedio y objetivo, desactiva la secuencia de calibracion en FW8 y cambia el boton a Activar control. `MEAN_X10` registra la media en decimas, `TARGET_DEG` registra el destino y `CONTROL_PAUSED` informa pausa. Se conservan las tramas D y la lectura de CSV anteriores; los campos de umbrales muestran las fronteras fijas 5/10.

Los limites fijos son una simplificacion solicitada para probar el montaje. La variabilidad fisica del sensor sigue [en investigacion](diagnostico-sensor.md). Promediar reduce el efecto de fluctuaciones breves; no demuestra que todo ruido ambiental o cardiaco haya sido eliminado.
