#ifndef TASK_ANALYSIS_H
#define TASK_ANALYSIS_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <vector>
#include "task_Display.h"

// ============================================================================
// Mínimo de muestras necesarias para calcular una "mejor franja" fiable.
// Con SENSOR_INTERVAL_MS = 30000 (30s/muestra): 120 muestras = 1 hora real.
// Si cambias el intervalo de muestreo en config.h, recalcula este valor:
//   MIN_SAMPLES_FOR_BESTHOUR = 3600000 / SENSOR_INTERVAL_MS
// ============================================================================
#define MIN_SAMPLES_FOR_BESTHOUR 120

struct SessionStats {
    unsigned long sessionId;
    unsigned long startTimestamp;
    unsigned long duration;
    
    // ================================================================
    // FECHA Y HORA (necesarias para NTP)
    // ================================================================
    String date;                    // "2024-01-15"
    String startTime;               // "14:30:00"
    String endTime;                 // "15:45:00"
    unsigned long sessionStartEpoch; // Segundos desde 1970
    
    float co2_avg, co2_max, co2_min;
    int co2_score;
    
    float temp_avg, temp_max, temp_min;
    int temp_score;
    
    float hum_avg, hum_max, hum_min;
    int hum_score;
    
    float light_avg, light_max, light_min;
    int light_score;
    
    int sleepScore;
    String interpretation;
    
    unsigned long bestHourStart;
    unsigned long bestHourEnd;
    bool bestHourValid;   // true si hay datos suficientes (>= MIN_SAMPLES_FOR_BESTHOUR)
    
    std::vector<unsigned long> timestamps;
    std::vector<float> co2_values;
    std::vector<float> temp_values;
    std::vector<float> hum_values;
    std::vector<float> light_values;
};

class AnalysisTask {
public:
    // ================================================================
    // FUNCIONES PÚBLICAS (disponibles para tests y uso externo)
    // ================================================================
    
    // Inicia la tarea
    static void start(QueueHandle_t cmdQueue, unsigned long* sessionCounter);
    
    // Lectura y análisis
    static bool readSessionFile(const String& filename, SessionStats &stats);
    static void calculateStatistics(SessionStats &stats);
    static void findBestHour(SessionStats &stats);
    static void saveStatisticsToSD(const SessionStats &stats);
    static String getLastSessionFileName();
    
    // Puntuaciones
    static int calculateCO2Score(float co2_avg);
    static int calculateTempScore(float temp_avg);
    static int calculateHumidityScore(float hum_avg);
    static int calculateLightScore(float light_avg);
    static int calculateSleepScore(SessionStats &stats);
    static String getInterpretation(int score);

private:
    static TaskHandle_t _taskHandle;
    static QueueHandle_t _cmdQueue;
    static unsigned long* _sessionCounter;
    
    static void taskFunction(void* pvParams);
};

#endif // TASK_ANALYSIS_H