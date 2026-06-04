#ifndef ALERT_TASK_H
#define ALERT_TASK_H

#include <Arduino.h>
#include <SPI.h>                     // Para comunicarse con la tarjeta SD
#include <SD.h>                      // Librería para manejar la tarjeta SD
#include <freertos/task.h>           // Tareas FreeRTOS
#include <freertos/queue.h>          // Colas FreeRTOS
#include "SensorTask.h"              // Para SensorData

// Estructura para enviar recomendaciones a DisplayTask
struct Recommendation {
    char message[64];        // Texto de la recomendación
    unsigned long duration;  // Duración en ms (0 = usar valor por defecto)
};

class AlertTask {
public:
    // Inicia la tarea: recibe cola de sensores, cola de recomendaciones y puntero al contador de sesiones
    static void start(QueueHandle_t sensorQueue, QueueHandle_t recQueue, unsigned long* sessionCounter);

private:
    static TaskHandle_t _taskHandle;           // Manejador de la tarea FreeRTOS
    static QueueHandle_t _sensorQueue;         // Cola para recibir datos de sensores
    static QueueHandle_t _recQueue;            // Cola para enviar recomendaciones a DisplayTask
    static unsigned long* _sessionCounter;     // Puntero al contador de sesiones (desde StorageTask)
    
    static bool _sessionActive;                // Indica si hay sesión activa
    static unsigned long _sessionStartTime;    // Momento de inicio de sesión (ms)
    
    // Archivo de alertas
    static File _alertsFile;                   // Archivo JSON para guardar alertas
    static bool _alertsFileOpen;               // Indica si el archivo está abierto
    
    static void taskFunction(void* pvParams);  // Función principal de la tarea
    
    // Funciones para guardar alertas en JSON
    static void createAlertsFile();            // Crea el archivo JSON al iniciar sesión
    static void closeAlertsFile();             // Cierra el archivo JSON al finalizar sesión
    static void saveAlertToSD(const char* type, const char* message); // Guarda una alerta
    static String getCurrentTimeString();      // Devuelve hora actual en formato HH:MM (desde inicio sesión)
    
    // Evaluar estado de cada variable
    static int getCo2State(float co2);
    static int getTempState(float temp);
    static int getHumState(float hum);
    static int getLightState(float light);
    
    // Obtener estado global (0=Verde, 1=Amarillo, 2=Rojo)
    static int getGlobalState(SensorData &data);
    
    // Controlar LED RGB según estado
    static void setLedState(int state);
    
    // Activar buzzer con melodía de alarma
    static void playAlarm();
    
    // Generar recomendación según la variable crítica
    static const char* getRecommendation(SensorData &data);
};

#endif