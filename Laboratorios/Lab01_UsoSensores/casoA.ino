/* 
   Caso A: El tablero de control de una planta pequena 
   solo debe abrirse a operarios autorizados. 
   Cada intento (valido o no) queda registrado 
   con marca de tiempo y UID, y tres intentos 
   fallidos bloquean el lector durante 10 segundos.
*/

#include <SPI.h>
#include <MFRC522.h>

//  Pines 
const uint8_t PIN_SS         = 5;   // D5  - VSPI CS0
const uint8_t PIN_RST        = 27;  // D27 - reset del RC522
const uint8_t PIN_LED_VERDE  = 25;  // D25
const uint8_t PIN_LED_ROJO   = 26;  // D26
const uint8_t PIN_BUZZER     = 4;   // D4
const uint8_t PIN_SERVO      = 32;  // D32 - cerrojo

//  Parámetros 
const uint32_t T_APERTURA     = 3000;   // el cerrojo queda abierto 3 s
const uint32_t T_DENEGADO     = 1500;
const uint32_t T_BLOQUEO      = 10000;  // bloqueo tras 3 fallos
const uint8_t  MAX_INTENTOS   = 3;
const uint32_t T_POLL_RFID    = 100;    // cada cuánto se busca tarjeta
const uint32_t T_REINTENTO    = 3000;   // reintento del lector en error

// Servo (cerrojo)
const uint32_t SERVO_FREQ_HZ  = 50;
const uint8_t  SERVO_RES_BITS = 16;
const uint16_t SERVO_US_MIN   = 500;
const uint16_t SERVO_US_MAX   = 2400;
const uint8_t  ANGULO_CERRADO = 0;
const uint8_t  ANGULO_ABIERTO = 90;

//  Lista blanca 
const char* UIDS_AUTORIZADOS[] = {
  "AD:B4:AB:00"
  };
const char* NOMBRES[] = { "Operario 1", "Supervisor" };
const uint8_t N_AUTORIZADOS = sizeof(UIDS_AUTORIZADOS) / sizeof(UIDS_AUTORIZADOS[0]);

MFRC522 rfid(PIN_SS, PIN_RST);

//  Máquina de estados 
enum Estado { E_INICIO, E_ESPERA, E_VERIFICANDO, E_CONCEDIDO, E_DENEGADO,
              E_BLOQUEO, E_ERROR_LECTOR };
Estado estado = E_INICIO;

String   uidLeido      = "";
uint8_t  intentos      = 0;
uint32_t tEstado       = 0;   // instante en que se entró al estado actual
uint32_t tUltimoPoll   = 0;
uint32_t tUltimoBeep   = 0;
uint32_t tUltimoBlink  = 0;
bool     nivelBlink    = false;

//  Utilidades
void moverCerrojo(uint8_t grados) {
  uint32_t us   = map(grados, 0, 180, SERVO_US_MIN, SERVO_US_MAX);
  uint32_t duty = (uint32_t)((uint64_t)us * ((1UL << SERVO_RES_BITS) - 1) / 20000UL);
  ledcWrite(PIN_SERVO, duty);
}

void buzzerTono(uint32_t f) { ledcWriteTone(PIN_BUZZER, f); }

// Devuelve el UID en formato "AA:BB:CC:DD"
String uidComoTexto() {
  String s = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) s += "0";
    s += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) s += ":";
  }
  s.toUpperCase();
  return s;
}

int buscarEnListaBlanca(const String& uid) {
  for (uint8_t i = 0; i < N_AUTORIZADOS; i++) {
    if (uid.equalsIgnoreCase(UIDS_AUTORIZADOS[i])) return i;
  }
  return -1;
}

bool lectorResponde() {
  byte v = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  return !(v == 0x00 || v == 0xFF);   // 0x00 / 0xFF = sin comunicación
}

// Registro de trazabilidad: este es el payload que luego iría al broker
void registrar(const String& uid, const char* resultado, const char* quien) {
  Serial.print(F("[LOG] t="));  Serial.print(millis() / 1000);
  Serial.print(F("s | uid="));  Serial.print(uid.length() ? uid : String("-"));
  Serial.print(F(" | resultado=")); Serial.print(resultado);
  Serial.print(F(" | usuario="));   Serial.println(quien);
}

void cambiarEstado(Estado nuevo) {
  estado  = nuevo;
  tEstado = millis();
  switch (estado) {
    case E_ESPERA:
      digitalWrite(PIN_LED_VERDE, LOW);
      digitalWrite(PIN_LED_ROJO,  LOW);
      buzzerTono(0);
      moverCerrojo(ANGULO_CERRADO);
      Serial.println(F(">> ESPERA: acerque la tarjeta"));
      break;
    case E_CONCEDIDO:
      digitalWrite(PIN_LED_VERDE, HIGH);
      digitalWrite(PIN_LED_ROJO,  LOW);
      moverCerrojo(ANGULO_ABIERTO);
      Serial.println(F(">> ACCESO CONCEDIDO"));
      break;
    case E_DENEGADO:
      digitalWrite(PIN_LED_VERDE, LOW);
      digitalWrite(PIN_LED_ROJO,  HIGH);
      buzzerTono(400);                       // tono grave
      Serial.println(F(">> ACCESO DENEGADO"));
      break;
    case E_BLOQUEO:
      digitalWrite(PIN_LED_VERDE, LOW);
      digitalWrite(PIN_LED_ROJO,  HIGH);
      Serial.println(F(">> BLOQUEO POR INTENTOS FALLIDOS (10 s)"));
      break;
    case E_ERROR_LECTOR:
      digitalWrite(PIN_LED_VERDE, LOW);
      buzzerTono(0);
      Serial.println(F(">> ERROR: el RC522 no responde"));
      break;
    default: break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_ROJO,  OUTPUT);
  ledcAttach(PIN_BUZZER, 2000, 10);
  ledcAttach(PIN_SERVO, SERVO_FREQ_HZ, SERVO_RES_BITS);
  moverCerrojo(ANGULO_CERRADO);

  SPI.begin();          // usa VSPI: SCK 18, MISO 19, MOSI 23
  rfid.PCD_Init();
  delay(50);

  Serial.println(F("=== Control de acceso RFID ==="));
  if (lectorResponde()) {
    rfid.PCD_DumpVersionToSerial();
    cambiarEstado(E_ESPERA);
  } else {
    cambiarEstado(E_ERROR_LECTOR);
  }
}

void loop() {
  uint32_t ahora = millis();

  switch (estado) {

    //  Espera de tarjeta 
    case E_ESPERA:
      if (ahora - tUltimoPoll >= T_POLL_RFID) {
        tUltimoPoll = ahora;
        if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
          uidLeido = uidComoTexto();
          rfid.PICC_HaltA();
          rfid.PCD_StopCrypto1();
          cambiarEstado(E_VERIFICANDO);
        }
      }
      break;

    //  Verificación 
    case E_VERIFICANDO: {
      int idx = buscarEnListaBlanca(uidLeido);
      if (idx >= 0) {
        intentos = 0;
        registrar(uidLeido, "CONCEDIDO", NOMBRES[idx]);
        cambiarEstado(E_CONCEDIDO);
      } else {
        intentos++;
        registrar(uidLeido, "DENEGADO", "desconocido");
        Serial.print(F("      intentos fallidos: "));
        Serial.print(intentos); Serial.print(F("/")); Serial.println(MAX_INTENTOS);
        cambiarEstado(intentos >= MAX_INTENTOS ? E_BLOQUEO : E_DENEGADO);
      }
      break;
    }

    //  Acceso concedido 
    case E_CONCEDIDO:
      // Dos tonos agudos cortos al entrar, sin bloquear el loop
      if (ahora - tEstado < 120)                          buzzerTono(1800);
      else if (ahora - tEstado < 200)                     buzzerTono(0);
      else if (ahora - tEstado < 320)                     buzzerTono(2400);
      else                                                buzzerTono(0);

      if (ahora - tEstado >= T_APERTURA) {
        Serial.println(F("      cerrojo cerrado"));
        cambiarEstado(E_ESPERA);
      }
      break;

    //  Acceso denegado 
    case E_DENEGADO:
      if (ahora - tEstado >= 600) buzzerTono(0);
      if (ahora - tEstado >= T_DENEGADO) cambiarEstado(E_ESPERA);
      break;

    //  Bloqueo 
    case E_BLOQUEO:
      if (ahora - tUltimoBeep >= 1000) {                  // beep cada segundo
        tUltimoBeep = ahora;
        buzzerTono(600);
      } else if (ahora - tUltimoBeep >= 150) {
        buzzerTono(0);
      }
      if (ahora - tEstado >= T_BLOQUEO) {
        intentos = 0;
        Serial.println(F("      bloqueo liberado"));
        cambiarEstado(E_ESPERA);
      }
      break;

    // ---------------- Error del lector ----------------
    case E_ERROR_LECTOR:
      if (ahora - tUltimoBlink >= 500) {                  // rojo a 1 Hz
        tUltimoBlink = ahora;
        nivelBlink = !nivelBlink;
        digitalWrite(PIN_LED_ROJO, nivelBlink);
      }
      if (ahora - tEstado >= T_REINTENTO) {               // reintento periódico
        tEstado = ahora;
        rfid.PCD_Init();
        delay(50);
        if (lectorResponde()) {
          Serial.println(F("      lector recuperado"));
          cambiarEstado(E_ESPERA);
        }
      }
      break;

    default:
      cambiarEstado(E_ESPERA);
      break;
  }
}