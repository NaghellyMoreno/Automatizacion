/*
 * ==========================================================================
 *  LABORATORIO DE AUTOMATIZACIÓN #5 - Grupo 1
 *  Riego automático de una huerta con protección del tanque
 *  Integrantes: Karen - Camila
 *  Placa: ESP32 (DevKit v1 / WROOM-32)
 * ==========================================================================
 *
 *  ENTRADAS
 *   - Sensor DHT11 (humedad relativa, equivalente del sensor de suelo) GPIO4
 *       NOTA: el DHT11 mide humedad del AIRE, no del suelo. Se usa como
 *       sensor equivalente: soplar/acercar algo húmedo sube la humedad
 *       ("suelo húmedo"); el ambiente normal representa "suelo seco".
 *   - Nivel del tanque:
 *       a) HC-SR04 ultrasónico: TRIG GPIO19, ECHO GPIO18 (con divisor 5V->3.3V)
 *       b) o potenciómetro (equivalente de simulación) en GPIO35 (ADC1)
 *
 *  SALIDAS (dos tipos distintos)
 *   - Servomotor (válvula de la bomba) ............ GPIO13
 *   - LED "riego en curso" (bomba) + 220 Ω ........ GPIO25
 *   - LED "alarma de tanque bajo" + 220 Ω ......... GPIO26
 *
 *  LIBRERÍAS REQUERIDAS (Gestor de Librerías de Arduino IDE):
 *   - "ESP32Servo"
 *   - "DHT sensor library" de Adafruit (+ "Adafruit Unified Sensor")
 *  Compatible con el núcleo ESP32 de Arduino 2.x y 3.x.
 * ==========================================================================
 */

#include <Arduino.h>
#include <ESP32Servo.h>
#include <DHT.h>
#include "driver/gpio.h"

// --------------------------------------------------------------------------
// Selección del sensor de nivel
//   1 = sensor ultrasónico HC-SR04 (montaje físico)
//   0 = potenciómetro (equivalente para pruebas)
// --------------------------------------------------------------------------
#define NIVEL_CON_ULTRASONICO 1

// --------------------------------------------------------------------------
// Pines
// --------------------------------------------------------------------------
const int PIN_DHT        = 4;    // DATA del DHT11
const int PIN_NIVEL_POT  = 35;   // ADC1_CH7, solo entrada (modo potenciómetro)
const int PIN_TRIG       = 19;   // HC-SR04 TRIG
const int PIN_ECHO       = 18;   // HC-SR04 ECHO (¡con divisor de tensión!)
const int PIN_SERVO      = 13;   // Señal del servomotor (válvula)
const int PIN_LED_BOMBA  = 25;   // LED verde: riego en curso
const int PIN_LED_ALARMA = 26;   // LED rojo: tanque bajo

#define TIPO_DHT DHT11

// --------------------------------------------------------------------------
// PARÁMETROS DE CALIBRACIÓN (ajustar con las lecturas del Monitor Serie)
// --------------------------------------------------------------------------
// Humedad relativa del DHT11 (%). Medir primero la humedad ambiente
// (= "suelo seco") y la humedad al soplar/acercar un paño húmedo
// (= "suelo mojado"). Sugerencia:
//   UMBRAL_SECO   ≈ humedad ambiente + 3..5 %
//   UMBRAL_HUMEDO ≈ humedad ambiente + 12..15 %
const float UMBRAL_SECO_PCT   = 20.0;   // <= enciende el riego
const float UMBRAL_HUMEDO_PCT = 30.0;   // >= apaga el riego

// Tanque (sensor ultrasónico arriba del tanque): distancia hasta el agua.
const float DIST_TANQUE_LLENO_CM = 5.0;   // distancia con tanque lleno
const float DIST_TANQUE_VACIO_CM = 30.0;  // distancia con tanque vacío

// Histéresis del nivel (en %)
const float NIVEL_MIN_PCT     = 20.0;   // <= alarma
const float NIVEL_REARME_PCT  = 30.0;   // >= se rearma
const float NIVEL_LLENO_PCT   = 90.0;   // >= tanque lleno (solo indicación)

// Posiciones del servo (válvula)
const int SERVO_CERRADO = 0;    // grados: bomba apagada
const int SERVO_ABIERTO = 90;   // grados: bomba encendida

// Tiempos (múltiplos del tick del timer de 10 ms)
const uint32_t TICK_US          = 10000; // periodo del timer: 10 ms
const uint32_t TICKS_TRIGGER    = 6;     // disparo ultrasónico cada 60 ms
const uint32_t TICKS_CONTROL    = 10;    // control cada 100 ms
const uint32_t TICKS_DHT        = 200;   // DHT11 cada 2 s (máx. 1 lectura/s)
const uint32_t TICKS_REPORTE    = 50;    // Monitor Serie cada 500 ms
const uint32_t TICKS_PARPADEO   = 25;    // LED de alarma conmuta cada 250 ms
const uint32_t TIMEOUT_ECO_MS   = 500;   // sin eco válido -> falla de nivel
const uint32_t TIMEOUT_DHT_MS   = 6000;  // sin lectura DHT -> falla de humedad
const uint32_t ESPERA_MAX_DHT_MS = 200;  // si no llega eco, leer DHT igual

// --------------------------------------------------------------------------
// Objetos y variables globales
// ------------------------------------------------------------ --------------
Servo valvula;
DHT   dht(PIN_DHT, TIPO_DHT);

// Timer por hardware
hw_timer_t  *timerControl = nullptr;
portMUX_TYPE muxTimer = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t ticks         = 0;
volatile bool     flagTrigger   = false;
volatile bool     flagControl   = false;
volatile bool     flagDht       = false;
volatile bool     flagReporte   = false;
volatile bool     flagParpadeo  = false;

// Medición del eco por interrupción externa
portMUX_TYPE muxEcho = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t tSubidaEco = 0;
volatile uint32_t anchoEcoUs = 0;
volatile bool     ecoNuevo   = false;

// Estado del proceso
enum Estado { NORMAL, RIEGO, ALARMA_TANQUE, FALLA_HUMEDAD, LLENO };
Estado estado = NORMAL;

float humedadPct     = 0.0;
float temperaturaC   = 0.0;
bool  humedadValida  = false;
uint32_t tUltimoDht  = 0;
bool  dhtPendiente   = false;
uint32_t tDhtPedido  = 0;

int   nivelRaw      = 0;      // solo en modo potenciómetro
float distanciaCm   = -1.0;   // -1 = aún sin lectura
float nivelPct      = 0.0;
bool  nivelValido   = false;
uint32_t tUltimoEco = 0;

bool demandaRiego = false;    // lo que pide la humedad (con histéresis)
bool tanqueBajo   = true;     // arranca protegido hasta tener lectura
bool bombaOn      = false;
bool ledAlarmaOn  = false;

// ==========================================================================
// RUTINAS DE INTERRUPCIÓN (cortas, en IRAM)
// ==========================================================================

// Timer cada 10 ms: solo levanta banderas; el trabajo se hace en loop().
void IRAM_ATTR isrTimer() {
  portENTER_CRITICAL_ISR(&muxTimer);
  ticks++;
  if (ticks % TICKS_TRIGGER  == 0) flagTrigger  = true;
  if (ticks % TICKS_CONTROL  == 0) flagControl  = true;
  if (ticks % TICKS_DHT      == 0) flagDht      = true;
  if (ticks % TICKS_REPORTE  == 0) flagReporte  = true;
  if (ticks % TICKS_PARPADEO == 0) flagParpadeo = true;
  portEXIT_CRITICAL_ISR(&muxTimer);
}

// Flanco de subida: guarda el instante. Flanco de bajada: calcula el ancho.
void IRAM_ATTR isrEcho() {
  uint32_t ahora = micros();
  portENTER_CRITICAL_ISR(&muxEcho);
  if (gpio_get_level((gpio_num_t)PIN_ECHO)) {
    tSubidaEco = ahora;
  } else {
    anchoEcoUs = ahora - tSubidaEco;
    ecoNuevo   = true;
  }
  portEXIT_CRITICAL_ISR(&muxEcho);
}

// ==========================================================================
// FUNCIONES AUXILIARES
// ==========================================================================

// Lee una bandera del timer y la limpia de forma atómica.
bool tomarBandera(volatile bool &bandera) {
  bool valor;
  portENTER_CRITICAL(&muxTimer);
  valor   = bandera;
  bandera = false;
  portEXIT_CRITICAL(&muxTimer);
  return valor;
}

// Promedio de varias lecturas del ADC para reducir ruido.
int leerAdcPromedio(int pin, int muestras = 16) {
  uint32_t suma = 0;
  for (int i = 0; i < muestras; i++) suma += analogRead(pin);
  return (int)(suma / muestras);
}

// Convierte una lectura entre dos puntos de calibración a 0-100 %.
float aPorcentaje(float valor, float valor0pct, float valor100pct) {
  float pct = (valor - valor0pct) * 100.0 / (valor100pct - valor0pct);
  return constrain(pct, 0.0, 100.0);
}

// Pulso de 10 µs en TRIG.
void dispararUltrasonico() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
}

// Procesa el último eco recibido. Devuelve true si llegó un eco nuevo.
bool procesarEco() {
  bool     nuevo;
  uint32_t ancho;
  portENTER_CRITICAL(&muxEcho);
  nuevo    = ecoNuevo;
  ancho    = anchoEcoUs;
  ecoNuevo = false;
  portEXIT_CRITICAL(&muxEcho);

  if (!nuevo) return false;

  float d = ancho * 0.0343 / 2.0;            // cm
  if (d >= 2.0 && d <= 400.0) {
    // Filtro exponencial para suavizar la lectura
    distanciaCm = (distanciaCm < 0) ? d : (0.7 * distanciaCm + 0.3 * d);
    tUltimoEco  = millis();
  }
  return true;
}

// Lectura del DHT11 (~25 ms). Si falla, conserva el último valor válido.
void leerDht() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h)) {
    humedadPct = h;
    tUltimoDht = millis();
  }
  if (!isnan(t)) temperaturaC = t;
}

// Mueve la válvula y el LED solo cuando cambia el estado de la bomba.
void aplicarBomba(bool encender) {
  if (encender == bombaOn) return;
  bombaOn = encender;
  valvula.write(bombaOn ? SERVO_ABIERTO : SERVO_CERRADO);
  digitalWrite(PIN_LED_BOMBA, bombaOn ? HIGH : LOW);
}

// ==========================================================================
// LÓGICA DE CONTROL (cada 100 ms)
// ==========================================================================
void actualizarControl() {
  // ---- Humedad (DHT11) ----
  humedadValida = (tUltimoDht != 0) && (millis() - tUltimoDht < TIMEOUT_DHT_MS);

  // ---- Nivel del tanque ----
#if NIVEL_CON_ULTRASONICO
  nivelValido = (distanciaCm > 0) && (millis() - tUltimoEco < TIMEOUT_ECO_MS);
  if (nivelValido) {
    nivelPct = aPorcentaje(distanciaCm, DIST_TANQUE_VACIO_CM, DIST_TANQUE_LLENO_CM);
  }
#else
  nivelRaw    = leerAdcPromedio(PIN_NIVEL_POT);
  nivelPct    = aPorcentaje(nivelRaw, 0, 4095);
  nivelValido = true;
#endif

  // ---- Protección del tanque con histéresis ----
  if (!nivelValido || nivelPct <= NIVEL_MIN_PCT) {
    tanqueBajo = true;
  } else if (nivelPct >= NIVEL_REARME_PCT) {
    tanqueBajo = false;
  }

  // ---- Demanda de riego con dos umbrales (histéresis) ----
  if (!humedadValida) {
    demandaRiego = false;           // sin dato de humedad no se riega
  } else if (humedadPct <= UMBRAL_SECO_PCT) {
    demandaRiego = true;            // "suelo seco" -> pedir riego
  } else if (humedadPct >= UMBRAL_HUMEDO_PCT) {
    demandaRiego = false;           // "suelo húmedo" -> dejar de regar
  }  // entre umbrales conserva el estado anterior

  // ---- Salida: la bomba solo funciona si hay demanda Y hay agua ----
  aplicarBomba(demandaRiego && !tanqueBajo);

  // ---- Estado e indicadores ----
  if (tanqueBajo)          estado = ALARMA_TANQUE;
  else if (!humedadValida) estado = FALLA_HUMEDAD;
  else if (bombaOn)        estado = RIEGO;
  else                     estado = NORMAL;

  if (!tanqueBajo && ledAlarmaOn) {
    ledAlarmaOn = false;
    digitalWrite(PIN_LED_ALARMA, LOW);
  }
}

// LED de alarma intermitente (250 ms) mientras el tanque esté bajo.
void actualizarParpadeo() {
  if (estado == ALARMA_TANQUE) {
    ledAlarmaOn = !ledAlarmaOn;
    digitalWrite(PIN_LED_ALARMA, ledAlarmaOn ? HIGH : LOW);
  }
}

// Reporte para calibrar umbrales y registrar las pruebas.
void reportarSerie() {
  const char *txtEstado = (estado == RIEGO)         ? "RIEGO EN CURSO"
                        : (estado == ALARMA_TANQUE) ? "ALARMA TANQUE BAJO"
                        : (estado == FALLA_HUMEDAD) ? "FALLA SENSOR DHT11"
                        : (estado == LLENO) ? "TANQUE LLENO"
                                                    : "NORMAL";
  Serial.print("Hum%:");
  if (humedadValida) Serial.print(humedadPct, 0); else Serial.print("--");
  Serial.print("  Temp(C):");  Serial.print(temperaturaC, 0);
#if NIVEL_CON_ULTRASONICO
  Serial.print("  Dist(cm):");
  if (nivelValido) Serial.print(distanciaCm, 1); else Serial.print("SIN ECO");
#else
  Serial.print("  NivRaw:");   Serial.print(nivelRaw);
#endif
  
  Serial.print("  Nivel%:");   Serial.print(nivelPct, 1);

  // Estado del tanque
  Serial.print("  Tanque:");
  if (!nivelValido)                   Serial.print("SIN LECTURA");
  else if (tanqueBajo)                Serial.print("BAJO");
  else if (nivelPct >= NIVEL_LLENO_PCT) Serial.print("LLENO");
  else                                Serial.print("CON AGUA");
  Serial.print("  Demanda:");  Serial.print(demandaRiego ? "SI" : "NO");
  Serial.print("  Bomba:");    Serial.print(bombaOn ? "ON" : "OFF");
  Serial.print("  Servo:");    Serial.print(bombaOn ? SERVO_ABIERTO : SERVO_CERRADO);
  Serial.print("  Estado:");   Serial.println(txtEstado);
}

// ==========================================================================
// CONFIGURACIÓN
// ==========================================================================
void configurarTimer() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  // Núcleo ESP32 3.x
  timerControl = timerBegin(1000000);                 // 1 MHz -> 1 tick = 1 µs
  timerAttachInterrupt(timerControl, &isrTimer);
  timerAlarm(timerControl, TICK_US, true, 0);         // cada 10 ms, recarga
#else
  // Núcleo ESP32 2.x
  timerControl = timerBegin(0, 80, true);             // 80 MHz / 80 = 1 MHz
  timerAttachInterrupt(timerControl, &isrTimer, true);
  timerAlarmWrite(timerControl, TICK_US, true);
  timerAlarmEnable(timerControl);
#endif
}

void setup() {
  Serial.begin(115200);
  delay(200);   // solo al arrancar
  Serial.println();
  Serial.println("=== Lab 4: Riego automatico con proteccion del tanque (ESP32 + DHT11) ===");

  // Salidas digitales
  pinMode(PIN_LED_BOMBA, OUTPUT);
  pinMode(PIN_LED_ALARMA, OUTPUT);
  digitalWrite(PIN_LED_BOMBA, LOW);
  digitalWrite(PIN_LED_ALARMA, LOW);

  // Sensor DHT11
  dht.begin();

#if !NIVEL_CON_ULTRASONICO
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_NIVEL_POT, ADC_11db);
#endif

  // Servo por PWM de hardware (LEDC), 50 Hz, pulso 500-2400 µs
  ESP32PWM::allocateTimer(0);
  valvula.setPeriodHertz(50);
  valvula.attach(PIN_SERVO, 500, 2400);
  valvula.write(SERVO_CERRADO);   // arranca con la válvula cerrada
  bombaOn = false;

  // Ultrasónico: TRIG salida, ECHO con interrupción en ambos flancos
#if NIVEL_CON_ULTRASONICO
  pinMode(PIN_TRIG, OUTPUT);
  digitalWrite(PIN_TRIG, LOW);
  pinMode(PIN_ECHO, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_ECHO), isrEcho, CHANGE);
#endif

  // Timer por hardware de 10 ms (base de tiempo de todo el sistema)
  configurarTimer();

  // El DHT11 necesita ~1 s tras encender: primera lectura pedida en 2 s
  Serial.println("Sistema listo. Valvula cerrada. Esperando lectura del DHT11...");
}

// ==========================================================================
// LAZO PRINCIPAL (no bloqueante: atiende banderas de las interrupciones)
// ==========================================================================
void loop() {
  // Pedido de lectura del DHT11 (cada 2 s)
  if (tomarBandera(flagDht)) {
    dhtPendiente = true;
    tDhtPedido   = millis();
  }

#if NIVEL_CON_ULTRASONICO
  if (tomarBandera(flagTrigger)) dispararUltrasonico();
  bool ecoRecibido = procesarEco();

  // El DHT11 se lee justo después de un eco: la próxima medición del
  // ultrasónico empieza en ≥ 35 ms, así el bloqueo del DHT no la afecta.
  // Si el ultrasónico no responde, se lee igual tras 200 ms de espera.
  if (dhtPendiente && (ecoRecibido || millis() - tDhtPedido > ESPERA_MAX_DHT_MS)) {
    leerDht();
    dhtPendiente = false;
  }
#else
  if (dhtPendiente) {
    leerDht();
    dhtPendiente = false;
  }
#endif

  if (tomarBandera(flagControl))  actualizarControl();
  if (tomarBandera(flagParpadeo)) actualizarParpadeo();
  if (tomarBandera(flagReporte))  reportarSerie();
}
