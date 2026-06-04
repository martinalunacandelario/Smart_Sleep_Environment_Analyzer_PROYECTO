#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// PINES I2C (compartidos por SCD41, BH1750 y OLED)
// ============================================================
#define I2C_SDA     21        // Pin SDA para bus I2C (datos)
#define I2C_SCL     22        // Pin SCL para bus I2C (reloj)

// ============================================================
// DIRECCIONES I2C
// ============================================================
#define SCD41_ADDR  0x62      // Dirección I2C del sensor SCD41 (CO2, Temp, Hum)
#define BH1750_ADDR 0x23      // Dirección I2C del sensor BH1750 (luz)

// ============================================================
// CONFIGURACIÓN DE LA TAREA DE SENSORES
// ============================================================
#define SENSOR_INTERVAL_MS  30000   // 30 segundos (lectura de sensores cada 30s)
#define SENSOR_TASK_PRIORITY 4      // Prioridad alta para SensorTask
#define SENSOR_TASK_STACK   4096    // Tamaño de pila en bytes (4KB)

// ============================================================
// UMBRALES PARA CLASIFICACIÓN AMBIENTAL (según tabla del proyecto)
// ============================================================

// --- CO₂ (ppm) ---
#define CO2_GOOD_MAX      900     // < 900  → Verde (Bueno)
#define CO2_ACCEPTABLE_MAX 1400   // 900–1400 → Amarillo (Regular); >1400 → Rojo (Malo)

// --- Temperatura (°C) ---
#define TEMP_GOOD_MIN     18.0    // 18–24°C → Verde (Bueno)
#define TEMP_GOOD_MAX     24.0    // Temperatura máxima óptima
#define TEMP_ACCEPTABLE_MAX 28.5  // 24–28.5°C → Amarillo (Regular); <18 o >28.5 → Rojo

// --- Humedad (%) ---
#define HUM_GOOD_MIN      40.0    // 40–60% → Verde (Bueno)
#define HUM_GOOD_MAX      60.0    // Humedad máxima óptima
#define HUM_ACCEPTABLE_MIN1 30.0  // 30–40% → Amarillo (Regular)
#define HUM_ACCEPTABLE_MAX1 40.0  // Límite superior del primer rango regular
#define HUM_ACCEPTABLE_MIN2 60.0  // 60–70% → Amarillo (Regular)
#define HUM_ACCEPTABLE_MAX2 70.0  // Límite superior del segundo rango regular
// Por debajo de 30% o por encima de 70% → Rojo (Malo)

// --- Iluminación (lux) ---
#define LIGHT_GOOD_MAX    5.0     // <5 lux → Verde (Bueno)
#define LIGHT_ACCEPTABLE_MAX 20.0 // 5–20 lux → Amarillo (Regular); >20 lux → Rojo (Malo)

// ============================================================
// CONFIGURACIÓN DE LA TAREA DE DISPLAY (OLED SH1106)
// ============================================================
#define DISPLAY_INTERVAL_MS     1000     // Actualizar pantalla cada 1 segundo
#define DISPLAY_TASK_PRIORITY   2        // Prioridad media (2)
#define DISPLAY_TASK_STACK      4096     // Tamaño de pila en bytes (4KB)

// Duración de la pantalla encendida después de finalizar sesión (ms)
#define DISPLAY_POST_SESSION_DURATION_MS  60000   // 1 minuto (60 segundos)

// Dirección I2C de la OLED (normalmente 0x3C para SH1106)
#define OLED_I2C_ADDR           0x3C     // Dirección I2C de la pantalla OLED

// ============================================================
// CONFIGURACIÓN DE LA TAREA DE ALERTAS (LED RGB + BUZZER)
// ============================================================

// Pines LED RGB (cátodo común o LEDs individuales)
#define LED_RED_PIN     13       // GPIO13 → LED Rojo (condición crítica)
#define LED_GREEN_PIN   27       // GPIO27 → LED Verde (condición óptima)
#define LED_YELLOW_PIN  14       // GPIO14 → LED Amarillo (condición regular)

// Pin del buzzer pasivo (genera tonos con tone())
#define BUZZER_PIN      26       // GPIO26 → Buzzer pasivo

// Prioridad de la tarea de alertas (alta, para respuesta rápida)
#define ALERT_TASK_PRIORITY  4   // Prioridad alta (4)

// Tamaño de pila para AlertTask en bytes
#define ALERT_TASK_STACK     4096 // 4KB de pila

// Duración que se muestra una recomendación en la OLED (ms)
#define RECOMMENDATION_DURATION_MS  5000   // 5 segundos

// ============================================================
// PINES SPI PARA MÓDULO MICROSD (StorageTask)
// ============================================================
#define SD_CS       5      // Chip Select (GPIO5) - selecciona la tarjeta SD
#define SD_SCK      18     // Clock (GPIO18) - reloj SPI
#define SD_MOSI     23     // Master Out Slave In (GPIO23) - datos del ESP32 a la SD
#define SD_MISO     19     // Master In Slave Out (GPIO19) - datos de la SD al ESP32

// ============================================================
// CONFIGURACIÓN DE LA TAREA DE ALMACENAMIENTO (StorageTask)
// ============================================================
#define STORAGE_TASK_PRIORITY   1        // Prioridad baja (1) - no interfiere con tareas críticas
#define STORAGE_TASK_STACK      4096     // Tamaño de pila en bytes (4KB)

// Directorio y formato de archivos en la microSD
#define SD_BASE_PATH            "/sessions"          // Carpeta donde se guardan las sesiones
#define SD_FILENAME_PREFIX      "session_"           // Prefijo de los archivos CSV (cambio a session_)
#define SD_FILENAME_EXT         ".csv"               // Extensión de los archivos de datos

// ============================================================
// CONFIGURACIÓN DE LA TAREA DE ANÁLISIS (AnalysisTask)
// ============================================================
#define ANALYSIS_TASK_PRIORITY  2        // Prioridad media (2)
#define ANALYSIS_TASK_STACK     8192     // Tamaño de pila en bytes (8KB) - necesita más memoria

// ============================================================
// CONFIGURACIÓN DE NTP (Network Time Protocol) - PARA FUTURO
// ============================================================
// #define NTP_SERVER              "pool.ntp.org"       // Servidor NTP gratuito
// #define GMT_OFFSET_SEC          3600                 // UTC+1 (España peninsular)
// #define DAYLIGHT_OFFSET_SEC     3600                 // Horario de verano (1 hora)

// ============================================================
// CONFIGURACIÓN DE LA TAREA WEB (WebServerTask) - PARA FUTURO
// ============================================================
// #define WEB_TASK_PRIORITY       2        // Prioridad media (2)
// #define WEB_TASK_STACK          8192     // Tamaño de pila (8KB)
// #define WEB_SERVER_PORT         80       // Puerto del servidor web

// ============================================================
// CREDENCIALES WiFi - PARA FUTURO (¡NO SUBIR A GIT!)
// ============================================================
// #define WIFI_SSID               "TU_WIFI_SSID"       // Cambia por tu SSID
// #define WIFI_PASSWORD           "TU_WIFI_PASSWORD"   // Cambia por tu contraseña

#endif // CONFIG_H