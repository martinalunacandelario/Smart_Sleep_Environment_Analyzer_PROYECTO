#include <Arduino.h>
#include "tasks/SensorTask.h"
#include "tasks/DisplayTask.h"
#include "tasks/ButtonTask.h"
#include "tasks/AlertTask.h"
#include "tasks/StorageTask.h"
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
// SessionManager es la única fuente que escribe en estas colas.
// Tanto el botón físico como la web usan SessionManager, nunca las colas directamente.
// ============================================================================
QueueHandle_t sessionQueueDisplay  = nullptr;  // SessionManager → DisplayTask
QueueHandle_t sessionQueueSensor   = nullptr;  // SessionManager → SensorTask
QueueHandle_t sessionQueueStorage  = nullptr;  // SessionManager → StorageTask
QueueHandle_t sessionQueueAlert    = nullptr;  // SessionManager → AlertTask

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

    recommendationQueue = xQueueCreate(5, sizeof(Recommendation));
    if (recommendationQueue == nullptr) {
        Serial.println("Error al crear recommendationQueue"); while(1);
    }

    // ========================================================================
    // SUSCRIBIR COLAS AL SESSION MANAGER
    // Al llamar startSession() o stopSession() desde cualquier sitio,
    // SessionManager notifica automáticamente a todas estas colas
    // ========================================================================
    SessionManager::subscribe(sessionQueueDisplay);
    SessionManager::subscribe(sessionQueueSensor);
    SessionManager::subscribe(sessionQueueStorage);
    SessionManager::subscribe(sessionQueueAlert);

    // ========================================================================
    // INICIO DE TAREAS
    // ========================================================================

    // SensorTask: publica datos en 3 colas, escucha su cola de sesión
    SensorTask::start(sensorQueueForDisplay, sensorQueueForAlert,
                      sensorQueueForStorage, sessionQueueSensor);

    // DisplayTask: recibe datos de sensores, comandos de sesión y recomendaciones
    DisplayTask::start(sensorQueueForDisplay, sessionQueueDisplay, recommendationQueue);

    // ButtonTask: ya no gestiona colas directamente, delega en SessionManager
    ButtonTask::start();

    // AlertTask: controla LED RGB y buzzer, escucha su cola de sesión
    AlertTask::start(sensorQueueForAlert, recommendationQueue,
                     StorageTask::getSessionCounterPtr(), sessionQueueAlert);

    // StorageTask: guarda en SD, escucha su cola de sesión
    StorageTask::start(sensorQueueForStorage, sessionQueueStorage);

    // WebServerTask: ya no necesita cola, usa SessionManager directamente
    WebServerTask::start();
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}