#ifndef ANALYSIS_TASK_H
#define ANALYSIS_TASK_H

#include <Arduino.h>                     // Funciones básicas de Arduino (Serial, millis, etc.)
#include <SPI.h>                         // Bus SPI (para leer la tarjeta SD)
#include <SD.h>                          // Librería para manejar la tarjeta SD
#include <freertos/task.h>               // Tareas FreeRTOS
#include <freertos/queue.h>              // Colas FreeRTOS
#include <vector>                        // Vector de C++ para almacenar listas de datos
#include "DisplayTask.h"                 // Para DisplayCommand (comandos de sesión)

// Estructura que almacena todas las estadísticas de una sesión
struct SessionStats {
    unsigned long sessionId;             // Número de sesión (1, 2, 3...)
    unsigned long startTimestamp;        // Timestamp de inicio (ms desde inicio del ESP32)
    unsigned long duration;              // Duración de la sesión en segundos
    
    // --- CO2 ---
    float co2_avg, co2_max, co2_min;     // Media, máximo y mínimo de CO2 (ppm)
    int co2_score;                       // Puntuación parcial (0-40 puntos)
    
    // --- Temperatura ---
    float temp_avg, temp_max, temp_min;  // Media, máximo y mínimo de temperatura (°C)
    int temp_score;                      // Puntuación parcial (0-25 puntos)
    
    // --- Humedad ---
    float hum_avg, hum_max, hum_min;     // Media, máximo y mínimo de humedad (%)
    int hum_score;                       // Puntuación parcial (0-20 puntos)
    
    // --- Luz ---
    float light_avg, light_max, light_min; // Media, máximo y mínimo de luz (lux)
    int light_score;                     // Puntuación parcial (0-15 puntos)
    
    // --- Sleep Score ---
    int sleepScore;                      // Puntuación total (0-100 puntos)
    String interpretation;               // Texto interpretativo (ej: "Condiciones optimas")
    
    // --- Mejor franja horaria ---
    unsigned long bestHourStart;         // Inicio de la mejor hora (segundos desde inicio)
    unsigned long bestHourEnd;           // Fin de la mejor hora (segundos desde inicio)
    
    // --- Datos para el timeline (vectores) - se usan durante el cálculo y luego se descartan ---
    std::vector<unsigned long> timestamps;  // Lista de timestamps (ms)
    std::vector<float> co2_values;          // Lista de valores de CO2
    std::vector<float> temp_values;         // Lista de valores de temperatura
    std::vector<float> hum_values;          // Lista de valores de humedad
    std::vector<float> light_values;        // Lista de valores de luz
};

class AnalysisTask {
public:
    // Inicia la tarea: recibe cola de comandos y puntero al contador de sesiones
    static void start(QueueHandle_t cmdQueue, unsigned long* sessionCounter);

private:
    static TaskHandle_t _taskHandle;          // Manejador de la tarea FreeRTOS
    static QueueHandle_t _cmdQueue;           // Cola para recibir comandos de sesión
    static unsigned long* _sessionCounter;    // Puntero al contador de sesiones (lo comparte con StorageTask)
    
    static void taskFunction(void* pvParams); // Función principal de la tarea
    
    // --- Funciones de lectura y análisis ---
    static bool readSessionFile(const String& filename, SessionStats &stats); // Lee el CSV
    static void calculateStatistics(SessionStats &stats);                     // Calcula medias, máx, min
    static void findBestHour(SessionStats &stats);                            // Encuentra mejor franja horaria
    static void saveStatisticsToSD(const SessionStats &stats);                // Guarda JSON con resultados
    
    // --- Funciones de cálculo del Sleep Score (según la tabla del proyecto) ---
    static int calculateCO2Score(float co2_avg);      // Puntuación CO2 (0-40)
    static int calculateTempScore(float temp_avg);    // Puntuación temperatura (0-25)
    static int calculateHumidityScore(float hum_avg); // Puntuación humedad (0-20)
    static int calculateLightScore(float light_avg);  // Puntuación luz (0-15)
    static int calculateSleepScore(SessionStats &stats); // Suma total (0-100)
    static String getInterpretation(int score);       // Texto según puntuación
    
    // Función auxiliar para obtener el nombre del archivo de la última sesión
    static String getLastSessionFileName();
};

#endif // ANALYSIS_TASK_H