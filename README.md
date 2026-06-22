# Smart Sleep Environment Analyzer

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange)](https://platformio.org/)
[![Arduino](https://img.shields.io/badge/Arduino-ESP32-blue)](https://www.arduino.cc/)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)

## 📖 Descripción

**Smart Sleep Environment Analyzer** es un sistema basado en ESP32 que monitoriza y analiza las condiciones ambientales durante el sueño. Mide parámetros clave como CO₂, temperatura, humedad y luz para evaluar la calidad del entorno y generar un **Sleep Score** (0-100) con recomendaciones personalizadas.

El sistema almacena los datos en una tarjeta microSD y los visualiza a través de una **interfaz web** accesible desde cualquier dispositivo conectado a la red WiFi del ESP32.

---

## 🚀 Características principales

- ✅ **Monitorización en tiempo real** de CO₂, temperatura, humedad y luz
- ✅ **Sleep Score** (0-100) para evaluar la calidad del entorno de sueño
- ✅ **Best Hour** (mejor franja horaria) identificada automáticamente
- ✅ **Alertas y recomendaciones** personalizadas
- ✅ **Sleep Timeline** con gráficas interactivas (Chart.js)
- ✅ **Almacenamiento en SD** (CSV + JSON)
- ✅ **Interfaz web** responsive con API REST
- ✅ **Actualización OTA** (Over-The-Air) sin cables
- ✅ **Pantalla OLED** para feedback local

---

## 🛠️ Hardware utilizado

| Componente | Modelo | Función | Bus |
|------------|--------|---------|-----|
| Microcontrolador | ESP32 DevKit V1 | Procesador principal | - |
| Sensor de CO₂ | Sensirion SCD41 | CO₂, temperatura y humedad | I²C |
| Sensor de luz | BH1750 | Iluminación en lux | I²C |
| Pantalla OLED | SH1106 128x64 | Visualización de datos | I²C |
| Tarjeta SD | Módulo microSD | Almacenamiento de datos | SPI |
| LED RGB | 3 LEDs (rojo, amarillo, verde) | Indicadores de estado | GPIO |
| Buzzer | Pasivo | Alarma sonora | GPIO |
| Botón | Pulsador | Control de sesión | GPIO |

---

## 📋 Esquema de conexiones (pinout)

| Componente | Pines | Bus |
|------------|-------|-----|
| **SCD41** (CO₂, Temp, Hum) | SDA: GPIO21, SCL: GPIO22 | I²C |
| **BH1750** (Luz) | SDA: GPIO21, SCL: GPIO22 | I²C |
| **OLED SH1106** | SDA: GPIO21, SCL: GPIO22 | I²C |
| **MicroSD** | CS: GPIO5, SCK: GPIO18, MOSI: GPIO23, MISO: GPIO19 | SPI |
| **LED Rojo** | GPIO13 | GPIO |
| **LED Amarillo** | GPIO14 | GPIO |
| **LED Verde** | GPIO27 | GPIO |
| **Buzzer** | GPIO26 | GPIO |
| **Botón** | GPIO25 | GPIO |

---

## 📁 Estructura del proyecto
Smart_Sleep_Environment_Analyzer_PROYECTO/
├── platformio.ini # Configuración de PlatformIO
├── README.md # Este archivo
├── include/
│ └── config.h # Definiciones globales (pines, umbrales)
├── lib/
│ └── drivers/ # Drivers de bajo nivel
│ ├── BH1750.cpp/h # Sensor de luz
│ └── SCD41.cpp/h # Sensor de CO₂, temp, humedad
├── src/
│ ├── main.cpp # Punto de entrada del programa
│ ├── SessionManager.cpp/h # Gestión de sesiones
│ ├── network/ # Módulos de red
│ │ ├── NTPManager.cpp/h # Sincronización horaria
│ │ └── OTAManager.cpp/h # Actualización OTA
│ └── tasks/ # Tareas FreeRTOS
│ ├── task_Sensor.cpp/h # Lectura de sensores
│ ├── task_Display.cpp/h # Pantalla OLED
│ ├── task_Alert.cpp/h # Alertas, LEDs y buzzer
│ ├── task_Storage.cpp/h # Almacenamiento en SD
│ ├── task_Analysis.cpp/h # Procesamiento de datos
│ ├── task_Button.cpp/h # Botón físico
│ └── task_WebServer.cpp/h # Servidor web y API
├── test/ # Tests (lógicos y de hardware)
│ ├── test_logic/ # Tests de lógica de negocio
│ └── test_*/ # Tests de cada componente
└── docs/ # Documentación
├── memoria.pdf
├── esquema_electrico.pdf
└── diagrama_bloques.png

text

---

## 🔧 Instalación y configuración

### 1. Clonar el repositorio

```bash
git clone https://github.com/tu-usuario/Smart_Sleep_Environment_Analyzer_PROYECTO.git
cd Smart_Sleep_Environment_Analyzer_PROYECTO
```

### 2. Abrir el proyecto en VS Code con PlatformIO
````bash
code .
````

### 3. Instalar dependencias
PlatformIO instalará automáticamente las dependencias definidas en platformio.ini:

```bash 
ini
lib_deps =
    olikraus/U8g2@^2.35.15
    claws/BH1750@^1.2.0
    bblanchon/ArduinoJson@^6.21.3
``` 

### 4. Configurar WiFi (opcional)
Si quieres que el ESP32 se conecte a internet para NTP, modifica src/main.cpp con los datos de tu red WiFi:

````cpp
const char* ssid = "TU_WIFI_SSID";
const char* password = "TU_WIFI_PASSWORD";
````

5. Subir el firmware

```bash
pio run --target upload
```

### 6. Abrir el monitor serie
```bash
pio device monitor
```

## 🌐 Uso Conexión al ESP32
1. El ESP32 crea un Access Point (AP) con SSID: SmartSleep_Analyzer

2. Contraseña: 12345678

3. Conéctate a esta red desde tu móvil, tablet u ordenador.

4. Abre el navegador y ve a: http://192.168.4.1

**Control de sesiones**
Iniciar sesión: Desde la web (botón "Iniciar sesión") o presionando el botón físico.

Finalizar sesión: Desde la web (botón "Finalizar sesión") o presionando el botón físico.

**Visualización de datos**
Pantalla principal: Datos en tiempo real de los sensores.

Ranking: Sesiones ordenadas por Sleep Score.

Historial: Todas las sesiones guardadas.

Detalle de sesión: Gráficas, estadísticas, best hour y recomendaciones.

## 📡 Actualización OTA (Over-The-Air)
Para actualizar el firmware sin cables:

1. Conéctate al AP del ESP32.

2. Ejecuta el comando:

````bash
pio run --target upload --upload-port 192.168.4.1 -e ota
Nota: No es necesario mDNS, ya que se usa la IP fija del AP.
````

## 🧪 Tests
**Tests lógicos**
````bash
pio test -e test -f test_logic
````

**Tests de hardware**
````bash
pio test -e test -f test_BH1750
pio test -e test -f test_SCD41
pio test -e test -f test_LEDs
# ... etc
````

## 📊 Sleep Score
El Sleep Score se calcula sumando las puntuaciones de cada variable:

| Variable | Puntuación máxima | Criterios |
|----------|-------------------|-----------|
| **CO₂** | 40 puntos | < 800 ppm → 40 pts |
| **Temperatura** | 25 puntos | 18-22°C → 25 pts |
| **Humedad** | 20 puntos | 40-60% → 20 pts |
| **Luz** | 15 puntos | < 5 lux → 15 pts |

Interpretación:

85-100: Condiciones óptimas

70-84: Buenas condiciones

50-69: Condiciones aceptables

30-49: Condiciones desfavorables

0-29: Condiciones críticas

## 📝 Documentación
La documentación completa del proyecto está disponible en la carpeta docs/:

memoria.pdf — Memoria técnica del proyecto

esquema_electrico.pdf — Esquema de conexiones

diagrama_bloques.png — Arquitectura del software

👥 Autores
Martina Luna

Gerard Bustillo

