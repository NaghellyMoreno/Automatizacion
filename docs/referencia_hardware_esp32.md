# Referencia de hardware: ESP32 DevKit 30 pines + placa de expansión 30P

## 1. Entorno verificado

| Elemento | Valor |
|---|---|
| Microcontrolador | ESP32 DevKit (ESP-WROOM-32), 30 pines, USB-C |
| Placa de expansión | ESP32 30P Expansion Board (headers GVS) |
| IDE | Arduino IDE 2.3.10 (Apple Silicon) |
| Core | esp32 by Espressif Systems 3.3.12 |
| Placa en el IDE | ESP32 Dev Module |
| Puerto (Mac) | /dev/cu.usbserial-0001 |
| Monitor Serie | 115200 baudios |
| Cable de programación | Al USB-C del ESP32 (no al de la placa de expansión) |

Prueba de funcionamiento superada: parpadeo del LED integrado en GPIO2 y lectura de la versión del core por el Monitor Serie.

## 2. Distribución física de pines (antena arriba, USB abajo)

| Columna izquierda (arriba → abajo) | Columna derecha (arriba → abajo) |
|---|---|
| EN | D23 |
| VP (GPIO36) | D22 |
| VN (GPIO39) | TX0 (GPIO1) |
| D34 | RX0 (GPIO3) |
| D35 | D21 |
| D32 | D19 |
| D33 | D18 |
| D25 | D5 |
| D26 | TX2 (GPIO17) |
| D27 | RX2 (GPIO16) |
| D14 | D4 |
| D12 | D2 |
| D13 | D15 |
| GND | GND |
| VIN | 3V3 |

## 3. Tabla completa de GPIO

Clasificación: **Recomendado** = uso libre. **Precaución** = funciona, pero afecta el arranque o tiene comportamiento especial. **Solo entrada** = no puede ser salida. **Evitar** = reservado.

| GPIO | Serigrafía | Tipo | ADC | Otras funciones | Clasificación y notas |
|---|---|---|---|---|---|
| 36 | VP | Solo entrada | ADC1_CH0 ✅ WiFi | Sensor VP | Solo entrada. Sin pull-up/pull-down. Ideal para analógico. |
| 39 | VN | Solo entrada | ADC1_CH3 ✅ WiFi | Sensor VN | Solo entrada. Sin pull-up/pull-down. Ideal para analógico. |
| 34 | D34 | Solo entrada | ADC1_CH6 ✅ WiFi | — | Solo entrada. Sin pull-up/pull-down. Ideal para analógico. |
| 35 | D35 | Solo entrada | ADC1_CH7 ✅ WiFi | — | Solo entrada. Sin pull-up/pull-down. Ideal para analógico. |
| 32 | D32 | E/S | ADC1_CH4 ✅ WiFi | TOUCH9 | Recomendado. Analógico compatible con WiFi. |
| 33 | D33 | E/S | ADC1_CH5 ✅ WiFi | TOUCH8 | Recomendado. Analógico compatible con WiFi. |
| 25 | D25 | E/S | ADC2_CH8 ❌ WiFi | DAC1 | Recomendado para digital, PWM o DAC. |
| 26 | D26 | E/S | ADC2_CH9 ❌ WiFi | DAC2 | Recomendado para digital, PWM o DAC. |
| 27 | D27 | E/S | ADC2_CH7 ❌ WiFi | TOUCH7 | Recomendado para digital o PWM. |
| 14 | D14 | E/S | ADC2_CH6 ❌ WiFi | TOUCH6, HSPI CLK | Precaución: emite señal PWM durante el arranque. No usar para relés o cargas que no deban activarse al encender. |
| 12 | D12 | E/S | ADC2_CH5 ❌ WiFi | TOUCH5, HSPI MISO | Precaución: pin de arranque. Si está en ALTO al encender, la placa no arranca. No conectar nada con pull-up. |
| 13 | D13 | E/S | ADC2_CH4 ❌ WiFi | TOUCH4, HSPI MOSI | Recomendado para digital o PWM. |
| 23 | D23 | E/S | — | VSPI MOSI | Recomendado. |
| 22 | D22 | E/S | — | I2C SCL | Recomendado. Reservar para I2C. |
| 1 | TX0 | E/S | — | UART0 TX | Evitar: comunicación USB y Monitor Serie. |
| 3 | RX0 | E/S | — | UART0 RX | Evitar: comunicación USB y Monitor Serie. |
| 21 | D21 | E/S | — | I2C SDA | Recomendado. Reservar para I2C. |
| 19 | D19 | E/S | — | VSPI MISO | Recomendado. |
| 18 | D18 | E/S | — | VSPI CLK | Recomendado. |
| 5 | D5 | E/S | — | VSPI CS0 | Precaución: pin de arranque, emite señal PWM al encender. Bien como CS de SPI. |
| 17 | TX2 | E/S | — | UART2 TX | Recomendado. Reservar si se usa un módulo serial. |
| 16 | RX2 | E/S | — | UART2 RX | Recomendado. Reservar si se usa un módulo serial. |
| 4 | D4 | E/S | ADC2_CH0 ❌ WiFi | TOUCH0 | Recomendado para digital o PWM. |
| 2 | D2 | E/S | ADC2_CH2 ❌ WiFi | TOUCH2, LED azul integrado | Precaución: pin de arranque y LED integrado. Bien como salida. |
| 15 | D15 | E/S | ADC2_CH3 ❌ WiFi | TOUCH3, HSPI CS0 | Precaución: pin de arranque, emite señal PWM al encender. |

**Resumen rápido:**

- **Entradas analógicas (con WiFi):** 32, 33, 34, 35, 36, 39.
- **Digitales y PWM recomendados:** 4, 13, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33.
- **Con precaución:** 2, 5, 12, 14, 15.
- **Evitar:** 1, 3.
- **Buses por defecto:** I2C → SDA 21, SCL 22. SPI (VSPI) → MOSI 23, MISO 19, CLK 18, CS 5. UART2 → TX 17, RX 16.

## 4. Límites eléctricos

- Lógica de **3.3 V**. Los GPIO **no toleran 5 V**. Una señal de 5 V necesita divisor de voltaje o conversor de nivel.
- Corriente por GPIO: mantener por debajo de unos 20 mA. Un GPIO no alimenta relés, motores, bombas ni servos directamente.
- ADC de 12 bits (0–4095). Con la atenuación por defecto mide aproximadamente de 0 a 3.1 V y es poco lineal cerca de los extremos. Usar `analogReadMilliVolts()` para lecturas calibradas y promediar varias muestras.
- Cargas de potencia: fuente externa, GND común con el ESP32 y el elemento de conmutación adecuado (transistor, MOSFET, driver o módulo de relé; diodo flyback en cargas inductivas).

## 5. Placa de expansión ESP32 30P

| # | Elemento | Descripción |
|---|---|---|
| 1 | PWR LED | Indicador de alimentación |
| 2 | USB-C / micro-USB | Entrada de alimentación de 5 V. No sirve para programar. |
| 3 | Jack DC | Entrada de alimentación de 6.5–16 V |
| 4 | Conectores I2C | Dos grupos con SDA (21), SCL (22), VCC y GND |
| 5 | Headers GVS | Por cada pin: G = GND, V = voltaje según jumper, S = señal (GPIO) |
| 6 | Zócalo | Donde se monta el ESP32 |
| 7 | Salida 5V / 3.3V | Pines de alimentación para módulos externos |
| 8 | Jumper V | Selecciona si el pin V de los headers GVS entrega 5 V o 3.3 V |

**Estado actual verificado:**

- El pin V de los headers GVS depende de la posición del jumper amarillo (JUMP).
- **Posición actual del jumper: 3.3 V.**

**Reglas de uso:**

- El jumper V se mantiene en **3.3 V** por defecto. Solo se cambia a 5 V cuando un laboratorio lo indique, y verificando que ninguna señal de 5 V llegue a un GPIO.
- Con el jumper en 5 V, **todos** los pines V de los headers GVS entregan 5 V al mismo tiempo. Un sensor de 5 V conectado a un header GVS puede devolver 5 V por su pin S al GPIO.
- Los módulos de 5 V (por ejemplo el HC-SR04) se alimentan desde el pin de salida de 5 V de la placa, sin cambiar el jumper, y su señal de salida pasa por divisor de voltaje.
- El regulador de la placa no se ha verificado. No alimentar cargas de potencia desde la placa de expansión.

**Pendiente de verificar con multímetro:**

- Si el VCC de los conectores I2C sigue al jumper o es fijo.

## 6. Notas de la API del core 3.x

El core 3.x cambió varias funciones respecto al 2.x. Mucho código de internet usa la API antigua y no compila.

| Función | Core 2.x (antiguo) | Core 3.x (usar este) |
|---|---|---|
| Configurar PWM | `ledcSetup(canal, freq, res)` + `ledcAttachPin(pin, canal)` | `ledcAttach(pin, freq, res)` |
| Escribir PWM | `ledcWrite(canal, duty)` | `ledcWrite(pin, duty)` |
| Crear timer | `timerBegin(num, divisor, true)` | `timerBegin(frecuencia_Hz)` |
| Interrupción de timer | `timerAttachInterrupt(timer, fn, true)` + `timerAlarmWrite(...)` + `timerAlarmEnable(...)` | `timerAttachInterrupt(timer, fn)` + `timerAlarm(timer, valor, autoreload, repeticiones)` |

## 7. Solución de problemas ya resueltos

| Problema | Causa | Solución |
|---|---|---|
| Error `xcrun: unable to load libxcrun ... need 'x86_64'` | Arduino IDE instalado en versión Intel | Instalar la versión Apple Silicon desde arduino.cc |
| No aparece "ESP32 Dev Module" | Solo estaba instalado "Arduino ESP32 Boards" | Instalar "esp32 by Espressif Systems" desde el Gestor de Placas |
| Primera compilación muy lenta | Compila todo el framework la primera vez | Normal; las siguientes usan caché |
| Carga detenida en "Connecting..." | La placa no entró en modo de programación | Mantener presionado BOOT hasta que empiece a escribir |
