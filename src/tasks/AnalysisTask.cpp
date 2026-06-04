#include "AnalysisTask.h"
#include "../../include/config.h"
#include <SPI.h>
#include <SD.h>

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================
TaskHandle_t AnalysisTask::_taskHandle = nullptr;
QueueHandle_t AnalysisTask::_cmdQueue = nullptr;
unsigned long* AnalysisTask::_sessionCounter = nullptr;

// ============================================================================
// start() - Inicializa y crea la tarea
// ============================================================================
void AnalysisTask::start(QueueHandle_t cmdQueue, unsigned long* sessionCounter) {
    _cmdQueue = cmdQueue;
    _sessionCounter = sessionCounter;
    
    xTaskCreatePinnedToCore(
        taskFunction,
        "AnalysisTask",
        ANALYSIS_TASK_STACK,
        nullptr,
        ANALYSIS_TASK_PRIORITY,
        &_taskHandle,
        1
    );
}

// ============================================================================
// taskFunction() - Bucle principal
// ============================================================================
void AnalysisTask::taskFunction(void* pvParams) {
    DisplayCommand cmd;
    
    while (true) {
        if (xQueueReceive(_cmdQueue, &cmd, 0) == pdTRUE) {
            if (!cmd.sessionActive && _sessionCounter != nullptr) {
                Serial.println("[Analysis] Fin de sesión detectado. Analizando datos...");
                
                String filename = getLastSessionFileName();
                
                if (filename.length() > 0) {
                    SessionStats stats;
                    stats.sessionId = *_sessionCounter;
                    
                    if (readSessionFile(filename, stats)) {
                        calculateStatistics(stats);
                        findBestHour(stats);
                        stats.sleepScore = calculateSleepScore(stats);
                        stats.interpretation = getInterpretation(stats.sleepScore);
                        saveStatisticsToSD(stats);
                        
                        // Mostrar resumen por Serial
                        Serial.println("\n[Analysis] ====== RESUMEN DE LA SESIÓN ======");
                        Serial.printf("Sesión #%lu\n", stats.sessionId);
                        
                        // Duración en horas y minutos
                        unsigned long horas = stats.duration / 3600;
                        unsigned long minutos = (stats.duration % 3600) / 60;
                        Serial.printf("Duración: %lu h %lu min\n", horas, minutos);
                        
                        Serial.printf("CO2   - Media: %.0f ppm, Max: %.0f, Min: %.0f (Score: %d)\n", 
                                      stats.co2_avg, stats.co2_max, stats.co2_min, stats.co2_score);
                        Serial.printf("Temp  - Media: %.1f°C, Max: %.1f, Min: %.1f (Score: %d)\n", 
                                      stats.temp_avg, stats.temp_max, stats.temp_min, stats.temp_score);
                        Serial.printf("Hum   - Media: %.0f%%, Max: %.0f, Min: %.0f (Score: %d)\n", 
                                      stats.hum_avg, stats.hum_max, stats.hum_min, stats.hum_score);
                        Serial.printf("Luz   - Media: %.0f lux, Max: %.0f, Min: %.0f (Score: %d)\n", 
                                      stats.light_avg, stats.light_max, stats.light_min, stats.light_score);
                        Serial.printf("\nSLEEP SCORE: %d/100 - %s\n", stats.sleepScore, stats.interpretation.c_str());
                        
                        if (stats.bestHourStart > 0 || stats.bestHourEnd > 0) {
                            Serial.printf("Mejor franja: %lu:%02lu - %lu:%02lu\n", 
                                          stats.bestHourStart / 60, stats.bestHourStart % 60,
                                          stats.bestHourEnd / 60, stats.bestHourEnd % 60);
                        }
                        Serial.println("========================================\n");
                    } else {
                        Serial.println("[Analysis] Error al leer el archivo de la sesión");
                    }
                } else {
                    Serial.println("[Analysis] No se encontró archivo para analizar");
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// getLastSessionFileName() - Obtiene el nombre del archivo CSV de la última sesión
// ============================================================================
String AnalysisTask::getLastSessionFileName() {
    if (_sessionCounter == nullptr) return "";
    
    File root = SD.open(SD_BASE_PATH);
    if (!root) return "";
    
    File file = root.openNextFile();
    String targetFile = "";
    
    while (file) {
        String name = file.name();
        char searchPattern[32];
        snprintf(searchPattern, sizeof(searchPattern), "session_%03lu", *_sessionCounter);
        
        if (name.indexOf(searchPattern) >= 0 && name.endsWith(".csv")) {
            targetFile = name;
            file.close();
            break;
        }
        file = root.openNextFile();
    }
    root.close();
    
    return targetFile;
}

// ============================================================================
// readSessionFile() - Lee el archivo CSV y guarda los datos en vectores
// ============================================================================
bool AnalysisTask::readSessionFile(const String& filename, SessionStats &stats) {
    File file = SD.open(filename.c_str(), FILE_READ);
    if (!file) {
        Serial.printf("[Analysis] Error al abrir archivo: %s\n", filename.c_str());
        return false;
    }
    
    stats.timestamps.clear();
    stats.co2_values.clear();
    stats.temp_values.clear();
    stats.hum_values.clear();
    stats.light_values.clear();
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        
        if (line.length() == 0 || line.startsWith("#")) continue;
        if (line.startsWith("timestamp_ms")) continue;
        
        int idx1 = line.indexOf(',');
        int idx2 = line.indexOf(',', idx1 + 1);
        int idx3 = line.indexOf(',', idx2 + 1);
        int idx4 = line.indexOf(',', idx3 + 1);
        
        if (idx1 < 0 || idx2 < 0 || idx3 < 0 || idx4 < 0) continue;
        
        unsigned long ts = line.substring(0, idx1).toInt();
        float co2 = line.substring(idx1 + 1, idx2).toFloat();
        float temp = line.substring(idx2 + 1, idx3).toFloat();
        float hum = line.substring(idx3 + 1, idx4).toFloat();
        float light = line.substring(idx4 + 1).toFloat();
        
        stats.timestamps.push_back(ts);
        stats.co2_values.push_back(co2);
        stats.temp_values.push_back(temp);
        stats.hum_values.push_back(hum);
        stats.light_values.push_back(light);
    }
    file.close();
    
    if (stats.timestamps.empty()) {
        Serial.println("[Analysis] No hay datos en el archivo");
        return false;
    }
    
    if (stats.timestamps.size() > 1) {
        stats.duration = (stats.timestamps.back() - stats.timestamps.front()) / 1000;
    } else {
        stats.duration = 0;
    }
    
    stats.startTimestamp = stats.timestamps.front();
    
    return true;
}

// ============================================================================
// calculateStatistics() - Calcula medias, máximos y mínimos
// ============================================================================
void AnalysisTask::calculateStatistics(SessionStats &stats) {
    if (stats.co2_values.empty()) return;
    
    // CO2
    stats.co2_min = stats.co2_values[0];
    stats.co2_max = stats.co2_values[0];
    float co2_sum = 0;
    for (float v : stats.co2_values) {
        co2_sum += v;
        if (v < stats.co2_min) stats.co2_min = v;
        if (v > stats.co2_max) stats.co2_max = v;
    }
    stats.co2_avg = co2_sum / stats.co2_values.size();
    stats.co2_score = calculateCO2Score(stats.co2_avg);
    
    // Temperatura
    stats.temp_min = stats.temp_values[0];
    stats.temp_max = stats.temp_values[0];
    float temp_sum = 0;
    for (float v : stats.temp_values) {
        temp_sum += v;
        if (v < stats.temp_min) stats.temp_min = v;
        if (v > stats.temp_max) stats.temp_max = v;
    }
    stats.temp_avg = temp_sum / stats.temp_values.size();
    stats.temp_score = calculateTempScore(stats.temp_avg);
    
    // Humedad
    stats.hum_min = stats.hum_values[0];
    stats.hum_max = stats.hum_values[0];
    float hum_sum = 0;
    for (float v : stats.hum_values) {
        hum_sum += v;
        if (v < stats.hum_min) stats.hum_min = v;
        if (v > stats.hum_max) stats.hum_max = v;
    }
    stats.hum_avg = hum_sum / stats.hum_values.size();
    stats.hum_score = calculateHumidityScore(stats.hum_avg);
    
    // Luz
    stats.light_min = stats.light_values[0];
    stats.light_max = stats.light_values[0];
    float light_sum = 0;
    for (float v : stats.light_values) {
        light_sum += v;
        if (v < stats.light_min) stats.light_min = v;
        if (v > stats.light_max) stats.light_max = v;
    }
    stats.light_avg = light_sum / stats.light_values.size();
    stats.light_score = calculateLightScore(stats.light_avg);
}

// ============================================================================
// calculateCO2Score() - Calcula puntuación de CO2 (0-40)
// ============================================================================
int AnalysisTask::calculateCO2Score(float co2_avg) {
    if (co2_avg < 800) return 40;
    if (co2_avg <= 1000) return 32;
    if (co2_avg <= 1400) return 20;
    if (co2_avg <= 1800) return 10;
    return 0;
}

// ============================================================================
// calculateTempScore() - Calcula puntuación de temperatura (0-25)
// ============================================================================
int AnalysisTask::calculateTempScore(float temp_avg) {
    if (temp_avg >= 18.0 && temp_avg <= 22.0) return 25;
    if (temp_avg > 22.0 && temp_avg <= 24.0) return 18;
    if (temp_avg > 24.0 && temp_avg <= 26.0) return 10;
    return 0;
}

// ============================================================================
// calculateHumidityScore() - Calcula puntuación de humedad (0-20)
// ============================================================================
int AnalysisTask::calculateHumidityScore(float hum_avg) {
    if (hum_avg >= 40.0 && hum_avg <= 60.0) return 20;
    if ((hum_avg >= 30.0 && hum_avg < 40.0) || 
        (hum_avg > 60.0 && hum_avg <= 70.0)) return 12;
    return 0;
}

// ============================================================================
// calculateLightScore() - Calcula puntuación de iluminación (0-15)
// ============================================================================
int AnalysisTask::calculateLightScore(float light_avg) {
    if (light_avg < 5.0) return 15;
    if (light_avg <= 20.0) return 8;
    return 0;
}

// ============================================================================
// calculateSleepScore() - Calcula el Sleep Score total (0-100)
// ============================================================================
int AnalysisTask::calculateSleepScore(SessionStats &stats) {
    int score = stats.co2_score + stats.temp_score + stats.hum_score + stats.light_score;
    return score;
}

// ============================================================================
// getInterpretation() - Devuelve texto según la puntuación
// ============================================================================
String AnalysisTask::getInterpretation(int score) {
    if (score >= 85) return "Condiciones optimas";
    if (score >= 70) return "Buenas condiciones";
    if (score >= 50) return "Condiciones aceptables";
    if (score >= 30) return "Condiciones desfavorables";
    return "Condiciones criticas";
}

// ============================================================================
// findBestHour() - Encuentra la mejor franja horaria de 1 hora
// ============================================================================
void AnalysisTask::findBestHour(SessionStats &stats) {
    if (stats.timestamps.size() < 2) {
        stats.bestHourStart = 0;
        stats.bestHourEnd = 0;
        return;
    }
    
    int windowSize = 2;
    int bestIndex = 0;
    float bestScore = -1;
    
    for (int i = 0; i <= (int)stats.timestamps.size() - windowSize; i++) {
        float windowScore = 0;
        for (int j = 0; j < windowSize; j++) {
            float score = 0;
            float co2 = stats.co2_values[i + j];
            float temp = stats.temp_values[i + j];
            float hum = stats.hum_values[i + j];
            float light = stats.light_values[i + j];
            
            if (co2 > 900) score += (co2 - 900) / 100;
            if (temp < 18) score += (18 - temp) * 2;
            if (temp > 22) score += (temp - 22) * 2;
            if (hum < 40) score += (40 - hum);
            if (hum > 60) score += (hum - 60);
            if (light > 5) score += (light - 5);
            
            windowScore += score;
        }
        if (bestScore < 0 || windowScore < bestScore) {
            bestScore = windowScore;
            bestIndex = i;
        }
    }
    
    if (bestIndex >= 0 && bestIndex + windowSize - 1 < (int)stats.timestamps.size()) {
        stats.bestHourStart = stats.timestamps[bestIndex] / 1000;
        stats.bestHourEnd = stats.timestamps[bestIndex + windowSize - 1] / 1000;
        if (stats.bestHourEnd < stats.bestHourStart + 3600) {
            stats.bestHourEnd = stats.bestHourStart + 3600;
        }
    } else {
        stats.bestHourStart = 0;
        stats.bestHourEnd = 0;
    }
}

// ============================================================================
// saveStatisticsToSD() - Guarda las estadísticas en un archivo JSON
// ============================================================================
void AnalysisTask::saveStatisticsToSD(const SessionStats &stats) {
    char statsPath[64];
    snprintf(statsPath, sizeof(statsPath), "%s/session_%03lu_stats.json", SD_BASE_PATH, stats.sessionId);
    
    File statsFile = SD.open(statsPath, FILE_WRITE);
    if (!statsFile) {
        Serial.printf("[Analysis] Error al crear archivo de estadísticas: %s\n", statsPath);
        return;
    }
    
    statsFile.println("{");
    statsFile.printf("  \"sessionId\": %lu,\n", stats.sessionId);
    statsFile.printf("  \"duration\": %lu,\n", stats.duration);
    statsFile.printf("  \"sleepScore\": %d,\n", stats.sleepScore);
    statsFile.printf("  \"interpretation\": \"%s\",\n", stats.interpretation.c_str());
    
    statsFile.println("  \"co2\": {");
    statsFile.printf("    \"avg\": %.0f,\n", stats.co2_avg);
    statsFile.printf("    \"max\": %.0f,\n", stats.co2_max);
    statsFile.printf("    \"min\": %.0f,\n", stats.co2_min);
    statsFile.printf("    \"score\": %d\n", stats.co2_score);
    statsFile.println("  },");
    
    statsFile.println("  \"temperature\": {");
    statsFile.printf("    \"avg\": %.1f,\n", stats.temp_avg);
    statsFile.printf("    \"max\": %.1f,\n", stats.temp_max);
    statsFile.printf("    \"min\": %.1f,\n", stats.temp_min);
    statsFile.printf("    \"score\": %d\n", stats.temp_score);
    statsFile.println("  },");
    
    statsFile.println("  \"humidity\": {");
    statsFile.printf("    \"avg\": %.0f,\n", stats.hum_avg);
    statsFile.printf("    \"max\": %.0f,\n", stats.hum_max);
    statsFile.printf("    \"min\": %.0f,\n", stats.hum_min);
    statsFile.printf("    \"score\": %d\n", stats.hum_score);
    statsFile.println("  },");
    
    statsFile.println("  \"light\": {");
    statsFile.printf("    \"avg\": %.0f,\n", stats.light_avg);
    statsFile.printf("    \"max\": %.0f,\n", stats.light_max);
    statsFile.printf("    \"min\": %.0f,\n", stats.light_min);
    statsFile.printf("    \"score\": %d\n", stats.light_score);
    statsFile.println("  },");
    
    statsFile.println("  \"bestHour\": {");
    statsFile.printf("    \"start\": %lu,\n", stats.bestHourStart);
    statsFile.printf("    \"end\": %lu\n", stats.bestHourEnd);
    statsFile.println("  }");
    
    statsFile.println("}");
    
    statsFile.close();
    Serial.printf("[Analysis] Estadísticas guardadas: %s\n", statsPath);
}