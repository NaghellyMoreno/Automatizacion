const int PIN_MARCHA = 25;
const int PIN_PARO   = 26;
const int PIN_LED    = 23;

bool motorEnMarcha  = false;   // memoria del enclavamiento
bool marchaAnterior = false;   // NUEVA: estado previo del switch de MARCHA

void setup() {
  pinMode(PIN_MARCHA, INPUT_PULLUP);
  pinMode(PIN_PARO,   INPUT_PULLUP);
  pinMode(PIN_LED,    OUTPUT);
  digitalWrite(PIN_LED, LOW);
  Serial.begin(115200);
  Serial.println("Reposo: motor detenido");
}

void loop() {
  bool marcha = (digitalRead(PIN_MARCHA) == LOW);
  bool paro   = (digitalRead(PIN_PARO)   == LOW);

  bool estadoAnterior = motorEnMarcha;

  if (paro) {
    motorEnMarcha = false;                    // PARO tiene prioridad
  } else if (marcha && !marchaAnterior) {     // MARCHA solo al activarse (flanco)
    motorEnMarcha = true;
  }
  marchaAnterior = marcha;

  digitalWrite(PIN_LED, motorEnMarcha ? HIGH : LOW);

  if (motorEnMarcha != estadoAnterior) {
    Serial.println(motorEnMarcha ? "Motor EN MARCHA" : "Motor DETENIDO");
  }

  delay(20);
}