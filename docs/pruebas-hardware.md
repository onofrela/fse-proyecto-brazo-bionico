# Protocolo de demostración y medición física

Estado: pendiente de ejecución con placa, batería y mano. Completar los resultados con capturas, sin copiar los números de simulación como mediciones reales. Registrar fecha, autor, etiqueta/commit, equipo, versión de dependencias, ajustes de `Config.h`, batería, carga y archivos de evidencia.

## Arranque, cableado y flujo

1. Revisar la tabla de pines del README y `arquitectura.md`; verificar AD8232 a 3.3 V, masas y fuente de servos. Ajustar límites con tensores sueltos antes de accionar la mano.
2. Compilar/cargar la etiqueta correspondiente. Seleccionar manual (D9 a GND) y comprobar una pulsación por cierre/apertura, RGB y los cinco servos.
3. Cortar y restablecer alimentación sin USB; confirmar arranque autónomo y apertura. Con EEPROM válida, comprobar umbrales guardados. Sin registro válido, comprobar calibración solicitada y mano abierta.
4. Medir reposo/contracción y exigir separación repetible. En EMG comprobar reposo para armar, cierre por actividad y apertura al relajar. Los datos UART a 10 Hz no reconstruyen la forma de onda.
5. Confirmar `overruns=0` y registro sin descartes durante la captura. Si se descartan líneas, repetir la captura; no completar métricas con un registro incompleto.

## Tabla de mediciones

| Caso | Procedimiento / esperado | Obtenido | Estado |
| --- | --- | --- | --- |
| R1 | 20 eventos; <=50 ms desde cruce de umbral hasta primer pulso PWM nuevo, osciloscopio. El AD8232 no entrega envolvente: acordar y registrar referencia de disparo antes del ensayo | Sin medir | Pendiente |
| R2 | Video >=60 fps, mano sin objeto, batería >=12 V; 10 cierres, promedio <=1000 ms. Guardar inicio/fin por repetición | Sin medir | Pendiente |
| R3 | Goniómetro/foto con escala, 3 repeticiones por dedo; 0–150 grados ±5 | Sin medir | Pendiente |
| R4 | Tensor de 30 cm y mismos nudos, masa de 2.5 kg por 60 s, longitud 5 min después; sin ruptura, elongación <=2% | Sin medir | Pendiente |
| R5 | LiPo 3S 1000 mAh a 12.6 V, apertura+cierre cada 10 s, fin 10.5 V; >=90 min, voltaje cada 10 min; corriente promedio <=1 A en riel de 5 V | Sin medir | Pendiente |
| R6 | 50 contracciones y 5 min reposo; >=45 detectadas, <=5 falsas en el reposo y revisar distribución por minuto <=1 | Sin medir | Pendiente |
| R7 | Capturar D3/D5/D6/D10/D11 bajo movimiento; periodos entre 19.8 y 20.2 ms y error de ancho <=20 us contra orden | Sin medir | Pendiente |
| R8 | Capturar D2, servos activos: 0.99–1.01 ms entre flancos consecutivos. Si el instrumento muestra onda completa, debe indicar ~500 Hz, no 1000 Hz | Sin medir | Pendiente |
| R9 | Osciloscopio CH1 botón/selector y CH2 LED; incluir rebotes y distintas fases; todos <=20 ms | Sin medir | Pendiente |
| R10 | 100 arranques simultáneos de los 5 servos; buck entre 4.75 y 5.25 V y contador BOOT sin cambio dentro de la sesión | Sin medir | Pendiente |
| F1 físico | Despegar y recolocar electrodo: apertura si LO detecta desconexión, no cierre sostenido por ruido, >=2000 muestras estables y reposo antes del cierre; recuperación sin reset | Sin medir | Pendiente |
| F2 mecánico | Ensayo de dedo bloqueado por 10 s con montaje adecuado: riel >=4.75 V, sin reset ni daño/elongación permanente; al soltar responde. No hay detección de bloqueo en firmware | Sin medir | Pendiente |
| F3 físico | Desconectar/reconectar OUTPUT y observar ADC/LO; documentar si el hardware detecta la desconexión. Un cable flotante puede quedar en rango, por lo que no se garantiza detección | Sin medir | Pendiente |

Para R2, calcular cada duración como `(cuadro_final - cuadro_inicial) / fps`; registrar las diez duraciones y su promedio. Para R5, promediar corriente sobre el ciclo completo de 10 s, incluyendo agarre y reposo; la capacidad de batería sola no demuestra autonomía. Para R10, conservar el mismo encendido durante los 100 ciclos de movimiento; el reinicio deliberado previo incrementa BOOT y no debe confundirse con una falla.

La prueba F2-SW de la tabla automatizada significa “ADC sin muestras”; no es el F2 mecánico. Las pruebas SW de contacto no prueban detección real de todos los cables o electrodos. En particular, la señal de ECG y los artefactos pueden parecer actividad; R6 necesita evidencia con la tarjeta real.

## Material para entregar

Preparar un informe técnico con cableado, configuración, diagrama y resultados físicos. Adjuntar la etiqueta o commit evaluado, logs, capturas originales y registros de prueba con fecha y autor. El código y la documentación Markdown se complementan con las mediciones y la demostración del montaje.

## Archivos de apoyo del equipo

- `Prueba_Servo_Serial/`: comprobar respuesta de un servo individual a angulos enviados por UART, antes de integrarlo a la mano. Ver su README para pin, baudios y entradas.
- `Prueba_AD8232_Serial/`: observar ADC crudo y LO+/LO- para entender niveles y contacto antes de usar filtros y umbrales. Ver su README para escala y conexiones.

Cada sketch reemplaza temporalmente al firmware principal en el UNO. Se compilan por separado. La lectura ADC de apoyo es de 500 Hz nominales; no demuestra el requisito R8 de 1 kHz del firmware integrado. Una confirmacion serial del servo tampoco demuestra rango articular ni tiempo de cierre de la mano.
