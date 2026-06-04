// test_sleep_score.cpp
// Prueba del cálculo del Sleep Score - Versión para monitor serie real
// Compila y sube a tu placa, luego abre el monitor serie a 115200 baudios

#include <Arduino.h>

// ============================================================
// Umbrales (simulando config.h)
// ============================================================
#define CO2_GOOD_MAX         800
#define CO2_ACCEPTABLE_MAX  1200

#define TEMP_OPTIMAL_MIN     20.0
#define TEMP_OPTIMAL_MAX     22.0
#define TEMP_ACCEPTABLE_MIN  18.0
#define TEMP_ACCEPTABLE_MAX  25.0

#define HUM_OPTIMAL_MIN      40
#define HUM_OPTIMAL_MAX      60
#define HUM_ACCEPTABLE_MIN   30
#define HUM_ACCEPTABLE_MAX   70

#define LUX_SLEEP_GOOD        5
#define LUX_SLEEP_ACCEPT     20

// ============================================================
// Función que calcula el Sleep Score (0 a 100)
// ============================================================
int computeSleepScore(float co2, float temp, float hum, float lux) {
    int score = 100;

    if (co2 > CO2_ACCEPTABLE_MAX) {
        score -= 40;
    } else if (co2 > CO2_GOOD_MAX) {
        score -= 20;
    }

    if (temp < TEMP_OPTIMAL_MIN || temp > TEMP_OPTIMAL_MAX) {
        if (temp < TEMP_ACCEPTABLE_MIN || temp > TEMP_ACCEPTABLE_MAX) {
            score -= 30;
        } else {
            score -= 15;
        }
    }

    if (hum < HUM_OPTIMAL_MIN || hum > HUM_OPTIMAL_MAX) {
        if (hum < HUM_ACCEPTABLE_MIN || hum > HUM_ACCEPTABLE_MAX) {
            score -= 30;
        } else {
            score -= 15;
        }
    }

    if (lux > LUX_SLEEP_ACCEPT) {
        score -= 40;
    } else if (lux > LUX_SLEEP_GOOD) {
        score -= 20;
    }

    if (score < 0) score = 0;
    return score;
}

// ============================================================
// setup() - Se ejecuta una vez al inicio
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(1000);  // Espera a que el monitor serie esté listo
    Serial.println("\n=== TEST SLEEP SCORE ===");
    Serial.println("(Resultados visibles en monitor serie a 115200 baudios)\n");

    // --- Caso 1: Ideal ---
    float co2 = 800, temp = 21.0, hum = 50, lux = 2;
    int score = computeSleepScore(co2, temp, hum, lux);
    Serial.print("1. IDEAL (CO2=800, T=21, H=50, Lux=2): Score = ");
    Serial.print(score);
    Serial.println(score == 100 ? " -> CORRECTO (100)" : " -> ERROR");

    // --- Caso 2: Muy malo ---
    co2 = 1500; temp = 28.0; hum = 80; lux = 30;
    score = computeSleepScore(co2, temp, hum, lux);
    Serial.print("2. MALO (CO2=1500, T=28, H=80, Lux=30): Score = ");
    Serial.print(score);
    Serial.println(score < 50 ? " -> CORRECTO (bajo)" : " -> ERROR");

    // --- Caso 3: CO2 aceptable, temperatura y humedad fuera del óptimo, luz media ---
    co2 = 1100; temp = 23.5; hum = 65; lux = 10;
    score = computeSleepScore(co2, temp, hum, lux);
    Serial.print("3. INTERMEDIO A: Score = ");
    Serial.println(score);

    // --- Caso 4: Solo CO2 alto pero aceptable, resto perfecto, sin luz ---
    co2 = 1100; temp = 21.0; hum = 50; lux = 0;
    score = computeSleepScore(co2, temp, hum, lux);
    Serial.print("4. CO2=1100, resto perfecto: Score = ");
    Serial.print(score);
    Serial.println(score == 80 ? " -> CORRECTO (80)" : " -> ERROR");

    // --- Caso 5: Luz extrema ---
    co2 = 600; temp = 21.0; hum = 50; lux = 100;
    score = computeSleepScore(co2, temp, hum, lux);
    Serial.print("5. Luz extrema (100 lux): Score = ");
    Serial.println(score);

    Serial.println("\n✅ Test completado. Reinicia la placa para volver a ejecutarlo.");
}

void loop() {
    // Vacío: solo queremos que se ejecute una vez
}