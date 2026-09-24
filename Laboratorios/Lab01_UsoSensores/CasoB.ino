/* 
   Caso b: Ventilación automática de invernadero 
 */

//  Pines 
const uint8_t PIN_TRIG       = 13;  // D13 - disparo HC-SR04
const uint8_t PIN_ECHO       = 34;  // D34 - solo entrada, tras divisor
const uint8_t PIN_LED_VERDE  = 25;  // D25 - zona libre / normal
const uint8_t PIN_LED_AMAR   = 26;  // D26 - ventilando
const uint8_t PIN_LED_ROJO   = 27;  // D27 - obstrucción
const uint8_t PIN_BUZZER     = 4;   // D4  - buzzer pasivo
const uint8_t PIN_SERVO      = 32;  // D32 - ventana

//  Zonas de distancia (cm) 
const float DIST_LIBRE       = 50.0; // más lejos → zona libre
const float DIST_OBSTRUCCION = 15.0; // más cerca → obstrucción

//  Parámetros del servo 
const uint32_t SERVO_FREQ_HZ  = 50;
const uint8_t  SERVO_RES_BITS = 16;
const uint16_t SERVO_US_MIN   = 500;
const uint16_t SERVO_US_MAX   = 2400;
const uint8_t  ANGULO_CERRADO = 0;
const uint8_t  ANGULO_ABIERTO = 90;

// Movimiento suave del servo
const int16_t  PASO_GRADOS   = 1;    // grados por paso
const uint32_t T_PASO_MS     = 20;   // ms entre pasos → 90° en 1.8 s

//  Tiempos 
const uint32_t T_MUESTREO    = 100;  // ms entre mediciones
const uint32_t T_PARPADEO    = 300;  // ms parpadeo LED amarillo
const uint32_t T_BEEP_OBST   = 400;  // ms periodo buzzer en obstrucción

//  Máquina de estados 
enum Estado { E_NORMAL, E_VENTILANDO, E_OBSTRUCCION, E_CERRANDO };
Estado estado = E_NORMAL;

float    distancia       = 999.0;
int16_t  anguloActual    = ANGULO_CERRADO;
int16_t  anguloObjetivo  = ANGULO_CERRADO;

uint32_t tUltimaMuest    = 0;
uint32_t tUltimoPaso     = 0;
uint32_t tUltimoParpadeo = 0;
uint32_t tUltimoBeep     = 0;
uint32_t tEstado         = 0;
bool     estadoParpadeo  = false;
bool     buzzerOn        = false;
bool     beepVentilando  = false; // bandera: ya sonó el tono de entrada

//  Utilidades
void escribirServo(int16_t grados) {
  if (grados < 0)   grados = 0;
  if (grados > 180) grados = 180;
  uint32_t us   = map(grados, 0, 180, SERVO_US_MIN, SERVO_US_MAX);
  uint32_t duty = (uint32_t)((uint64_t)us *
                  ((1UL << SERVO_RES_BITS) - 1) / 20000UL);
  ledcWrite(PIN_SERVO, duty);
}

void pedirAngulo(int16_t objetivo) {
  anguloObjetivo = objetivo;
}

// Avanza el servo un paso hacia el objetivo (no bloqueante)
void actualizarServo() {
  if (anguloActual == anguloObjetivo) return;
  uint32_t ahora = millis();
  if (ahora - tUltimoPaso < T_PASO_MS) return;
  tUltimoPaso = ahora;

  if (anguloActual < anguloObjetivo) {
    anguloActual += PASO_GRADOS;
    if (anguloActual > anguloObjetivo) anguloActual = anguloObjetivo;
  } else {
    anguloActual -= PASO_GRADOS;
    if (anguloActual < anguloObjetivo) anguloActual = anguloObjetivo;
  }
  escribirServo(anguloActual);
}

float medirDistanciaCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(3);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  uint32_t dur = pulseIn(PIN_ECHO, HIGH, 25000UL);
  if (dur == 0) return 999.0;
  float d = dur * 0.0343 / 2.0;
  return (d < 2.0 || d > 400.0) ? 999.0 : d;
}

void apagarLeds() {
  digitalWrite(PIN_LED_VERDE, LOW);
  digitalWrite(PIN_LED_AMAR,  LOW);
  digitalWrite(PIN_LED_ROJO,  LOW);
}

void cambiarEstado(Estado nuevo) {
  estado  = nuevo;
  tEstado = millis();
  apagarLeds();
  ledcWriteTone(PIN_BUZZER, 0);

  switch (estado) {
    case E_NORMAL:
      digitalWrite(PIN_LED_VERDE, HIGH);
      pedirAngulo(ANGULO_CERRADO);
      beepVentilando = false;
      Serial.println(F(">> NORMAL: ventana cerrando, ambiente estable"));
      break;

    case E_VENTILANDO:
      pedirAngulo(ANGULO_ABIERTO);
      beepVentilando = false;         // se disparará una sola vez en loop
      Serial.println(F(">> VENTILANDO: presencia detectada, abriendo ventana"));
      break;

    case E_OBSTRUCCION:
      digitalWrite(PIN_LED_ROJO, HIGH);
      // El servo se queda donde está (no se le da nuevo objetivo)
      Serial.println(F(">> OBSTRUCCION: demasiado cerca, ventana detenida"));
      break;

    case E_CERRANDO:
      pedirAngulo(ANGULO_CERRADO);
      Serial.println(F(">> CERRANDO: operario se retiró, cerrando ventana"));
      break;
  }
}

//  Salidas no bloqueantes según el estado
void actualizarSalidas() {
  uint32_t ahora = millis();

  switch (estado) {

    case E_NORMAL:
      digitalWrite(PIN_LED_VERDE, HIGH);
      ledcWriteTone(PIN_BUZZER, 0);
      break;

    case E_VENTILANDO:
      // LED amarillo parpadea a ~1.6 Hz
      if (ahora - tUltimoParpadeo >= T_PARPADEO) {
        tUltimoParpadeo = ahora;
        estadoParpadeo  = !estadoParpadeo;
        digitalWrite(PIN_LED_AMAR, estadoParpadeo);
      }
      // Tono suave de confirmación, una sola vez al entrar
      if (!beepVentilando) {
        ledcWriteTone(PIN_BUZZER, 800);
      }
      if (ahora - tEstado >= 400) {
        ledcWriteTone(PIN_BUZZER, 0);
        beepVentilando = true;
      }
      break;

    case E_OBSTRUCCION:
      digitalWrite(PIN_LED_ROJO, HIGH);
      // Buzzer intermitente rápido
      if (ahora - tUltimoBeep >= T_BEEP_OBST) {
        tUltimoBeep = ahora;
        buzzerOn = !buzzerOn;
        ledcWriteTone(PIN_BUZZER, buzzerOn ? 2000 : 0);
      }
      break;

    case E_CERRANDO:
      // LED amarillo parpadea rápido mientras cierra
      if (ahora - tUltimoParpadeo >= 150) {
        tUltimoParpadeo = ahora;
        estadoParpadeo  = !estadoParpadeo;
        digitalWrite(PIN_LED_AMAR, estadoParpadeo);
      }
      ledcWriteTone(PIN_BUZZER, 0);
      break;
  }
}

//  Lógica de transiciones
void evaluarEstado() {
  switch (estado) {

    case E_NORMAL:
      if (distancia < DIST_LIBRE) cambiarEstado(E_VENTILANDO);
      break;

    case E_VENTILANDO:
      if (distancia < DIST_OBSTRUCCION) {
        cambiarEstado(E_OBSTRUCCION);
      } else if (distancia >= DIST_LIBRE) {
        cambiarEstado(E_CERRANDO);
      }
      break;

    case E_OBSTRUCCION:
      if (distancia >= DIST_OBSTRUCCION && distancia < DIST_LIBRE) {
        cambiarEstado(E_VENTILANDO);  // vuelve a ventilar si se aleja un poco
      } else if (distancia >= DIST_LIBRE) {
        cambiarEstado(E_CERRANDO);
      }
      break;

    case E_CERRANDO:
      // Si vuelve a acercarse antes de que cierre del todo
      if (distancia < DIST_LIBRE) {
        cambiarEstado(E_VENTILANDO);
      }
      // Cuando termina de cerrar, vuelve a normal
      if (anguloActual == ANGULO_CERRADO) {
        cambiarEstado(E_NORMAL);
      }
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(PIN_TRIG, OUTPUT);
  digitalWrite(PIN_TRIG, LOW);
  pinMode(PIN_ECHO, INPUT);

  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_AMAR,  OUTPUT);
  pinMode(PIN_LED_ROJO,  OUTPUT);

  ledcAttach(PIN_BUZZER, 800, 10);
  ledcAttach(PIN_SERVO, SERVO_FREQ_HZ, SERVO_RES_BITS);

  escribirServo(ANGULO_CERRADO);
  anguloActual = anguloObjetivo = ANGULO_CERRADO;

  Serial.println(F("=== Ventilacion automatica de invernadero ==="));
  Serial.print(F("Zona libre:       > ")); Serial.print(DIST_LIBRE);       Serial.println(F(" cm"));
  Serial.print(F("Zona ventilando:  ")); Serial.print(DIST_OBSTRUCCION);
  Serial.print(F(" - ")); Serial.print(DIST_LIBRE); Serial.println(F(" cm"));
  Serial.print(F("Zona obstruccion: < ")); Serial.print(DIST_OBSTRUCCION); Serial.println(F(" cm"));

  cambiarEstado(E_NORMAL);
}

void loop() {
  uint32_t ahora = millis();

  // Medición periódica
  if (ahora - tUltimaMuest >= T_MUESTREO) {
    tUltimaMuest = ahora;
    distancia = medirDistanciaCm();

    Serial.print(F("dist="));
    if (distancia >= 999.0) Serial.print(F("--- cm"));
    else { Serial.print(distancia, 1); Serial.print(F(" cm")); }
    Serial.print(F(" | ventana=")); Serial.print(anguloActual);
    Serial.print(F("° | estado="));
    switch (estado) {
      case E_NORMAL:      Serial.println(F("NORMAL"));      break;
      case E_VENTILANDO:  Serial.println(F("VENTILANDO"));  break;
      case E_OBSTRUCCION: Serial.println(F("OBSTRUCCION")); break;
      case E_CERRANDO:    Serial.println(F("CERRANDO"));    break;
    }

    evaluarEstado();
  }

  actualizarSalidas();
  actualizarServo();
}