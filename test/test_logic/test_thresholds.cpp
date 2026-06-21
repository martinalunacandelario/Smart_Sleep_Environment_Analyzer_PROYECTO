// test_logic/test_thresholds.cpp
// ============================================================================
// LOGIC TEST: Verifica coherencia de umbrales definidos en config.h
// ============================================================================
// DESCRIPCIÓN: Comprueba que los umbrales en config.h son lógicos y consistentes
//              Usa las definiciones REALES del proyecto, no duplicados.
// ============================================================================

#include <Arduino.h>
#include "../../include/config.h"    // ← UMBRALES REALES DEL PROYECTO

// ============================================================================
// Funciones de verificación (usando los #define de config.h)
// ============================================================================
bool checkCO2Thresholds() {
    bool ok = true;
    Serial.println("  Verificando CO₂:");
    Serial.printf("    CO2_GOOD_MAX = %d, CO2_ACCEPTABLE_MAX = %d\n", 
                  CO2_GOOD_MAX, CO2_ACCEPTABLE_MAX);
    
    if (CO2_GOOD_MAX >= CO2_ACCEPTABLE_MAX) {
        Serial.println("    ❌ ERROR: CO2_GOOD_MAX debe ser menor que CO2_ACCEPTABLE_MAX");
        ok = false;
    } else {
        Serial.println("    ✅ CO2_GOOD_MAX < CO2_ACCEPTABLE_MAX");
    }
    
    if (CO2_GOOD_MAX <= 0) {
        Serial.println("    ❌ ERROR: CO2_GOOD_MAX debe ser positivo");
        ok = false;
    } else {
        Serial.println("    ✅ CO2_GOOD_MAX es positivo");
    }
    
    return ok;
}

bool checkTemperatureThresholds() {
    bool ok = true;
    Serial.println("  Verificando Temperatura:");
    Serial.printf("    TEMP_GOOD_MIN = %.1f, TEMP_GOOD_MAX = %.1f, TEMP_ACCEPTABLE_MAX = %.1f\n",
                  TEMP_GOOD_MIN, TEMP_GOOD_MAX, TEMP_ACCEPTABLE_MAX);
    
    if (TEMP_GOOD_MIN >= TEMP_GOOD_MAX) {
        Serial.println("    ❌ ERROR: TEMP_GOOD_MIN debe ser menor que TEMP_GOOD_MAX");
        ok = false;
    } else {
        Serial.println("    ✅ TEMP_GOOD_MIN < TEMP_GOOD_MAX");
    }
    
    if (TEMP_GOOD_MAX > TEMP_ACCEPTABLE_MAX) {
        Serial.println("    ❌ ERROR: TEMP_GOOD_MAX debe ser <= TEMP_ACCEPTABLE_MAX");
        ok = false;
    } else {
        Serial.println("    ✅ TEMP_GOOD_MAX <= TEMP_ACCEPTABLE_MAX");
    }
    
    if (TEMP_GOOD_MIN <= 0) {
        Serial.println("    ❌ ERROR: TEMP_GOOD_MIN debe ser positivo");
        ok = false;
    } else {
        Serial.println("    ✅ TEMP_GOOD_MIN es positivo");
    }
    
    // Verificar que el rango bueno está dentro del aceptable
    if (TEMP_GOOD_MIN < 0 || TEMP_GOOD_MAX > 50) {
        Serial.println("    ⚠️ ADVERTENCIA: Los valores de temperatura parecen poco realistas");
    }
    
    return ok;
}

bool checkHumidityThresholds() {
    bool ok = true;
    Serial.println("  Verificando Humedad:");
    Serial.printf("    HUM_GOOD_MIN = %d, HUM_GOOD_MAX = %d\n", 
                  HUM_GOOD_MIN, HUM_GOOD_MAX);
    Serial.printf("    HUM_ACCEPTABLE_MIN1 = %d, HUM_ACCEPTABLE_MAX1 = %d\n",
                  HUM_ACCEPTABLE_MIN1, HUM_ACCEPTABLE_MAX1);
    Serial.printf("    HUM_ACCEPTABLE_MIN2 = %d, HUM_ACCEPTABLE_MAX2 = %d\n",
                  HUM_ACCEPTABLE_MIN2, HUM_ACCEPTABLE_MAX2);
    
    // Rango bueno debe estar dentro del rango aceptable
    if (HUM_GOOD_MIN <= HUM_ACCEPTABLE_MIN1) {
        Serial.println("    ❌ ERROR: HUM_GOOD_MIN debe ser mayor que HUM_ACCEPTABLE_MIN1");
        ok = false;
    } else {
        Serial.println("    ✅ HUM_GOOD_MIN > HUM_ACCEPTABLE_MIN1");
    }
    
    if (HUM_GOOD_MAX >= HUM_ACCEPTABLE_MAX2) {
        Serial.println("    ❌ ERROR: HUM_GOOD_MAX debe ser menor que HUM_ACCEPTABLE_MAX2");
        ok = false;
    } else {
        Serial.println("    ✅ HUM_GOOD_MAX < HUM_ACCEPTABLE_MAX2");
    }
    
    if (HUM_GOOD_MIN >= HUM_GOOD_MAX) {
        Serial.println("    ❌ ERROR: HUM_GOOD_MIN debe ser menor que HUM_GOOD_MAX");
        ok = false;
    } else {
        Serial.println("    ✅ HUM_GOOD_MIN < HUM_GOOD_MAX");
    }
    
    // Verificar que los rangos aceptables no se solapan incorrectamente
    if (HUM_ACCEPTABLE_MAX1 >= HUM_ACCEPTABLE_MIN2) {
        Serial.println("    ❌ ERROR: HUM_ACCEPTABLE_MAX1 debe ser menor que HUM_ACCEPTABLE_MIN2");
        ok = false;
    } else {
        Serial.println("    ✅ HUM_ACCEPTABLE_MAX1 < HUM_ACCEPTABLE_MIN2");
    }
    
    if (HUM_ACCEPTABLE_MIN1 < 0 || HUM_ACCEPTABLE_MAX2 > 100) {
        Serial.println("    ❌ ERROR: Los rangos de humedad deben estar entre 0 y 100");
        ok = false;
    } else {
        Serial.println("    ✅ Rangos de humedad dentro de 0-100");
    }
    
    // Verificar coherencia del rango aceptable (30-40 y 60-70)
    if (HUM_ACCEPTABLE_MIN1 >= HUM_ACCEPTABLE_MAX1) {
        Serial.println("    ❌ ERROR: HUM_ACCEPTABLE_MIN1 debe ser menor que HUM_ACCEPTABLE_MAX1");
        ok = false;
    }
    
    if (HUM_ACCEPTABLE_MIN2 >= HUM_ACCEPTABLE_MAX2) {
        Serial.println("    ❌ ERROR: HUM_ACCEPTABLE_MIN2 debe ser menor que HUM_ACCEPTABLE_MAX2");
        ok = false;
    }
    
    return ok;
}

bool checkLightThresholds() {
    bool ok = true;
    Serial.println("  Verificando Luz:");
    Serial.printf("    LIGHT_GOOD_MAX = %d, LIGHT_ACCEPTABLE_MAX = %d\n",
                  LIGHT_GOOD_MAX, LIGHT_ACCEPTABLE_MAX);
    
    if (LIGHT_GOOD_MAX >= LIGHT_ACCEPTABLE_MAX) {
        Serial.println("    ❌ ERROR: LIGHT_GOOD_MAX debe ser menor que LIGHT_ACCEPTABLE_MAX");
        ok = false;
    } else {
        Serial.println("    ✅ LIGHT_GOOD_MAX < LIGHT_ACCEPTABLE_MAX");
    }
    
    if (LIGHT_GOOD_MAX < 0) {
        Serial.println("    ❌ ERROR: LIGHT_GOOD_MAX no puede ser negativo");
        ok = false;
    } else {
        Serial.println("    ✅ LIGHT_GOOD_MAX no es negativo");
    }
    
    return ok;
}

// ============================================================================
// setup() - Ejecuta todas las comprobaciones
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n==================================================");
    Serial.println("  LOGIC TEST: COHERENCIA DE UMBRALES");
    Serial.println("  Usando definiciones REALES de config.h");
    Serial.println("==================================================\n");

    bool allOk = true;

    Serial.println("[1] Verificando umbrales de CO₂...");
    if (!checkCO2Thresholds()) allOk = false;
    Serial.println();

    Serial.println("[2] Verificando umbrales de Temperatura...");
    if (!checkTemperatureThresholds()) allOk = false;
    Serial.println();

    Serial.println("[3] Verificando umbrales de Humedad...");
    if (!checkHumidityThresholds()) allOk = false;
    Serial.println();

    Serial.println("[4] Verificando umbrales de Luz...");
    if (!checkLightThresholds()) allOk = false;
    Serial.println();

    // ================================================================
    // RESUMEN FINAL
    // ================================================================
    Serial.println("==================================================");
    if (allOk) {
        Serial.println("  ✅ TODOS los umbrales son coherentes");
        Serial.println("  ✅ Los rangos BUENO están dentro de ACEPTABLE");
        Serial.println("  ✅ No hay valores negativos ni incoherencias");
    } else {
        Serial.println("  ❌ Se encontraron INCOHERENCIAS en los umbrales");
        Serial.println("  Revisa los mensajes de error arriba");
    }
    Serial.println("==================================================\n");
}

void loop() {
    delay(1000);
}