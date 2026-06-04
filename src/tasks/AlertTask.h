#ifndef ALERT_TASK_H
#define ALERT_TASK_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "SensorTask.h"

// Estructura para enviar recomendaciones a DisplayTask
struct Recommendation {
    char message[64];        // Texto de la recomendación
    unsigned long duration;  // Duración en ms (0 = usar valor por defecto)
};

// Estructura de comando de sesión (igual que DisplayCommand)
struct AlertCommand {
    bool sessionActive;      // true = sesión iniciada, false = finalizada
};

class AlertTask {
public:
    // Recibe cola de sensores, cola de recomendaciones, puntero al contador
    // y cola de comandos de sesión propia
    static void start(QueueHandle_t sensorQueue,
                      QueueHandle_t recQueue,
                      unsigned long* sessionCounter,
                      QueueHandle_t cmdQueue);

private:
    static TaskHandle_t   _taskHandle;          // Manejador de la tarea FreeRTOS
    static QueueHandle_t  _sensorQueue;         // Cola de datos de sensores
    static QueueHandle_t  _recQueue;            // Cola para enviar recomendaciones a DisplayTask
    static QueueHandle_t  _cmdQueue;            // Cola para recibir comandos de sesión
    static unsigned long* _sessionCounter;      // Puntero al contador de sesiones

    static bool          _sessionActive;        // Indica si hay sesión activa
    static unsigned long _sessionStartTime;     // Momento de inicio de sesión (ms)

    // Archivo de alertas en SD
    static File _alertsFile;
    static bool _alertsFileOpen;

    static void taskFunction(void* pvParams);

    // Gestión del archivo JSON de alertas
    static void createAlertsFile();
    static void closeAlertsFile();
    static void saveAlertToSD(const char* type, const char* message);
    static String getCurrentTimeString();

    // Evaluación de variables
    static int getCo2State(float co2);
    static int getTempState(float temp);
    static int getHumState(float hum);
    static int getLightState(float light);
    static int getGlobalState(SensorData &data);

    // Control de hardware
    static void setLedState(int state);
    static void playAlarm();

    // Generación de recomendaciones
    static const char* getRecommendation(SensorData &data);
};

#endif