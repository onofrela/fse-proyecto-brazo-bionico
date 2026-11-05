/*
 * Arduino UNO: escribe un angulo entre 0 y 180 en el Monitor Serie.
 * Monitor Serie: 9600 baudios y terminacion Nueva linea o Ambos NL y CR.
 * Senal del servo: D3. Fuente del servo y Arduino con GND comun.
 */
#include <Servo.h>

const byte PIN_SERVO = 3;
const int ANGULO_INICIAL = 90;
Servo servo;

char entrada[16];
byte longitud = 0;
bool desbordamiento = false;

void procesarEntrada() {
  if (desbordamiento) {
    Serial.println(F("Entrada demasiado larga. Escribe un numero de 0 a 180."));
    return;
  }
  if (longitud == 0) return;

  // Solo se aceptan digitos: texto, signos y decimales no mueven el servo.
  int angulo = 0;
  for (byte i = 0; i < longitud; ++i) {
    if (entrada[i] < '0' || entrada[i] > '9') {
      Serial.println(F("Entrada invalida. Escribe un entero de 0 a 180."));
      return;
    }
    angulo = angulo * 10 + (entrada[i] - '0');
    if (angulo > 180) {
      Serial.println(F("Fuera de rango. Usa un angulo de 0 a 180."));
      return;
    }
  }

  servo.write(angulo);
  Serial.print(F("Angulo enviado al servo: "));
  Serial.print(angulo);
  Serial.println(F(" grados"));
}

void setup() {
  Serial.begin(9600);
  servo.write(ANGULO_INICIAL);
  servo.attach(PIN_SERVO);
  Serial.println(F("Prueba de servo lista. Posicion inicial: 90 grados."));
  Serial.println(F("Escribe de 0 a 180 y pulsa Enviar (Nueva linea)."));
}

void loop() {
  while (Serial.available() > 0) {
    const char caracter = Serial.read();
    if (caracter == '\n' || caracter == '\r') {
      procesarEntrada();
      longitud = 0;
      desbordamiento = false;
    } else if (longitud < sizeof(entrada)) {
      entrada[longitud++] = caracter;
    } else {
      desbordamiento = true;
    }
  }
}
