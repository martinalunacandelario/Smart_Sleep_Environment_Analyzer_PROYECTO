#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// PINES I2C (compartidos por SCD41, BH1750 y OLED)
// ============================================================
#define I2C_SDA     21
#define I2C_SCL     22

// ============================================================
// DIRECCIONES I2C
// ============================================================
#define SCD41_ADDR  0x62
#define BH1750_ADDR 0x23

// ============================================================
// CONFIGURACIÓN DE LA TAREA DE SENSORES
// ============================================================
#define SENSOR_INTERVAL_MS  30000   // 30 segundos (o 60000 para 1 minuto)
#define SENSOR_TASK_PRIORITY 4
#define SENSOR_TASK_STACK   4096

// ============================================================
// UMBRALES PARA CLASIFICACIÓN AMBIENTAL (según tabla del proyecto)
// ============================================================

// --- CO₂ (ppm) ---
#define CO2_GOOD_MAX      900     // < 900  → Verde
#define CO2_ACCEPTABLE_MAX 1400   // 900–1400 → Amarillo; >1400 → Rojo

// --- Temperatura (°C) ---
#define TEMP_GOOD_MIN     18.0    // 18–22 → Verde
#define TEMP_GOOD_MAX     22.0
#define TEMP_ACCEPTABLE_MAX 25.0  // 22–25 → Amarillo; <18 o >25 → Rojo

// --- Humedad (%) ---
#define HUM_GOOD_MIN      40.0    // 40–60 → Verde
#define HUM_GOOD_MAX      60.0
#define HUM_ACCEPTABLE_MIN1 30.0  // 30–40 → Amarillo
#define HUM_ACCEPTABLE_MAX1 40.0
#define HUM_ACCEPTABLE_MIN2 60.0  // 60–70 → Amarillo
#define HUM_ACCEPTABLE_MAX2 70.0
// Por debajo de 30 o por encima de 70 → Rojo

// --- Iluminación (lux) ---
#define LIGHT_GOOD_MAX    5.0     // <5   → Verde
#define LIGHT_ACCEPTABLE_MAX 20.0 // 5–20 → Amarillo; >20 → Rojo

// ============================================================
// CONFIGURACIÓN DE LA TAREA DE DISPLAY (OLED SH1106)
// ============================================================
#define DISPLAY_INTERVAL_MS     1000     // Actualizar pantalla cada 1 segundo
#define DISPLAY_TASK_PRIORITY   2        // Prioridad media (2)
#define DISPLAY_TASK_STACK      4096     // Tamaño de pila (bytes)

// Duración de la pantalla encendida después de finalizar sesión (ms)
#define DISPLAY_POST_SESSION_DURATION_MS  60000   // 1 minuto

// Dirección I2C de la OLED (normalmente 0x3C para SH1106)
#define OLED_I2C_ADDR           0x3C

#endif // CONFIG_H