# Prueba de un servo desde el Monitor Serie

1. Abre `Prueba_Servo_Serial.ino` en Arduino IDE y selecciona Arduino Uno y su puerto.
2. Conecta la señal del servo a **D3**. Alimenta el servo con una fuente adecuada a su voltaje y corriente, y une el GND de la fuente con GND del Arduino.
3. Carga el programa. El servo recibe una posición inicial de **90°** al arrancar.
4. Abre el Monitor Serie a **9600 baudios** y selecciona **Nueva línea** o **Ambos NL y CR** como terminación.
5. Escribe un entero, por ejemplo `45`, y pulsa **Enviar**. El programa acepta de **0 a 180** y confirma el ángulo enviado.

Las entradas inválidas conservan la posición anterior. El mensaje confirma la orden enviada; no mide la posición real del eje. Usa un servo de posición: en uno de rotación continua, el valor controla velocidad y sentido.

Para esta prueba, deja el servo sin tensores ni carga y verifica sus límites mecánicos antes de probar los extremos. No alimentes el servo desde el pin 5V o el USB del Arduino. Puedes cambiar `PIN_SERVO` y `ANGULO_INICIAL` al principio del programa.
