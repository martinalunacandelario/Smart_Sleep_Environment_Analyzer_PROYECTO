#ifndef TASK_STORAGE_H
#define TASK_STORAGE_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "task_Sensor.h"      // Para SensorData (estructura con CO2, temp, hum, luz)
#include "task_Display.h"     // Para DisplayCommand (comandos de inicio/fin sesión)

class StorageTask {
public:
    // Inicia la tarea: recibe cola de sensores y cola de comandos de sesión
    static void start(QueueHandle_t sensorQueue, QueueHandle_t cmdQueue);

    // Devuelve puntero al contador de sesiones para que otras tareas
    static unsigned long* getSessionCounterPtr();

private:
    static TaskHandle_t _taskHandle;          // Manejador de la tarea FreeRTOS
    static QueueHandle_t _sensorQueue;        // Cola para recibir datos de sensores
    static QueueHandle_t _cmdQueue;           // Cola para recibir comandos de sesión
    
    static bool _sessionActive;               // Indica si hay sesión activa (true/false)
    static File _currentSessionFile;          // Archivo CSV de la sesión actual
    static String _currentFileName;           // Nombre del archivo actual
    static unsigned long _sessionStartTime;   // Momento de inicio de sesión (millis)
    static unsigned long _sessionCounter;     // Contador de sesiones (1, 2, 3...)
    
    static void taskFunction(void* pvParams); // Función principal de la tarea
    
    // Funciones de gestión de archivos
    static bool initSD();                     // Inicializa la tarjeta SD
    static bool createSessionFile();          // Crea archivo CSV para la sesión
    static void closeSessionFile();           // Cierra el archivo al finalizar
    static void writeDataToSD(const SensorData &data); // Escribe una lectura en CSV
    
    // Funciones para gestionar el contador de sesiones
    static void readSessionCounter();         // Lee el contador desde la SD (counter.txt)
    static void saveSessionCounter();         // Guarda el contador en la SD (counter.txt)
    
    // Genera nombre de archivo: /sessions/session_001_1701700000000.csv
    static String generateFileName();
    
    // Obtiene fecha y hora actual (formato YYYYMMDD_HHMMSS)
    static String getCurrentDateTime();
};

#endif // TASK_STORAGE_H