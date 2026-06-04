// test_logic/test_averages.cpp
// Prueba de medias, máximos y mínimos de una sesión simulada
// Simula una sesión de 8 horas con datos cada 30 minutos.

#include <Arduino.h>
#include <vector>

struct SensorData {
    float co2;
    float temperature;
    float humidity;
    float light;
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== TEST MEDIAS, MÁXIMOS Y MÍNIMOS ===\n");

    // Simular una sesión de 8 horas (16 puntos, cada 30 min)
    std::vector<SensorData> session = {
        {400, 20.0, 50, 0},   // inicio
        {450, 20.5, 51, 0},
        {520, 21.0, 52, 0},
        {580, 21.5, 52, 0},
        {650, 21.5, 53, 0},   // hora 2
        {720, 21.0, 53, 0},
        {800, 20.5, 54, 0},
        {850, 20.0, 54, 0},
        {900, 20.0, 55, 0},   // hora 4
        {950, 20.0, 55, 5},   // amanecer
        {980, 20.0, 55, 20},  // luz sube
        {990, 20.0, 56, 50},  // luz alta
        {1000, 20.0, 56, 100},
        {1010, 20.0, 56, 150},
        {1020, 20.0, 56, 200},
        {1030, 20.0, 56, 250} // fin
    };

    // Calcular estadísticas
    float co2_sum = 0, temp_sum = 0, hum_sum = 0, light_sum = 0;
    float co2_max = 0, co2_min = 1e9;
    float temp_max = -100, temp_min = 100;
    float hum_max = 0, hum_min = 100;
    float light_max = 0, light_min = 1e9;

    for (const auto& d : session) {
        co2_sum += d.co2;
        temp_sum += d.temperature;
        hum_sum += d.humidity;
        light_sum += d.light;

        if (d.co2 > co2_max) co2_max = d.co2;
        if (d.co2 < co2_min) co2_min = d.co2;
        if (d.temperature > temp_max) temp_max = d.temperature;
        if (d.temperature < temp_min) temp_min = d.temperature;
        if (d.humidity > hum_max) hum_max = d.humidity;
        if (d.humidity < hum_min) hum_min = d.humidity;
        if (d.light > light_max) light_max = d.light;
        if (d.light < light_min) light_min = d.light;
    }

    int n = session.size();
    Serial.printf("CO2      - Media: %.0f ppm, Máx: %.0f, Mín: %.0f\n",
                  co2_sum / n, co2_max, co2_min);
    Serial.printf("Temperatura - Media: %.1f °C, Máx: %.1f, Mín: %.1f\n",
                  temp_sum / n, temp_max, temp_min);
    Serial.printf("Humedad  - Media: %.0f %%, Máx: %.0f, Mín: %.0f\n",
                  hum_sum / n, hum_max, hum_min);
    Serial.printf("Luz      - Media: %.0f lux, Máx: %.0f, Mín: %.0f\n",
                  light_sum / n, light_max, light_min);

    Serial.println("\n✅ Test de estadísticas COMPLETADO");
}

void loop() {
    // Vacío: la prueba se ejecuta una sola vez
}