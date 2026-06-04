// test_quality_classifier.cpp
// Prueba la función de clasificación (OPTIMO / ACEPTABLE / DESFAVORABLE)
// basada en los mismos umbrales usados en DisplayTask y AlertTask.

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

// Humedad (rangos: aceptable tiene dos intervalos, pero DisplayTask usa MIN1 y MAX2)
#define HUM_GOOD_MIN         40    // %
#define HUM_GOOD_MAX         60    // %
#define HUM_ACCEPTABLE_MIN1  30    // %
#define HUM_ACCEPTABLE_MAX1  40    // % (realmente es hasta 40, pero cuidado)
#define HUM_ACCEPTABLE_MIN2  60    // %
#define HUM_ACCEPTABLE_MAX2  70    // %
// Para simplificar, usaremos la lógica de DisplayTask:
//   - Malo: hum < 30 o hum > 70
//   - Regular: (hum entre 30 y 40) o (hum entre 60 y 70)
//   - Bueno: hum entre 40 y 60

// Luz
#define LIGHT_GOOD_MAX        5    // lux
#define LIGHT_ACCEPTABLE_MAX 20    // lux

// ============================================================
// Función que devuelve el estado global (0=BUENO, 1=REGULAR, 2=MALO)
// misma lógica que AlertTask::getGlobalState()
// ============================================================
int getCo2State(float co2) {
    if (co2 < CO2_GOOD_MAX)        return 0;
    if (co2 <= CO2_ACCEPTABLE_MAX) return 1;
    return 2;
}

int getTempState(float temp) {
    if (temp >= TEMP_GOOD_MIN && temp <= TEMP_GOOD_MAX) return 0;
    if (temp <= TEMP_ACCEPTABLE_MAX)                    return 1;
    return 2;
}

int getHumState(float hum) {
    // Según lógica de DisplayTask (y compatible con AlertTask)
    if (hum >= HUM_GOOD_MIN && hum <= HUM_GOOD_MAX) return 0;
    if ((hum >= HUM_ACCEPTABLE_MIN1 && hum < HUM_GOOD_MIN) ||
        (hum > HUM_GOOD_MAX && hum <= HUM_ACCEPTABLE_MAX2)) return 1;
    return 2;
}

int getLightState(float light) {
    if (light < LIGHT_GOOD_MAX)       return 0;
    if (light < LIGHT_ACCEPTABLE_MAX) return 1;
    return 2;
}

int getGlobalState(float co2, float temp, float hum, float light) {
    int states[4];
    states[0] = getCo2State(co2);
    states[1] = getTempState(temp);
    states[2] = getHumState(hum);
    states[3] = getLightState(light);

    int worst = 0;
    for (int i = 0; i < 4; i++) {
        if (states[i] > worst) worst = states[i];
    }
    return worst;
}

// ============================================================
// Función que devuelve el texto de clasificación
// ============================================================
const char* classifyEnvironment(float co2, float temp, float hum, float light) {
    int state = getGlobalState(co2, temp, hum, light);
    switch (state) {
        case 0: return "OPTIMO";
        case 1: return "ACEPTABLE";
        case 2: return "DESFAVORABLE";
        default: return "DESCONOCIDO";
    }
}

// ============================================================
// setup() - Ejecutar pruebas y mostrar resultados por Serial
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== TEST QUALITY CLASSIFIER ===");
    Serial.println("(Basado en umbrales de DisplayTask / AlertTask)\n");

    // Estructura para cada prueba: {co2, temp, hum, light, esperado}
    struct TestCase {
        float co2, temp, hum, light;
        const char* expected;
    };

    TestCase tests[] = {
        // Casos ideales
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
        // Múltiples parámetros fuera
        { 1300, 26.0, 75, 30, "DESFAVORABLE" },
        { 1000, 23.5, 65, 10, "ACEPTABLE" },   // Mixto pero ninguno malo
        { 400, 21.0, 80, 0,   "DESFAVORABLE" }, // Hum malo pese a lo demás bueno
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
    } else {
        Serial.println("❌ ALGUNAS PRUEBAS FALLARON - Revisa la lógica");
    }
    Serial.println("========================================\n");
}

void loop() {
    // Vacío: la prueba se ejecuta una sola vez al inicio
}