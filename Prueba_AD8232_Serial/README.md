# Medicion directa del AD8232

Programa independiente para Arduino Uno y el AD8232 de UNIT Electronics mencionado en el proyecto. Lee OUTPUT sin calibracion, filtrado digital, umbrales ni control de servomotores. No lee ni modifica la calibracion guardada en EEPROM.

## Conexiones

| AD8232 | Arduino Uno |
| --- | --- |
| 3.3V | 3.3V |
| GND | GND |
| OUTPUT | A0 |
| LO+ | A1 |
| LO- | A2 |
| SDN | 3.3V para mantener habilitado el modulo |

Alimenta el AD8232 a **3.3 V**, como indica el proyecto. Deja los servos sin alimentacion durante esta prueba.

## Uso

1. Abre `Prueba_AD8232_Serial.ino` en Arduino IDE.
2. Selecciona Arduino Uno y su puerto, y carga el programa.
3. Abre el **Serial Plotter** para ver la grafica, o el **Monitor Serie** para ver los numeros, a **115200 baudios**. No necesitas enviar comandos.

Cada linea tiene este formato:

```text
ADC:340 LO_mas:0 LO_menos:0
```

Los campos se separan con tabulaciones. `ADC` es la lectura directa de 10 bits (0 a 1023). Con referencia de 5 V y salida del modulo de 0 a 3.3 V, el intervalo esperado es aproximadamente 0 a 675 cuentas. El voltaje aproximado en OUTPUT se calcula como `ADC * 5.0 / 1023`; depende del voltaje real de referencia del Arduino.

`LO_mas` y `LO_menos` valen 1 cuando la respectiva salida del modulo indica perdida de contacto, y 0 en caso contrario. Se siguen mostrando las lecturas cuando hay perdida de contacto; esas lecturas no deben interpretarse como una señal valida de los electrodos. Por su escala 0/1, estas dos curvas quedan cerca de la parte inferior de la grafica.

La frecuencia nominal de lectura es **500 muestras por segundo**, temporizada con `micros()`. No es una adquisicion por interrupciones ni garantiza tiempos exactos. La señal conserva el nivel DC y los filtros analogicos propios de la tarjeta: el programa no obtiene una envolvente muscular. El AD8232 es un acondicionador ECG; las limitaciones para medir actividad muscular se explican en el README principal.

Con electrodos en el cuerpo, sigue las precauciones de aislamiento del README principal: alimentacion por bateria e interfaz de datos aislada apropiadamente para visualizar en la computadora. Este programa no aporta aislamiento electrico.
