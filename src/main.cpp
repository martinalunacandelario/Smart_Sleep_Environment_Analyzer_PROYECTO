#include <Arduino.h>
#include "tasks/SensorTask.h"
#include "tasks/DisplayTask.h"
#include "tasks/ButtonTask.h"
#include "tasks/AlertTask.h"
#include "tasks/StorageTask.h"

// ============================================================================
// COLAS DE DATOS DE SENSORES (una por consumidor)
// ============================================================================
QueueHandle_t sensorQueueForDisplay = nullptr;  // SensorTask → DisplayTask
QueueHandle_t sensorQueueForAlert   = nullptr;  // SensorTask → AlertTask
QueueHandle_t sensorQueueForStorage = nullptr;  // SensorTask → StorageTask

// ============================================================================
// COLAS DE COMANDOS (una por consumidor — nunca compartir entre tareas)
// ============================================================================
QueueHandle_t displayCommandQueue  = nullptr;  // ButtonTask → DisplayTask
QueueHandle_t sensorCommandQueue   = nullptr;  // ButtonTask → SensorTask
QueueHandle_t storageCommandQueue  = nullptr;  // ButtonTask → StorageTask
QueueHandle_t alertCommandQueue    = nullptr;  // ButtonTask → AlertTask  ← NUEVA

QueueHandle_t recommendationQueue  = nullptr;  // AlertTask → DisplayTask

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n=== Smart Sleep Environment Analyzer ===");

    // ========================================================================
    // COLAS DE DATOS
    // ========================================================================
    sensorQueueForDisplay = xQueueCreate(10, sizeof(SensorData));
    if (sensorQueueForDisplay == nullptr) {
        Serial.println("Error al crear sensorQueueForDisplay");
        while(1);
    }

    sensorQueueForAlert = xQueueCreate(10, sizeof(SensorData));
    if (sensorQueueForAlert == nullptr) {
        Serial.println("Error al crear sensorQueueForAlert");
        while(1);
    }

    sensorQueueForStorage = xQueueCreate(10, sizeof(SensorData));
    if (sensorQueueForStorage == nullptr) {
        Serial.println("Error al crear sensorQueueForStorage");
        while(1);
    }

    // ========================================================================
    // COLAS DE COMANDOS
    // ========================================================================
    displayCommandQueue = xQueueCreate(5, sizeof(DisplayCommand));
    if (displayCommandQueue == nullptr) {
        Serial.println("Error al crear displayCommandQueue");
        while(1);
    }

    sensorCommandQueue = xQueueCreate(5, sizeof(DisplayCommand));
    if (sensorCommandQueue == nullptr) {
        Serial.println("Error al crear sensorCommandQueue");
        while(1);
    }

    storageCommandQueue = xQueueCreate(5, sizeof(DisplayCommand));
    if (storageCommandQueue == nullptr) {
        Serial.println("Error al crear storageCommandQueue");
        while(1);
    }

    // Cola de comandos exclusiva para AlertTask
    alertCommandQueue = xQueueCreate(5, sizeof(DisplayCommand));
    if (alertCommandQueue == nullptr) {
        Serial.println("Error al crear alertCommandQueue");
        while(1);
    }

    recommendationQueue = xQueueCreate(5, sizeof(Recommendation));
    if (recommendationQueue == nullptr) {
        Serial.println("Error al crear recommendationQueue");
        while(1);
    }

    // ========================================================================
    // INICIO DE TAREAS
    // ========================================================================

    // SensorTask: publica en 3 colas de datos y escucha su cola de comandos
    SensorTask::start(sensorQueueForDisplay, sensorQueueForAlert,
                      sensorQueueForStorage, sensorCommandQueue);

    // DisplayTask: recibe datos, comandos de sesión y recomendaciones
    DisplayTask::start(sensorQueueForDisplay, displayCommandQueue, recommendationQueue);

    // ButtonTask: publica comandos en 4 colas separadas (Display, Sensor, Storage, Alert)
    ButtonTask::start(displayCommandQueue, sensorCommandQueue,
                      storageCommandQueue, alertCommandQueue);

    // AlertTask: controla LED RGB y buzzer, genera recomendaciones
    // Ahora recibe su propia cola de comandos de sesión
    AlertTask::start(sensorQueueForAlert, recommendationQueue,
                     StorageTask::getSessionCounterPtr(), alertCommandQueue);

    // StorageTask: guarda en SD y escucha su propia cola de comandos
    StorageTask::start(sensorQueueForStorage, storageCommandQueue);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}