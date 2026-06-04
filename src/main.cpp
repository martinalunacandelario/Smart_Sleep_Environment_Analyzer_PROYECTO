#include <Arduino.h>
#include "tasks/SensorTask.h"
#include "tasks/DisplayTask.h"
#include "tasks/ButtonTask.h"
#include "tasks/AlertTask.h"
#include "tasks/StorageTask.h"
#include "tasks/AnalysisTask.h"
#include "tasks/WebServerTask.h"
#include "SessionManager.h"

// ============================================================================
// COLAS DE DATOS DE SENSORES (una por consumidor)
// ============================================================================
QueueHandle_t sensorQueueForDisplay = nullptr;  // SensorTask → DisplayTask
QueueHandle_t sensorQueueForAlert   = nullptr;  // SensorTask → AlertTask
QueueHandle_t sensorQueueForStorage = nullptr;  // SensorTask → StorageTask

// ============================================================================
// COLAS DE COMANDOS DE SESIÓN (una por consumidor, gestionadas por SessionManager)
// ============================================================================
QueueHandle_t sessionQueueDisplay  = nullptr;  // SessionManager → DisplayTask
QueueHandle_t sessionQueueSensor   = nullptr;  // SessionManager → SensorTask
QueueHandle_t sessionQueueStorage  = nullptr;  // SessionManager → StorageTask
QueueHandle_t sessionQueueAlert    = nullptr;  // SessionManager → AlertTask
QueueHandle_t sessionQueueAnalysis = nullptr;  // SessionManager → AnalysisTask (NUEVA)

// ============================================================================
// COLA DE RECOMENDACIONES
// ============================================================================
QueueHandle_t recommendationQueue  = nullptr;  // AlertTask → DisplayTask

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n=== Smart Sleep Environment Analyzer ===");

    // Inicializar SessionManager PRIMERO (antes de crear cualquier tarea o cola)
    SessionManager::init();

    // ========================================================================
    // COLAS DE DATOS
    // ========================================================================
    sensorQueueForDisplay = xQueueCreate(10, sizeof(SensorData));
    if (sensorQueueForDisplay == nullptr) {
        Serial.println("Error al crear sensorQueueForDisplay"); while(1);
    }

    sensorQueueForAlert = xQueueCreate(10, sizeof(SensorData));
    if (sensorQueueForAlert == nullptr) {
        Serial.println("Error al crear sensorQueueForAlert"); while(1);
    }

    sensorQueueForStorage = xQueueCreate(10, sizeof(SensorData));
    if (sensorQueueForStorage == nullptr) {
        Serial.println("Error al crear sensorQueueForStorage"); while(1);
    }

    // ========================================================================
    // COLAS DE COMANDOS DE SESIÓN
    // ========================================================================
    sessionQueueDisplay = xQueueCreate(5, sizeof(SessionCommand));
    if (sessionQueueDisplay == nullptr) {
        Serial.println("Error al crear sessionQueueDisplay"); while(1);
    }

    sessionQueueSensor = xQueueCreate(5, sizeof(SessionCommand));
    if (sessionQueueSensor == nullptr) {
        Serial.println("Error al crear sessionQueueSensor"); while(1);
    }

    sessionQueueStorage = xQueueCreate(5, sizeof(SessionCommand));
    if (sessionQueueStorage == nullptr) {
        Serial.println("Error al crear sessionQueueStorage"); while(1);
    }

    sessionQueueAlert = xQueueCreate(5, sizeof(SessionCommand));
    if (sessionQueueAlert == nullptr) {
        Serial.println("Error al crear sessionQueueAlert"); while(1);
    }

    // NUEVA: Cola exclusiva para AnalysisTask
    sessionQueueAnalysis = xQueueCreate(5, sizeof(SessionCommand));
    if (sessionQueueAnalysis == nullptr) {
        Serial.println("Error al crear sessionQueueAnalysis"); while(1);
    }

    recommendationQueue = xQueueCreate(5, sizeof(Recommendation));
    if (recommendationQueue == nullptr) {
        Serial.println("Error al crear recommendationQueue"); while(1);
    }

    // ========================================================================
    // SUSCRIBIR COLAS AL SESSION MANAGER
    // SessionManager notifica a todas estas colas cuando cambia la sesión
    // ========================================================================
    SessionManager::subscribe(sessionQueueDisplay);
    SessionManager::subscribe(sessionQueueSensor);
    SessionManager::subscribe(sessionQueueStorage);
    SessionManager::subscribe(sessionQueueAlert);
    SessionManager::subscribe(sessionQueueAnalysis);  // NUEVA

    // ========================================================================
    // INICIO DE TAREAS
    // ========================================================================

    // SensorTask: publica datos en 3 colas, escucha su cola de sesión
    SensorTask::start(sensorQueueForDisplay, sensorQueueForAlert,
                      sensorQueueForStorage, sessionQueueSensor);

    // DisplayTask: recibe datos de sensores, comandos de sesión y recomendaciones
    DisplayTask::start(sensorQueueForDisplay, sessionQueueDisplay, recommendationQueue);

    // ButtonTask: delega en SessionManager
    ButtonTask::start();

    // AlertTask: controla LED RGB y buzzer, escucha su cola de sesión
    AlertTask::start(sensorQueueForAlert, recommendationQueue,
                     StorageTask::getSessionCounterPtr(), sessionQueueAlert);

    // StorageTask: guarda en SD, escucha su cola de sesión
    StorageTask::start(sensorQueueForStorage, sessionQueueStorage);

    // AnalysisTask: genera estadísticas JSON al finalizar sesión
    AnalysisTask::start(sessionQueueAnalysis, StorageTask::getSessionCounterPtr());

    // WebServerTask: servidor web
    WebServerTask::start();
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}