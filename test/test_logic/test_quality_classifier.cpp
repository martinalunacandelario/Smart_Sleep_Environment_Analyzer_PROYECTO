// test_logic/test_quality_classifier.cpp
// ============================================================================
// LOGIC TEST: Prueba de clasificación de calidad ambiental
// ============================================================================
// DESCRIPCIÓN: Verifica que las funciones REALES de AlertTask
//              clasifican correctamente según los umbrales de config.h
// ============================================================================

#include <Arduino.h>
#include "../../src/tasks/task_Alert.h"   // ← FUNCIONES REALES DEL PROYECTO
#include "../../include/config.h"        // ← UMBRALES REALES

// ============================================================================
// FUNCIÓN AUXILIAR: Clasifica usando las funciones REALES de AlertTask
// ============================================================================
const char* classifyEnvironment(float co2, float temp, float hum, float light) {
    // Creamos un objeto SensorData con los valores de prueba
    SensorData data;
    data.co2 = co2;
    data.temperature = temp;
    data.humidity = hum;
    data.light = light;
    
    // Usamos la función REAL de AlertTask
    int state = AlertTask::getGlobalState(data);
    
    switch (state) {
        case 0: return "OPTIMO";
        case 1: return "ACEPTABLE";
        case 2: return "DESFAVORABLE";
        default: return "DESCONOCIDO";
    }
}

// ============================================================================
// setup() - Ejecuta las pruebas
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== TEST QUALITY CLASSIFIER (CON CODIGO REAL) ===");
    Serial.println("Usando AlertTask::getGlobalState() y umbrales de config.h\n");

    // Estructura para cada prueba
    struct TestCase {
        float co2, temp, hum, light;
        const char* expected;
    };

    TestCase tests[] = {
        // Casos ideales (OPTIMO)
        { 400, 21.0, 50, 2,   "OPTIMO" },
        { 800, 20.0, 40, 0,   "OPTIMO" },
        { 799, 22.0, 60, 4,   "OPTIMO" },
        
        // Casos ACEPTABLE (regular)
        { 900, 21.0, 50, 2,   "ACEPTABLE" },  // CO2 regular
        { 800, 23.0, 50, 2,   "ACEPTABLE" },  // Temp regular
        { 800, 21.0, 35, 2,   "ACEPTABLE" },  // Hum regular (baja)
        { 800, 21.0, 65, 2,   "ACEPTABLE" },  // Hum regular (alta)
        { 800, 21.0, 50, 10,  "ACEPTABLE" },  // Luz regular
        
        // Casos DESFAVORABLE (malo)
        { 1300, 21.0, 50, 2,  "DESFAVORABLE" }, // CO2 malo
        { 800, 26.0, 50, 2,   "DESFAVORABLE" }, // Temp malo
        { 800, 21.0, 25, 2,   "DESFAVORABLE" }, // Hum malo (baja)
        { 800, 21.0, 75, 2,   "DESFAVORABLE" }, // Hum malo (alta)
        { 800, 21.0, 50, 25,  "DESFAVORABLE" }, // Luz malo
        
        // Múltiples parámetros
        { 1300, 26.0, 75, 30, "DESFAVORABLE" },
        { 1000, 23.5, 65, 10, "ACEPTABLE" },
        { 400, 21.0, 80, 0,   "DESFAVORABLE" },
    };

    int numTests = sizeof(tests) / sizeof(tests[0]);
    int passed = 0;

    for (int i = 0; i < numTests; i++) {
        const char* result = classifyEnvironment(tests[i].co2, tests[i].temp,
                                                  tests[i].hum, tests[i].light);
        bool ok = (strcmp(result, tests[i].expected) == 0);

        Serial.printf("Test %2d: CO2=%4.0f T=%4.1f H=%3.0f Lux=%3.0f -> %-12s [Esperado: %-12s] %s\n",
                      i+1,
                      tests[i].co2, tests[i].temp, tests[i].hum, tests[i].light,
                      result, tests[i].expected,
                      ok ? "✅" : "❌");

        if (ok) passed++;
    }

    Serial.println("\n========================================");
    Serial.printf("RESULTADO: %d de %d pruebas PASARON\n", passed, numTests);
    if (passed == numTests) {
        Serial.println("✅ TODAS LAS PRUEBAS SON CORRECTAS");
        Serial.println("✅ AlertTask::getGlobalState() funciona como se espera");
    } else {
        Serial.println("❌ ALGUNAS PRUEBAS FALLARON");
        Serial.println("   Revisa la lógica en AlertTask.cpp");
    }
    Serial.println("========================================\n");
}

void loop() {
    delay(1000);
}