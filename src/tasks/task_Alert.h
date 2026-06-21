#ifndef TASK_ALERT_H
#define TASK_ALERT_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "task_Sensor.h"          // Para SensorData

// ============================================================================
// ESTRUCTURAS DE DATOS
// ============================================================================

// Estructura para enviar recomendaciones a DisplayTask
struct Recommendation {
    char message[64];            // Texto de la recomendación (ej: "Ventilar la habitacion")
    unsigned long duration;      // Duración en ms (0 = usar valor por defecto)
};

// Estructura de comando de sesión (recibida desde ButtonTask o WebServerTask)
struct AlertCommand {
    bool sessionActive;          // true = sesión iniciada, false = finalizada
};

// ============================================================================
// CLASE ALERT TASK
// ============================================================================

class AlertTask {
public:
    // Inicia la tarea de alertas
    // @param sensorQueue    Cola de datos de sensores (desde SensorTask)
    // @param recQueue       Cola para enviar recomendaciones (hacia DisplayTask)
    // @param sessionCounter Puntero al contador de sesiones (desde StorageTask)
    // @param cmdQueue       Cola para recibir comandos de sesión (desde ButtonTask/WebServerTask)
    static void start(QueueHandle_t sensorQueue,
                      QueueHandle_t recQueue,
                      unsigned long* sessionCounter,
                      QueueHandle_t cmdQueue);

    // ==========================================================================
    // MÉTODOS PÚBLICOS PARA EVALUACIÓN (accesibles desde tests)
    // ==========================================================================
    
    // --- Evaluación de variables ambientales ---
    static int getCo2State(float co2);          // 0=bueno, 1=regular, 2=malo
    static int getTempState(float temp);        // 0=bueno, 1=regular, 2=malo
    static int getHumState(float hum);          // 0=bueno, 1=regular, 2=malo
    static int getLightState(float light);      // 0=bueno, 1=regular, 2=malo
    static int getGlobalState(SensorData &data); // Devuelve el peor estado (0,1,2)

    // --- Generación de recomendaciones ---
    static const char* getRecommendation(SensorData &data); // Mensaje según variable crítica

private:
    // ==========================================================================
    // MIEMBROS ESTÁTICOS
    // ==========================================================================
    
    // Manejadores y colas
    static TaskHandle_t   _taskHandle;          // Manejador de la tarea FreeRTOS
    static QueueHandle_t  _sensorQueue;         // Cola de datos de sensores (entrada)
    static QueueHandle_t  _recQueue;            // Cola para enviar recomendaciones (salida)
    static QueueHandle_t  _cmdQueue;            // Cola para recibir comandos de sesión (entrada)
    static unsigned long* _sessionCounter;      // Puntero al contador de sesiones (compartido con StorageTask)

    // Estado de sesión
    static bool          _sessionActive;        // true = sesión activa, false = inactiva
    static unsigned long _sessionStartTime;     // Momento de inicio de sesión (millis)

    // Archivo JSON de alertas en la microSD
    static File  _alertsFile;                   // Archivo abierto
    static bool  _alertsFileOpen;               // true si el archivo está abierto

    // ==========================================================================
    // MÉTODOS PRIVADOS
    // ==========================================================================
    
    // Función principal de la tarea (bucle FreeRTOS)
    static void taskFunction(void* pvParams);

    // --- Gestión del archivo JSON de alertas ---
    static void createAlertsFile();             // Crea el archivo al iniciar sesión
    static void closeAlertsFile();              // Cierra el archivo al finalizar sesión
    static void saveAlertToSD(const char* type, const char* message); // Guarda una alerta
    static String getCurrentTimeString();       // Devuelve hora actual desde inicio de sesión (HH:MM)

    // --- Control de hardware ---
    static void setLedState(int state);         // 0=Verde, 1=Amarillo, 2=Rojo
    static void playAlarm();                   // Reproduce la melodía de alarma
};

#endif // TASK_ALERT_H