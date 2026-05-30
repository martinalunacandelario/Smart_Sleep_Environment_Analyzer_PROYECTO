#include <Arduino.h>
#include "tasks/SensorTask.h"
#include "tasks/DisplayTask.h"
#include "tasks/ButtonTask.h"          

QueueHandle_t sensorDataQueue = nullptr;
QueueHandle_t displayCommandQueue = nullptr;

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n=== Smart Sleep Environment Analyzer ===");

    // Cola para datos de sensores (10 elementos)
    sensorDataQueue = xQueueCreate(10, sizeof(SensorData));
    if (sensorDataQueue == nullptr) {
        Serial.println("Error al crear la cola de sensores");
        while(1);
    }

    // Cola para comandos de display (inicio/fin sesión)
    displayCommandQueue = xQueueCreate(5, sizeof(DisplayCommand));
    if (displayCommandQueue == nullptr) {
        Serial.println("Error al crear la cola de comandos de display");
        while(1);
    }

    // Iniciar tareas
    SensorTask::start(sensorDataQueue);
    DisplayTask::start(sensorDataQueue, displayCommandQueue);
    ButtonTask::start(displayCommandQueue);   // Iniciar la tarea del botón (le pasa la misma cola)
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}