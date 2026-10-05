/*
  LABORATORIO: Control automatico de ventilacion 
  Logica de control:
    Normal     : T < 30 C          -> servo 0   -> LED verde
    Precaucion : 30 C <= T < 34 C  -> servo 90  -> LED amarillo
    Critico    : T >= 34 C         -> servo 180 -> LED rojo
*/

#include <DHT.h>         
#include <ESP32Servo.h>   )

// Pines
#define PIN_DHT       4    // D4
#define PIN_SERVO     13   // D13
#define LED_VERDE     25   // D25
#define LED_AMARILLO  26   // D26
#define LED_ROJO      27   // D27

DHT dht(PIN_DHT, DHT11);
Servo servo;

unsigned long tiempoAnterior = 0;
const unsigned long INTERVALO = 2000;   // leer el sensor cada 2 segundos

void setup() {
  Serial.begin(115200);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);

  dht.begin();

  servo.setPeriodHertz(50);             // servos trabajan a 50 Hz
  servo.attach(PIN_SERVO, 500, 2400);   // pulso del SG90 en microsegundos
  servo.write(0);

  Serial.println("Sistema de ventilacion iniciado");
}

void loop() {
  // Solo entra cuando han pasado 2 segundos 
  if (millis() - tiempoAnterior >= INTERVALO) {
    tiempoAnterior = millis();

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    // Si la lectura falla, se avisa y se espera la siguiente
    if (isnan(t) || isnan(h)) {
      Serial.println("Error leyendo el DHT11. Revisar conexiones.");
      return;
    }

    // Apagar los tres LEDs antes de encender el que corresponde
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_AMARILLO, LOW);
    digitalWrite(LED_ROJO, LOW);

    // NORMAL
    if (t < 30) {
      servo.write(0);
      digitalWrite(LED_VERDE, HIGH);
      Serial.print("NORMAL     | ");
    }

    // PRECAUCION
    if (t >= 30 && t < 34) {
      servo.write(90);
      digitalWrite(LED_AMARILLO, HIGH);
      Serial.print("PRECAUCION | ");
    }

    // CRITICO
    if (t >= 34) {
      servo.write(180);
      digitalWrite(LED_ROJO, HIGH);
      Serial.print("CRITICO    | ");
    }

    // Mostrar lecturas
    Serial.print("T: ");
    Serial.print(t, 1);
    Serial.print(" C | H: ");
    Serial.print(h, 1);
    Serial.println(" %");
  }
}
