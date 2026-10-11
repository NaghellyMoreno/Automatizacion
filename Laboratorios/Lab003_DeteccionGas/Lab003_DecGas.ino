/*
  Laboratorio Automatización #2 – Detección y respuesta automática ante gas
  Versión MONTAJE FÍSICO: ESP32 DevKit (core esp32 3.3.12) + MQ-2 + SG90 + 3 LEDs

  - Lee la salida AO del MQ-2 (a través de un divisor 10k/15k) en GPIO34 (D34).
  - Convierte la lectura a una "lectura equivalente" de 0 a 1023 (como un Arduino
    UNO a 5 V), para usar los mismos umbrales y la misma tabla de pruebas de la guía.
  - Máquina de estados: NORMAL / PRECAUCION / CRITICO.
  - Servo (válvula) con PWM nativo del core 3.x (ledcAttach / ledcWrite), sin librerías.
  - Modo prueba: escribir un número 0–1023 en el Monitor Serie fuerza esa lectura;
    escribir "r" vuelve a la lectura real del sensor.
*/

// ---------- Pines ----------
const int PIN_GAS          = 34;  // GPIO34 (D34) – ADC1, solo entrada‰
const int PIN_SERVO        = 13;  // GPIO13 (D13) – señal del servo
const int PIN_LED_VERDE    = 25;  // GPIO25 (D25)
const int PIN_LED_AMARILLO = 26;  // GPIO26 (D26)
const int PIN_LED_ROJO     = 27;  // GPIO27 (D27)

// ---------- Divisor de voltaje y alimentación del sensor ----------
const float R_ARRIBA      = 10000.0;  // resistencia entre AO y el GPIO34 (ohmios)
const float R_ABAJO       = 15000.0;  // resistencia entre el GPIO34 y GND (ohmios)
const float VCC_SENSOR_MV = 5000.0;   // voltaje real del pin 5V (medir con multímetro)
const int   MUESTRAS      = 16;       // lecturas promediadas por ciclo

// ---------- Umbrales (tabla de la guía, escala 0–1023) ----------
const int UMBRAL_PRECAUCION = 100;
const int UMBRAL_CRITICO    = 200;
const int HISTERESIS        = 20;   // banda para bajar de estado (0 = tabla exacta)

// ---------- Servo (válvula) ----------
const int ANGULO_NORMAL     = 0;
const int ANGULO_PRECAUCION = 90;
const int ANGULO_CRITICO    = 180;
const int FRECUENCIA_SERVO  = 50;    // Hz (periodo de 20 ms)
const int RESOLUCION_PWM    = 14;    // bits
const int PULSO_MIN_US      = 500;   // pulso para 0°
const int PULSO_MAX_US      = 2400;  // pulso para 180°

// ---------- Temporización ----------
const unsigned long PERIODO_MS = 200;  // un ciclo de lectura/decisión cada 200 ms

// ---------- Variables ----------
enum Estado { NORMAL, PRECAUCION, CRITICO };
Estado estado = NORMAL;
unsigned long ultimoCiclo = 0;
bool modoPrueba = false;
int lecturaPrueba = 0;

// Lee el sensor y devuelve la lectura equivalente 0–1023 (escala de 5 V)
int leerSensor() {
  uint32_t suma = 0;
  for (int i = 0; i < MUESTRAS; i++) {
    suma += analogReadMilliVolts(PIN_GAS);       // milivoltios ya calibrados
  }
  float mvPin    = suma / (float)MUESTRAS;                  // voltaje en el GPIO34
  float mvSensor = mvPin * (R_ARRIBA + R_ABAJO) / R_ABAJO;  // deshace el divisor
  int lectura    = (int)(mvSensor * 1023.0 / VCC_SENSOR_MV);
  return constrain(lectura, 0, 1023);
}

// Decide el siguiente estado a partir del estado actual y la lectura
Estado calcularEstado(int lectura) {
  switch (estado) {
    case NORMAL:
      if (lectura >= UMBRAL_CRITICO)    return CRITICO;
      if (lectura >= UMBRAL_PRECAUCION) return PRECAUCION;
      return NORMAL;
    case PRECAUCION:
      if (lectura >= UMBRAL_CRITICO)                  return CRITICO;
      if (lectura < UMBRAL_PRECAUCION - HISTERESIS)   return NORMAL;
      return PRECAUCION;
    case CRITICO:
      if (lectura < UMBRAL_PRECAUCION - HISTERESIS)   return NORMAL;
      if (lectura < UMBRAL_CRITICO - HISTERESIS)      return PRECAUCION;
      return CRITICO;
  }
  return estado;
}

// Lleva el servo al ángulo indicado (0–180°)
void moverServo(int angulo) {
  uint32_t pulsoUs = map(angulo, 0, 180, PULSO_MIN_US, PULSO_MAX_US);
  uint32_t duty = pulsoUs * ((1 << RESOLUCION_PWM) - 1) / 20000;  // 20000 µs = 20 ms
  ledcWrite(PIN_SERVO, duty);
}

// Acciones de cada estado: un solo LED encendido y el servo en su ángulo
void aplicarSalidas() {
  digitalWrite(PIN_LED_VERDE,    estado == NORMAL);
  digitalWrite(PIN_LED_AMARILLO, estado == PRECAUCION);
  digitalWrite(PIN_LED_ROJO,     estado == CRITICO);

  if (estado == NORMAL)     moverServo(ANGULO_NORMAL);
  if (estado == PRECAUCION) moverServo(ANGULO_PRECAUCION);
  if (estado == CRITICO)    moverServo(ANGULO_CRITICO);
}

// Texto del estado para el Monitor Serie
const char* nombreEstado() {
  if (estado == NORMAL)     return "NORMAL";
  if (estado == PRECAUCION) return "PRECAUCION";
  return "CRITICO";
}

int anguloEstado() {
  if (estado == NORMAL)     return ANGULO_NORMAL;
  if (estado == PRECAUCION) return ANGULO_PRECAUCION;
  return ANGULO_CRITICO;
}

// Modo prueba: número 0–1023 = lectura forzada, "r" = volver al sensor
void revisarMonitorSerie() {
  if (!Serial.available()) return;
  String texto = Serial.readStringUntil('\n');
  texto.trim();
  if (texto.length() == 0) return;

  if (texto == "r" || texto == "R") {
    modoPrueba = false;
    Serial.println(">> Modo SENSOR (lectura real)");
  } else {
    lecturaPrueba = constrain((int)texto.toInt(), 0, 1023);
    modoPrueba = true;
    Serial.print(">> Modo PRUEBA, lectura fija = ");
    Serial.println(lecturaPrueba);
  }
}

void imprimir(int lectura) {
  Serial.print(modoPrueba ? "[PRUEBA] " : "[SENSOR] ");
  Serial.print("Lectura: ");
  Serial.print(lectura);
  Serial.print(" | Estado: ");
  Serial.print(nombreEstado());
  Serial.print(" | Servo: ");
  Serial.print(anguloEstado());
  Serial.println(" grados");
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(20);  // que readStringUntil no bloquee el programa

  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_AMARILLO, OUTPUT);
  pinMode(PIN_LED_ROJO, OUTPUT);

  ledcAttach(PIN_SERVO, FRECUENCIA_SERVO, RESOLUCION_PWM);  // API core 3.x

  aplicarSalidas();  // arranca en NORMAL: LED verde y servo a 0°
  Serial.println("Lab 2 - Deteccion de gas (ESP32 + MQ-2)");
  Serial.println("Escriba 0-1023 para modo prueba, o r para volver al sensor");
}

void loop() {
  revisarMonitorSerie();

  if (millis() - ultimoCiclo >= PERIODO_MS) {
    ultimoCiclo = millis();

    int lectura = modoPrueba ? lecturaPrueba : leerSensor();
    Estado nuevo = calcularEstado(lectura);
    if (nuevo != estado) {     // solo actúa cuando cambia el estado
      estado = nuevo;
      aplicarSalidas();
    }
    imprimir(lectura);
  }
}