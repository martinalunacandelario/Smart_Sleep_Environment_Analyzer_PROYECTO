// test_logic/test_best_hours.cpp
// Propósito: Simular una sesión completa de 8 horas (datos cada 30 minutos)
// y encontrar la franja horaria de 1 hora con mejores condiciones
// (CO₂ bajo, temperatura óptima, humedad adecuada, poca luz).

#include <Arduino.h>
#include <vector>

struct SensorData {
    float co2;
    float temperature;
    float humidity;
    float light;
};

// ============================================================
// Función de puntuación para una hora (media de dos mediciones)
// A mayor puntuación, mejores condiciones para dormir.
// ============================================================
float computeHourScore(float co2, float temp, float hum, float lux) {
    float score = 100.0;

    // Penalización por CO₂ (lineal)
    if (co2 > 800) {
        score -= (co2 - 800) / 10.0;   // cada 10 ppm extra resta 1 punto
        if (co2 > 1200) score -= (co2 - 1200) / 5.0; // penalización extra si es muy malo
    }

    // Penalización por temperatura (óptimo 20-22°C)
    if (temp < 20.0) {
        score -= (20.0 - temp) * 5.0;
    } else if (temp > 22.0) {
        score -= (temp - 22.0) * 5.0;
    }

    // Penalización por humedad (óptimo 40-60%)
    if (hum < 40.0) {
        score -= (40.0 - hum) * 1.5;
    } else if (hum > 60.0) {
        score -= (hum - 60.0) * 1.5;
    }

    // Penalización por luz (fundamental)
    if (lux > 5.0) {
        score -= (lux - 5.0) * 2.0;
        if (lux > 20.0) score -= (lux - 20.0) * 3.0; // extra si es mucha luz
    }

    if (score < 0) score = 0;
    return score;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== TEST BEST HOURS (ventana de 1 hora) ===\n");

    // Simulación de 8 horas (16 puntos, cada 30 minutos)
    std::vector<SensorData> session = {
        {400, 20.0, 50, 0},   // minuto 0
        {450, 20.5, 51, 0},   // minuto 30
        {520, 21.0, 52, 0},   // minuto 60
        {580, 21.5, 52, 0},   // minuto 90
        {650, 21.5, 53, 0},   // minuto 120 (2h)
        {720, 21.0, 53, 0},   // minuto 150
        {800, 20.5, 54, 0},   // minuto 180
        {850, 20.0, 54, 0},   // minuto 210
        {900, 20.0, 55, 0},   // minuto 240 (4h)
        {950, 20.0, 55, 5},   // minuto 270 (amanecer)
        {980, 20.0, 55, 20},  // minuto 300
        {990, 20.0, 56, 50},  // minuto 330
        {1000, 20.0, 56, 100},// minuto 360
        {1010, 20.0, 56, 150},// minuto 390
        {1020, 20.0, 56, 200},// minuto 420
        {1030, 20.0, 56, 250} // minuto 450 (7.5h, fin)
    };

    int bestIndex = -1;          // índice del primer punto de la mejor ventana
    float bestScore = -1.0;

    // Buscar ventanas de 1 hora = 2 puntos consecutivos
    for (size_t i = 0; i < session.size() - 1; i++) {
        float avgCO2   = (session[i].co2 + session[i+1].co2) / 2.0;
        float avgTemp  = (session[i].temperature + session[i+1].temperature) / 2.0;
        float avgHum   = (session[i].humidity + session[i+1].humidity) / 2.0;
        float avgLux   = (session[i].light + session[i+1].light) / 2.0;

        float score = computeHourScore(avgCO2, avgTemp, avgHum, avgLux);
        int startMin = i * 30;          // minutos desde el inicio
        int endMin   = (i+2) * 30;      // minutos hasta el final de la ventana

        Serial.printf("Ventana %d-%d (%02d:%02d a %02d:%02d): Score = %.1f\n",
                      i, i+1,
                      startMin / 60, startMin % 60,
                      endMin / 60, endMin % 60,
                      score);

        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }

    // Mostrar la mejor ventana encontrada
    if (bestIndex != -1) {
        int startMin = bestIndex * 30;
        int endMin   = (bestIndex + 2) * 30;
        Serial.println("\n========================================");
        Serial.printf("🏆 MEJOR FRANJA HORARIA: %02d:%02d a %02d:%02d\n",
                      startMin / 60, startMin % 60,
                      endMin / 60, endMin % 60);
        Serial.printf("   Puntuación: %.1f\n", bestScore);
        Serial.println("========================================");
    } else {
        Serial.println("❌ No se encontró ninguna ventana válida.");
    }

    Serial.println("\n✅ Test best hours COMPLETADO");
}

void loop() {
    // Vacío: la prueba se ejecuta una sola vez
}