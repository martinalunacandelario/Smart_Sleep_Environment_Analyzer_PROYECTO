#ifndef STORAGE_TASK_H
#define STORAGE_TASK_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "SensorTask.h"      // Para SensorData
#include "DisplayTask.h"     // Para DisplayCommand

class StorageTask {
public:
    // Inicia la tarea: recibe cola de sensores y cola de comandos de sesión
    static void start(QueueHandle_t sensorQueue, QueueHandle_t cmdQueue);

private:
    static TaskHandle_t _taskHandle;
    static QueueHandle_t _sensorQueue;    // Cola para recibir datos de sensores
    static QueueHandle_t _cmdQueue;       // Cola para recibir comandos de sesión
    
    static bool _sessionActive;           // Indica si hay sesión activa
    static File _currentSessionFile;      // Archivo de la sesión actual
    static String _currentFileName;       // Nombre del archivo actual
    static unsigned long _sessionStartTime; // Momento de inicio de sesión (ms)
    static unsigned long _sessionCounter;   // Contador de sesiones (se incrementa cada inicio)
    
    static void taskFunction(void* pvParams);
    
    // Funciones de gestión de archivos
    static bool initSD();                 // Inicializa la tarjeta SD
    static bool createSessionFile();      // Crea archivo CSV para la sesión
    static void closeSessionFile();       // Cierra el archivo al finalizar
    static void writeDataToSD(const SensorData &data); // Escribe una lectura en CSV
    
    // Funciones para gestionar el contador de sesiones
    static void readSessionCounter();     // Lee el contador desde la SD
    static void saveSessionCounter();     // Guarda el contador en la SD
    
    // Genera nombre de archivo: /sessions/session_XXX_YYYYMMDD_HHMMSS.csv
    static String generateFileName();
    
    // Obtiene fecha y hora actual (formato YYYYMMDD_HHMMSS)
    static String getCurrentDateTime();
};

#endif