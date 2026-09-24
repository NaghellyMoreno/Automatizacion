# Inventario de componentes

## Sensores

| Componente | Modelo exacto | Cant. | ¿Módulo? | Voltaje de operación | Interfaz | Notas |
|---|---|---|---|---|---|---|
| Sensor ultrasónico de distancia | HC-SR04 | 1 | Sí | 5 V | Digital: Trig (entrada) y Echo (salida de pulso) | Rango 2–400 cm. Útil para nivel de tanque. Echo entrega 5 V: requiere divisor (680 Ω arriba + 2 × 680 Ω en serie abajo ≈ 3.33 V). Trig directo desde el ESP32. VCC al pin de 5 V de la placa de expansión, no a un header GVS. |
| Lector RFID/NFC 13.56 MHz | RFID-RC522 (placa HW-126), chip MFRC522 | 1 | Sí | **3.3 V** (no alimentar a 5 V) | SPI (también I2C/UART, no usados por defecto) | Pines: SDA(SS), SCK, MOSI, MISO, IRQ, GND, RST, 3.3V. Conexión sugerida en VSPI: SCK 18, MOSI 23, MISO 19, SS 5; RST a un GPIO libre; IRQ opcional. Headers ya soldados. Uso: control de acceso, identificación y trazabilidad. |
| Tarjeta RFID blanca | 13.56 MHz, tipo MIFARE (modelo exacto por confirmar al leerla) | 1 | — | Pasiva (sin batería) | Se lee con el RC522 | Tiene UID único. |
| Llavero RFID azul | 13.56 MHz, tipo MIFARE (modelo exacto por confirmar al leerlo) | 1 | — | Pasivo (sin batería) | Se lee con el RC522 | Tiene UID único. |

## Actuadores e indicadores

| Componente | Modelo exacto | Cant. | Voltaje de operación | Interfaz | Notas |
|---|---|---|---|---|---|
| Micro servomotor | Tower Pro SG90 (9 g) | 1 | 4.8–6 V | PWM 50 Hz (pulso ~0.5–2.4 ms) | 0–180°. Cables: café = GND, rojo = +5 V, naranja = señal. Señal de 3.3 V válida. Alimentación externa de 5 V con GND común. Incluye brazos y tornillos. |
| Buzzer con cables | — | 1 | 3–9 V (típico 5 V) | PWM | Pasivo: necesita señal PWM a la frecuencia del tono (`ledcWriteTone`); no suena con un HIGH fijo. |
| Display 7 segmentos, 1 dígito, 0.56" | 5161AS | 1 | ~2 V por segmento (rojo) | 8 pines de segmento + común | Cátodo común, rojo. 330 Ω por segmento. |
| Display 7 segmentos, 1 dígito, 0.56" | 5611BG | 1 | ~2–3 V por segmento (verde) | 8 pines de segmento + común | Ánodo común, verde. Compatible con el 74LS47. |
| LED 5 mm rojo | — | 5 | ~2.0 V | Digital/PWM | 300–330 Ω a 3.3 V. |
| LED 5 mm verde | — | 6 | ~2.0–3.0 V | Digital/PWM | 300–330 Ω a 3.3 V. |
| LED 5 mm amarillo | — | 2 | ~2.0 V | Digital/PWM | 300–330 Ω a 3.3 V. |
| LED 5 mm naranja | — | 1 | ~2.0 V | Digital/PWM | 300–330 Ω a 3.3 V. |
| LED 5 mm blanco | — | 2 | ~3.0–3.2 V | Digital/PWM | A 3.3 V brilla poco; mejor desde 5 V con transistor. |
| LED 5 mm azul | — | 1 | ~3.0–3.2 V | Digital/PWM | A 3.3 V brilla poco; mejor desde 5 V con transistor. |
| LED con cable | — | 4 | 3–9 V (típico 5 V) | Digital | Revisar si traen resistencia integrada (común en versiones de 9–12 V). |

## Circuitos integrados (lógica TTL 74LS, encapsulado DIP)

Todos funcionan a **5 V (4.75–5.25 V)**. Entrada desde el ESP32: directa (3.3 V cuenta como ALTO). Salida hacia el ESP32: **requiere divisor de voltaje**.

| Referencia | Función | Cant. | Pines | Notas |
|---|---|---|---|---|
| SN74LS04N | 6 inversores (NOT) | 3 | 14 | Texas Instruments |
| SN74LS08N | 4 compuertas AND de 2 entradas | 2 | 14 | Texas Instruments |
| SN74LS32N | 4 compuertas OR de 2 entradas | 2 | 14 | Texas Instruments |
| SN74LS85N | Comparador de magnitud de 4 bits | 1 | 16 | Texas Instruments |
| SN74LS157N | 4 multiplexores 2 a 1 | 1 | 16 | Texas Instruments |
| SN74LS47N | Decodificador BCD a 7 segmentos | 1 | 16 | Colector abierto, activo en bajo. Solo para displays de ánodo común. |

## Alimentación y potencia

| Componente | Modelo exacto | Cant. | Voltaje / corriente | Notas |
|---|---|---|---|---|
| Regulador lineal de 5 V | L7805CV (STMicroelectronics), TO-220 | 5 | Entrada 7–35 V, salida 5 V, hasta 1 A con disipador | Pines de frente: IN, GND, OUT. Requiere capacitor de 0.33 µF en la entrada y 0.1 µF en la salida (no hay capacitores en el inventario). Potencia disipada = (Vin − 5 V) × corriente. |
| Batería 9 V | Tronex 6F22 (carbón-zinc) | 1 | 9 V, baja capacidad | Útil con el 7805 para cargas pequeñas. No recomendable para servos o cargas continuas. |
| Conector de broche para batería 9 V | — | 1 | — | Cables rojo (+) y negro (−). |

## Componentes básicos

| Componente | Valor / tipo | Cant. | Usos típicos |
|---|---|---|---|
| Resistencia | 300 Ω, 5 % | 4 | LEDs y segmentos de display a 3.3 V. |
| Resistencia | 300 Ω, 10 % | 4 | Igual que la anterior. Valor poco común con 10 %: medir con multímetro (podrían ser 330 Ω). |
| Resistencia | 330 Ω, 5 % | 4 | LEDs y segmentos de display a 3.3 V. |
| Resistencia | 680 Ω, 5 % | 9 | Divisor 5 V → 3.3 V (1 arriba + 2 en serie abajo). LEDs a 5 V. |
| Resistencia | 820 Ω, 10 % | 1 | LEDs a 5 V, base de transistor. |
| Resistencia | 1 kΩ, 10 % | 1 | Base de transistor, pull-up/pull-down externo. |
| DIP switch | 2 posiciones | 4 | Entradas digitales. Un lado a GND, el otro al GPIO con `INPUT_PULLUP`. No usar GPIO 34–39. |
| Protoboard | 840 puntos | 1 | Los rieles de alimentación están cortados en la mitad: para usar toda la protoboard hay que puentear los carriles. |
| Jumpers Dupont | Macho-macho, macho-hembra, hembra-hembra | ~6 paquetes | Conexión entre headers GVS, protoboard y módulos. |
| Kit de puentes rígidos precortados | Varios largos y colores | 1 kit | Conexiones fijas en la protoboard. |
| Cable sólido trenzado | Tipo UTP | 2 rollos pequeños | Protoboard y cableado a sensores. |

## Herramientas

| Herramienta | Modelo | ¿La tengo? | Notas |
|---|---|---|---|
| Multímetro digital | UNI-T UT20B | Sí | Voltaje DC/AC, corriente DC hasta 200 mA, resistencia, prueba de diodos. Máx. 300 V. |
| Alicate de punta mini con cortador | — | Sí |  |
| Pinzas de punta curva | — | Sí | Para colocar componentes en la protoboard. |
| Destornillador plano pequeño | — | Sí |  |
| Punzón / gancho | — | Sí |  |
