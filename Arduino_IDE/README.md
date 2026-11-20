# Versión de un solo archivo para Arduino IDE

Abre [Inmoov_EMG_Servos_Invertidos.ino](Inmoov_EMG_Servos_Invertidos/Inmoov_EMG_Servos_Invertidos.ino). Su carpeta debe conservar el mismo nombre que el archivo: `Inmoov_EMG_Servos_Invertidos`.

1. Instala **Arduino AVR Boards** y la biblioteca **Servo** en Arduino IDE. La compilación de referencia usa AVR 1.8.6 y Servo 1.2.2; EEPROM viene con el núcleo AVR.
2. Selecciona **Arduino Uno** y el puerto de la placa (COM3 en la última carga).
3. Pulsa **Subir** en la ventana de este sketch.

El archivo incluye todo el firmware y su configuración. Medio (D6), anular (D10) y meñique (D11) invierten las órdenes: 0° → 180°, 90° → 90° y 180° → 0°. Pulgar (D3) e índice (D5) conservan su dirección. Para comprobarlo en modo manual, conecta D9 a GND; cada pulsación de D8 alterna apertura y cierre.

Este archivo es idéntico a [Inmoov_EMG.ino](../Inmoov_EMG/Inmoov_EMG.ino); ambos contienen el firmware completo. Para actualizar ambos después de modificar los módulos `.h` y `.cpp`, ejecuta `python scripts/export_ino.py`. En el Monitor Serie a 115200 baudios, envía `?` y comprueba `SERVO_REVERSE_MASK,28` para identificar la inversión cargada. Consulta el [README del proyecto](../README.md) para el cableado y funcionamiento completos.
