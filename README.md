
# Laboratorios de Automatización y Control de Procesos — ESP32

Repositorio de los códigos (`.ino`) de los laboratorios de la asignatura **Automatización y Control de Procesos (223G8F)**, Ingeniería de Sistemas y Computación, Universidad de Caldas.

Los laboratorios cubren instrumentación, acondicionamiento de señal, lazos de control (ON-OFF, PI, PD, PID), supervisión tipo SCADA y comunicación industrial/IoT, todos implementados sobre un ESP32.

> **Autor:** [Karen Moreno - Camila Giraldo] · **Semestre:** [2026-2] · **Docente:** [Diana Varón]

---

## Contenido

- [Laboratorios de Automatización y Control de Procesos — ESP32](#laboratorios-de-automatización-y-control-de-procesos--esp32)
  - [Contenido](#contenido)
  - [1. Hardware y entorno](#1-hardware-y-entorno)
  - [2. Estructura del repositorio](#2-estructura-del-repositorio)

---

## 1. Hardware y entorno

| Elemento | Valor |
|---|---|
| Microcontrolador | ESP32 DevKit (ESP-WROOM-32), 30 pines, USB-C |
| Placa de expansión | ESP32 30P Expansion Board (headers GVS) |
| Jumper V de la placa | **3.3 V** por defecto (solo se cambia si un laboratorio lo indica) |
| IDE | Arduino IDE 2.3.10 (Apple Silicon), macOS |
| Core | esp32 by Espressif Systems **3.3.12** |
| Placa en el IDE | ESP32 Dev Module |
| Puerto (Mac) | `/dev/cu.usbserial-0001` |
| Monitor Serie | 115200 baudios |
| Cable de programación | Al USB-C del ESP32 (no al de la placa de expansión) |

**Todo el código usa la API 3.x del core.** Mucho código de internet usa la API 2.x y no compila:

| Función | Core 2.x (no usar) | Core 3.x (usar) |
|---|---|---|
| Configurar PWM | `ledcSetup()` + `ledcAttachPin()` | `ledcAttach(pin, freq, res)` |
| Escribir PWM | `ledcWrite(canal, duty)` | `ledcWrite(pin, duty)` |
| Crear timer | `timerBegin(num, divisor, true)` | `timerBegin(frecuencia_Hz)` |
| Alarma de timer | `timerAlarmWrite()` + `timerAlarmEnable()` | `timerAlarm(timer, valor, autoreload, repeticiones)` |

---

## 2. Estructura del repositorio

Arduino IDE exige que **cada `.ino` esté dentro de una carpeta con su mismo nombre**. Por eso cada laboratorio es una carpeta autocontenida:

```
automatizacion-esp32/
├── README.md                     ← este archivo
├── .gitignore                    ← excluye secrets.h y archivos de macOS
├── docs/
│   ├── referencia_hardware_esp32.md
│   ├── inventario_componentes.md
│   └── img/                      ← diagramas del ESP32 y de la placa
└── laboratorios/
    ├── Lab01_UsoSensores/
    │   ├── casoA.ino
    │   └── casoB.ino    
    └── Lab02_NombreCorto/
        └── ...
```

**Convención de nombres:** `LabNN_NombreCorto`, con número de dos dígitos y sin espacios, tildes ni ñ (por ejemplo `Lab03_ControlPID_Servo`). Así las carpetas quedan ordenadas y el IDE no tiene problemas con las rutas.

Cada laboratorio es independiente: no comparte archivos con otros. Si una función se repite (por ejemplo, el promediado del ADC), se copia en cada sketch para que cada carpeta compile sola.

---
