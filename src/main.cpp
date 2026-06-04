#include <Arduino.h>
#include "tasks/SensorTask.h"
#include "tasks/DisplayTask.h"
#include "tasks/ButtonTask.h"
#include "tasks/AlertTask.h"
#include "tasks/StorageTask.h"

// Colas de datos de sensores (una por consumidor)
QueueHandle_t sensorQueueForDisplay = nullptr;  // SensorTask → DisplayTask
QueueHandle_t sensorQueueForAlert   = nullptr;  // SensorTask → AlertTask
QueueHandle_t sensorQueueForStorage = nullptr;  // SensorTask → StorageTask

// Colas de comandos: ButtonTask publica en ellas
QueueHandle_t displayCommandQueue   = nullptr;  // ButtonTask → DisplayTask
QueueHandle_t sensorCommandQueue    = nullptr;  // ButtonTask → SensorTask (NUEVA)
QueueHandle_t recommendationQueue   = nullptr;  // AlertTask → DisplayTask

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n=== Smart Sleep Environment Analyzer ===");

    // Crear colas de datos (una por consumidor)
    sensorQueueForDisplay = xQueueCreate(10, sizeof(SensorData));
    sensorQueueForAlert   = xQueueCreate(10, sizeof(SensorData));
    sensorQueueForStorage = xQueueCreate(10, sizeof(SensorData));

    // Cola de comandos para DisplayTask
    displayCommandQueue = xQueueCreate(5, sizeof(DisplayCommand));

    // Cola de comandos para SensorTask (para que sepa cuándo iniciar/finalizar sesión)
    sensorCommandQueue = xQueueCreate(5, sizeof(DisplayCommand));

    // Cola de recomendaciones
    recommendationQueue = xQueueCreate(5, sizeof(Recommendation));

    // Iniciar tareas
    SensorTask::start(sensorQueueForDisplay, sensorQueueForAlert, sensorQueueForStorage, sensorCommandQueue);
    DisplayTask::start(sensorQueueForDisplay, displayCommandQueue, recommendationQueue);
    ButtonTask::start(displayCommandQueue, sensorCommandQueue);  // AHORA CON 2 PARÁMETROS
    AlertTask::start(sensorQueueForAlert, recommendationQueue);
    StorageTask::start(sensorQueueForStorage, displayCommandQueue);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}