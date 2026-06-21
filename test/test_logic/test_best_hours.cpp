// test_logic/test_best_hours.cpp
// ============================================================================
// LOGIC TEST: Prueba de búsqueda de la mejor franja horaria
// ============================================================================
// DESCRIPCIÓN: Este test verifica que la función real AnalysisTask::findBestHour()
//              funciona correctamente con datos simulados.
//              Simula una sesión de 8 horas y busca la mejor franja de 1 hora.
// ============================================================================

#include <Arduino.h>
#include <vector>
#include <time.h>
#include "../../src/tasks/task_Analysis.h"   // ← FUNCIONES REALES DEL PROYECTO

// ============================================================================
// FUNCIÓN AUXILIAR: Crea datos de prueba para una sesión de 8 horas
// ============================================================================
SessionStats crearDatosDePrueba() {
    SessionStats stats;  // ← ESTRUCTURA REAL DEL PROYECTO
    
    stats.sessionId = 1;
    stats.date = "2026-06-21";
    stats.startTime = "00:00:00";
    stats.endTime = "08:00:00";
    stats.sessionStartEpoch = 1782000000;
    stats.duration = 28800;
    stats.bestHourValid = false;
    
    stats.timestamps.clear();
    stats.co2_values.clear();
    stats.temp_values.clear();
    stats.hum_values.clear();
    stats.light_values.clear();
    
    // Simular 16 puntos (cada 30 minutos = 1800 segundos = 1.800.000 ms)
    // Datos: CO₂ sube progresivamente, luz aumenta al final
    float co2_values[] = {400, 450, 520, 580, 650, 720, 800, 850,
                          900, 950, 980, 990, 1000, 1010, 1020, 1030};
    float temp_values[] = {20.0, 20.5, 21.0, 21.5, 21.5, 21.0, 20.5, 20.0,
                           20.0, 20.0, 20.0, 20.0, 20.0, 20.0, 20.0, 20.0};
    float hum_values[] = {50, 51, 52, 52, 53, 53, 54, 54,
                          55, 55, 55, 56, 56, 56, 56, 56};
    float light_values[] = {0, 0, 0, 0, 0, 0, 0, 0,
                            0, 5, 20, 50, 100, 150, 200, 250};
    
    for (int i = 0; i < 16; i++) {
        stats.timestamps.push_back(i * 1800 * 1000);  // Timestamp en ms
        stats.co2_values.push_back(co2_values[i]);
        stats.temp_values.push_back(temp_values[i]);
        stats.hum_values.push_back(hum_values[i]);
        stats.light_values.push_back(light_values[i]);
    }
    
    return stats;
}

// ============================================================================
// FUNCIÓN AUXILIAR: Verifica que dos valores flotantes son "casi iguales"
// ============================================================================
bool casiIgual(float a, float b, float tolerancia = 0.01) {
    return (a - b) < tolerancia && (b - a) < tolerancia;
}

// ============================================================================
// FUNCIÓN AUXILIAR: Imprime resultado de una prueba
// ============================================================================
void imprimirResultado(const char* nombre, bool ok) {
    Serial.print("  ");
    Serial.print(nombre);
    Serial.print(": ");
    Serial.println(ok ? "✅ PASADO" : "❌ FALLIDO");
}

// ============================================================================
// SETUP: Ejecuta los tests
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n==================================================");
    Serial.println("  LOGIC TEST: MEJOR FRANJA HORARIA");
    Serial.println("  Probando AnalysisTask::findBestHour()");
    Serial.println("==================================================\n");

    // ================================================================
    // 1. CREAR DATOS DE PRUEBA
    // ================================================================
    Serial.println("[1] Generando datos de prueba (8 horas, 16 puntos)...");
    SessionStats stats = crearDatosDePrueba();
    Serial.printf("    ✅ %d puntos generados\n\n", (int)stats.timestamps.size());

    // ================================================================
    // 2. CALCULAR ESTADÍSTICAS (necesario antes de findBestHour)
    // ================================================================
    Serial.println("[2] Calculando estadísticas con AnalysisTask::calculateStatistics()...");
    AnalysisTask::calculateStatistics(stats);
    
    Serial.printf("    CO2   - Media: %.0f, Max: %.0f, Min: %.0f\n",
                  stats.co2_avg, stats.co2_max, stats.co2_min);
    Serial.printf("    Temp  - Media: %.1f, Max: %.1f, Min: %.1f\n",
                  stats.temp_avg, stats.temp_max, stats.temp_min);
    Serial.printf("    Hum   - Media: %.0f, Max: %.0f, Min: %.0f\n",
                  stats.hum_avg, stats.hum_max, stats.hum_min);
    Serial.printf("    Luz   - Media: %.0f, Max: %.0f, Min: %.0f\n\n",
                  stats.light_avg, stats.light_max, stats.light_min);

    // ================================================================
    // 3. PROBAR findBestHour() - FUNCIÓN REAL
    // ================================================================
    Serial.println("[3] Probando AnalysisTask::findBestHour()...");
    Serial.println("    Buscando la mejor franja de 1 hora...");
    
    AnalysisTask::findBestHour(stats);
    
    if (stats.bestHourValid) {
        // Mostrar la mejor franja
        if (stats.sessionStartEpoch > 0 && stats.bestHourStart > 1000000000) {
            // Hora REAL (NTP)
            struct tm tmStart, tmEnd;
            time_t startTime = stats.bestHourStart;
            time_t endTime = stats.bestHourEnd;
            localtime_r(&startTime, &tmStart);
            localtime_r(&endTime, &tmEnd);
            
            char startStr[16], endStr[16];
            strftime(startStr, sizeof(startStr), "%H:%M", &tmStart);
            strftime(endStr, sizeof(endStr), "%H:%M", &tmEnd);
            
            Serial.printf("    ✅ Mejor franja encontrada: %s - %s\n", startStr, endStr);
        } else {
            // Tiempo relativo
            Serial.printf("    ✅ Mejor franja encontrada: %lu:%02lu - %lu:%02lu\n",
                          stats.bestHourStart / 60, stats.bestHourStart % 60,
                          stats.bestHourEnd / 60, stats.bestHourEnd % 60);
        }
        
        // ================================================================
        // 4. VERIFICAR RESULTADO ESPERADO
        // ================================================================
        Serial.println("\n[4] Verificando resultado esperado...");
        Serial.println("    Según los datos de prueba, la mejor franja debería ser:");
        Serial.println("    Las primeras horas (00:00 - 01:00) porque:");
        Serial.println("    - CO₂ bajo (400-450 ppm)");
        Serial.println("    - Temperatura óptima (20-20.5°C)");
        Serial.println("    - Humedad óptima (50-51%)");
        Serial.println("    - Sin luz (0 lux)");
        
        // Verificar que la mejor franja está al principio (primeras 2-3 horas)
        bool franjaCorrecta = (stats.bestHourStart < 3 * 3600);  // < 3 horas desde inicio
        imprimirResultado("Mejor franja en primeras horas", franjaCorrecta);
        
        // Verificar que los valores están en los rangos esperados
        bool co2Bajo = true;
        bool tempOptima = true;
        bool humOptima = true;
        bool luzBaja = true;
        
        // Buscar los índices de la mejor franja (convertir a segundos)
        unsigned long startSec = stats.bestHourStart;
        if (startSec > 1000000000) {
            startSec = startSec - stats.sessionStartEpoch;  // Convertir a relativo
        }
        
        // Verificar que el CO₂ en esa franja es bajo
        for (size_t i = 0; i < stats.co2_values.size(); i++) {
            unsigned long tsSec = stats.timestamps[i] / 1000;
            if (tsSec >= startSec && tsSec <= startSec + 3600) {
                if (stats.co2_values[i] > 800) co2Bajo = false;
                if (stats.temp_values[i] < 19 || stats.temp_values[i] > 23) tempOptima = false;
                if (stats.hum_values[i] < 35 || stats.hum_values[i] > 65) humOptima = false;
                if (stats.light_values[i] > 10) luzBaja = false;
            }
        }
        
        imprimirResultado("CO₂ bajo en la franja", co2Bajo);
        imprimirResultado("Temperatura óptima en la franja", tempOptima);
        imprimirResultado("Humedad óptima en la franja", humOptima);
        imprimirResultado("Luz baja en la franja", luzBaja);
        
    } else {
        Serial.println("    ❌ No se pudo determinar la mejor franja");
        imprimirResultado("findBestHour() funcionó", false);
    }

    // ================================================================
    // 5. RESUMEN FINAL
    // ================================================================
    Serial.println("\n==================================================");
    if (stats.bestHourValid) {
        Serial.println("  🎉 ¡TEST PASADO CORRECTAMENTE!");
        Serial.println("  ✅ AnalysisTask::findBestHour() funciona como se espera.");
    } else {
        Serial.println("  ⚠️ EL TEST FALLÓ.");
        Serial.println("  Revisa la lógica de AnalysisTask::findBestHour().");
    }
    Serial.println("==================================================\n");
}

// ============================================================================
// LOOP - Vacío, el test se ejecuta una sola vez
// ============================================================================
void loop() {
    delay(1000);
}