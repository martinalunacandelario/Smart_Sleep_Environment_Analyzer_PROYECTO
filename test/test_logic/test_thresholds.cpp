// test_thresholds.cpp
// Verifica que todos los umbrales definidos en config.h son coherentes.
// Por ejemplo: CO2_GOOD_MAX < CO2_ACCEPTABLE_MAX, rangos óptimos dentro de los aceptables, etc.

#include <Arduino.h>

// ============================================================
// Definición de umbrales (los mismos que en config.h)
// ============================================================
// CO₂
#define CO2_GOOD_MAX         800   // ppm
#define CO2_ACCEPTABLE_MAX  1200   // ppm

// Temperatura
#define TEMP_GOOD_MIN        20.0  // °C
#define TEMP_GOOD_MAX        22.0  // °C
#define TEMP_ACCEPTABLE_MAX  25.0  // °C

// Humedad
#define HUM_GOOD_MIN         40    // %
#define HUM_GOOD_MAX         60    // %
#define HUM_ACCEPTABLE_MIN1  30    // %
#define HUM_ACCEPTABLE_MAX2  70    // %

// Luz
#define LIGHT_GOOD_MAX        5    // lux
#define LIGHT_ACCEPTABLE_MAX 20    // lux

// ============================================================
// Funciones de verificación
// ============================================================
bool checkCO2Thresholds() {
    bool ok = true;
    if (CO2_GOOD_MAX >= CO2_ACCEPTABLE_MAX) {
        Serial.println("❌ ERROR: CO2_GOOD_MAX debe ser menor que CO2_ACCEPTABLE_MAX");
        ok = false;
    }
    if (CO2_GOOD_MAX <= 0) {
        Serial.println("❌ ERROR: CO2_GOOD_MAX debe ser positivo");
        ok = false;
    }
    return ok;
}

bool checkTemperatureThresholds() {
    bool ok = true;
    if (TEMP_GOOD_MIN >= TEMP_GOOD_MAX) {
        Serial.println("❌ ERROR: TEMP_GOOD_MIN debe ser menor que TEMP_GOOD_MAX");
        ok = false;
    }
    if (TEMP_GOOD_MAX > TEMP_ACCEPTABLE_MAX) {
        Serial.println("❌ ERROR: TEMP_GOOD_MAX debe ser <= TEMP_ACCEPTABLE_MAX");
        ok = false;
    }
    if (TEMP_GOOD_MIN <= 0) {
        Serial.println("❌ ERROR: TEMP_GOOD_MIN debe ser positivo");
        ok = false;
    }
    return ok;
}

bool checkHumidityThresholds() {
    bool ok = true;
    // Rango bueno debe estar dentro del rango aceptable
    if (HUM_GOOD_MIN <= HUM_ACCEPTABLE_MIN1) {
        Serial.println("❌ ERROR: HUM_GOOD_MIN debe ser mayor que HUM_ACCEPTABLE_MIN1");
        ok = false;
    }
    if (HUM_GOOD_MAX >= HUM_ACCEPTABLE_MAX2) {
        Serial.println("❌ ERROR: HUM_GOOD_MAX debe ser menor que HUM_ACCEPTABLE_MAX2");
        ok = false;
    }
    if (HUM_GOOD_MIN >= HUM_GOOD_MAX) {
        Serial.println("❌ ERROR: HUM_GOOD_MIN debe ser menor que HUM_GOOD_MAX");
        ok = false;
    }
    if (HUM_ACCEPTABLE_MIN1 >= HUM_ACCEPTABLE_MAX2) {
        Serial.println("❌ ERROR: HUM_ACCEPTABLE_MIN1 debe ser menor que HUM_ACCEPTABLE_MAX2");
        ok = false;
    }
    if (HUM_ACCEPTABLE_MIN1 < 0 || HUM_ACCEPTABLE_MAX2 > 100) {
        Serial.println("❌ ERROR: Los rangos de humedad deben estar entre 0 y 100");
        ok = false;
    }
    return ok;
}

bool checkLightThresholds() {
    bool ok = true;
    if (LIGHT_GOOD_MAX >= LIGHT_ACCEPTABLE_MAX) {
        Serial.println("❌ ERROR: LIGHT_GOOD_MAX debe ser menor que LIGHT_ACCEPTABLE_MAX");
        ok = false;
    }
    if (LIGHT_GOOD_MAX < 0) {
        Serial.println("❌ ERROR: LIGHT_GOOD_MAX no puede ser negativo");
        ok = false;
    }
    return ok;
}

// ============================================================
// setup() - Ejecuta todas las comprobaciones
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== TEST THRESHOLDS CONSISTENCY ===\n");

    bool allOk = true;

    Serial.println("Verificando umbrales de CO₂...");
    if (!checkCO2Thresholds()) allOk = false;

    Serial.println("\nVerificando umbrales de Temperatura...");
    if (!checkTemperatureThresholds()) allOk = false;

    Serial.println("\nVerificando umbrales de Humedad...");
    if (!checkHumidityThresholds()) allOk = false;

    Serial.println("\nVerificando umbrales de Luz...");
    if (!checkLightThresholds()) allOk = false;

    Serial.println("\n========================================");
    if (allOk) {
        Serial.println("✅ TODOS los umbrales son coherentes");
    } else {
        Serial.println("❌ Se encontraron incoherencias en los umbrales");
    }
    Serial.println("========================================\n");
}

void loop() {
    // Vacío: la prueba se ejecuta una sola vez
}