// test_logic/test_averages.cpp
// ============================================================================
// LOGIC TEST: Prueba de medias, máximos, mínimos y Sleep Score
// ============================================================================
// DESCRIPCIÓN: Este test verifica que las funciones reales del proyecto
//              (calculateStatistics, calculateSleepScore, findBestHour)
//              funcionan correctamente con datos simulados.
//              A diferencia de un test manual, aquí NO se usan sensores reales,
//              sino datos ficticios para comprobar la lógica del código.
// ============================================================================

#include <Arduino.h>
#include <vector>
#include "../../src/tasks/task_Analysis.h"   // ← FUNCIONES REALES DEL PROYECTO
#include "../../src/tasks/task_Storage.h"   // ← ESTRUCTURAS REALES

// ============================================================================
// FUNCIÓN AUXILIAR: Crea datos de prueba para una sesión de 8 horas
// ============================================================================
// Simula una sesión con 16 puntos (uno cada 30 minutos = 8 horas)
// Los datos simulan una noche donde:
//   - CO₂ sube progresivamente (400 → 1030 ppm)
//   - Temperatura sube ligeramente (20.0 → 21.5°C)
//   - Humedad sube ligeramente (50 → 56%)
//   - Luz = 0 hasta el amanecer (hora 4), luego sube hasta 250 lux
// ============================================================================
SessionStats crearDatosDePrueba() {
    SessionStats stats;  // ← ESTRUCTURA REAL DEL PROYECTO
    
    // Configurar metadatos de la sesión
    stats.sessionId = 1;
    stats.date = "2026-06-21";
    stats.startTime = "00:00:00";
    stats.endTime = "08:00:00";
    stats.sessionStartEpoch = 1782000000;  // Epoch de inicio (simulado)
    stats.duration = 28800;  // 8 horas en segundos
    
    // Limpiar vectores por si acaso
    stats.timestamps.clear();
    stats.co2_values.clear();
    stats.temp_values.clear();
    stats.hum_values.clear();
    stats.light_values.clear();
    
    // Generar 16 puntos (cada 30 minutos = 1800 segundos)
    for (int i = 0; i < 16; i++) {
        stats.timestamps.push_back(i * 1800 * 1000);  // Timestamp en ms
        
        // CO₂: sube gradualmente de 400 a 1030 ppm
        stats.co2_values.push_back(400 + i * 42);
        
        // Temperatura: sube ligeramente de 20.0 a 21.5°C
        stats.temp_values.push_back(20.0 + i * 0.1);
        
        // Humedad: sube ligeramente de 50 a 56%
        stats.hum_values.push_back(50 + i * 0.4);
        
        // Luz: 0 hasta el punto 8 (hora 4), luego sube progresivamente
        float light = 0;
        if (i >= 8) {
            light = (i - 8) * 35;  // 0, 35, 70, 105, 140, 175, 210, 245
        }
        stats.light_values.push_back(light);
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
    Serial.println("  LOGIC TEST: ESTADÍSTICAS Y SLEEP SCORE");
    Serial.println("  Probando funciones reales de AnalysisTask");
    Serial.println("==================================================\n");

    // ================================================================
    // 1. CREAR DATOS DE PRUEBA
    // ================================================================
    Serial.println("[1] Generando datos de prueba (8 horas, 16 puntos)...");
    SessionStats stats = crearDatosDePrueba();
    Serial.printf("    ✅ %d puntos generados\n\n", (int)stats.timestamps.size());

    // ================================================================
    // 2. PROBAR calculateStatistics() - FUNCIÓN REAL
    // ================================================================
    Serial.println("[2] Probando AnalysisTask::calculateStatistics()...");
    
    // Llamar a la función REAL del proyecto
    AnalysisTask::calculateStatistics(stats);
    
    // Verificar resultados esperados (calculados manualmente)
    bool co2_ok = casiIgual(stats.co2_avg, 736.25) && 
                  casiIgual(stats.co2_max, 1030.0) && 
                  casiIgual(stats.co2_min, 400.0);
    
    bool temp_ok = casiIgual(stats.temp_avg, 20.75) && 
                   casiIgual(stats.temp_max, 21.5) && 
                   casiIgual(stats.temp_min, 20.0);
    
    bool hum_ok = casiIgual(stats.hum_avg, 53.0) && 
                  casiIgual(stats.hum_max, 56.0) && 
                  casiIgual(stats.hum_min, 50.0);
    
    bool light_ok = casiIgual(stats.light_avg, 61.25) && 
                    casiIgual(stats.light_max, 245.0) && 
                    casiIgual(stats.light_min, 0.0);
    
    // Mostrar resultados
    Serial.printf("    CO2   - Media: %.0f, Max: %.0f, Min: %.0f (Score: %d)\n",
                  stats.co2_avg, stats.co2_max, stats.co2_min, stats.co2_score);
    Serial.printf("    Temp  - Media: %.1f, Max: %.1f, Min: %.1f (Score: %d)\n",
                  stats.temp_avg, stats.temp_max, stats.temp_min, stats.temp_score);
    Serial.printf("    Hum   - Media: %.0f, Max: %.0f, Min: %.0f (Score: %d)\n",
                  stats.hum_avg, stats.hum_max, stats.hum_min, stats.hum_score);
    Serial.printf("    Luz   - Media: %.0f, Max: %.0f, Min: %.0f (Score: %d)\n",
                  stats.light_avg, stats.light_max, stats.light_min, stats.light_score);
    
    imprimirResultado("CO2", co2_ok);
    imprimirResultado("Temperatura", temp_ok);
    imprimirResultado("Humedad", hum_ok);
    imprimirResultado("Luz", light_ok);
    Serial.println();

    // ================================================================
    // 3. PROBAR calculateSleepScore() - FUNCIÓN REAL
    // ================================================================
    Serial.println("[3] Probando AnalysisTask::calculateSleepScore()...");
    
    int sleepScore = AnalysisTask::calculateSleepScore(stats);
    String interpretacion = AnalysisTask::getInterpretation(sleepScore);
    
    Serial.printf("    Sleep Score: %d/100\n", sleepScore);
    Serial.printf("    Interpretación: %s\n", interpretacion.c_str());
    
    // Verificar que el score está en el rango correcto (0-100)
    bool score_ok = (sleepScore >= 0 && sleepScore <= 100);
    imprimirResultado("Sleep Score en rango 0-100", score_ok);
    Serial.println();

    // ================================================================
    // 4. PROBAR findBestHour() - FUNCIÓN REAL
    // ================================================================
    Serial.println("[4] Probando AnalysisTask::findBestHour()...");
    Serial.println("    Buscando la mejor franja de 1 hora...");
    
    AnalysisTask::findBestHour(stats);
    
    if (stats.bestHourValid) {
        // Si NTP está disponible, mostrar hora real
        if (stats.sessionStartEpoch > 0 && stats.bestHourStart > 1000000000) {
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
        imprimirResultado("findBestHour() funcionó", true);
    } else {
        Serial.println("    ⚠️ No se pudo determinar la mejor franja (¿datos insuficientes?)");
        imprimirResultado("findBestHour() funcionó", false);
    }
    Serial.println();

    // ================================================================
    // 5. VERIFICAR PUNTUACIONES PARCIALES (según la especificación)
    // ================================================================
    Serial.println("[5] Verificando puntuaciones según la especificación...");
    
    // Verificar que los scores están en los rangos correctos
    bool co2_score_ok = (stats.co2_score >= 0 && stats.co2_score <= 40);
    bool temp_score_ok = (stats.temp_score >= 0 && stats.temp_score <= 25);
    bool hum_score_ok = (stats.hum_score >= 0 && stats.hum_score <= 20);
    bool light_score_ok = (stats.light_score >= 0 && stats.light_score <= 15);
    
    imprimirResultado("CO2 score en rango 0-40", co2_score_ok);
    imprimirResultado("Temp score en rango 0-25", temp_score_ok);
    imprimirResultado("Hum score en rango 0-20", hum_score_ok);
    imprimirResultado("Light score en rango 0-15", light_score_ok);
    Serial.println();

    // ================================================================
    // 6. RESUMEN FINAL
    // ================================================================
    Serial.println("==================================================");
    bool todosOk = co2_ok && temp_ok && hum_ok && light_ok && 
                   score_ok && co2_score_ok && temp_score_ok && 
                   hum_score_ok && light_score_ok && stats.bestHourValid;
    
    if (todosOk) {
        Serial.println("  🎉 ¡TODOS LOS TESTS PASARON CORRECTAMENTE!");
        Serial.println("  ✅ Las funciones de AnalysisTask funcionan como se espera.");
    } else {
        Serial.println("  ⚠️ ALGUNOS TESTS FALLARON.");
        Serial.println("  Revisa la lógica de AnalysisTask para identificar el problema.");
    }
    Serial.println("==================================================\n");
}

// ============================================================================
// LOOP - Vacío, el test se ejecuta una sola vez
// ============================================================================
void loop() {
    // Nada que hacer aquí
    delay(1000);
}