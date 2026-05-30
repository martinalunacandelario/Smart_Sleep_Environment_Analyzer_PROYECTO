// ============================================================================
// main.cpp - Integración de SensorTask y AlertTask
// ============================================================================

#include <Arduino.h>
#include "SensorTask.h"
#include "AlertTask.h"
#include "DataStructures.h"
#include "Config.h"

// Colas para comunicación entre tareas
QueueHandle_t sensorDataQueue;  // SensorTask -> AlertTask
QueueHandle_t alertQueue;       // AlertTask -> (DisplayTask después)

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n==========================================");
    Serial.println("SMART SLEEP ENVIRONMENT ANALYZER");
    Serial.println("SensorTask + AlertTask");
    Serial.println("==========================================\n");
    
    // 1. Crear las colas
    sensorDataQueue = xQueueCreate(10, sizeof(SensorData));
    alertQueue = xQueueCreate(20, sizeof(DisplayAlert));
    
    if (!sensorDataQueue || !alertQueue) {
        Serial.println("ERROR: No se pudieron crear las colas");
        while(1) { delay(100); }
    }
    
    // 2. Iniciar SensorTask (envía datos a sensorDataQueue)
    SensorTask::start(sensorDataQueue);
    
    // 3. Iniciar AlertTask (recibe de sensorDataQueue, envía a alertQueue)
    AlertTask::start(sensorDataQueue, alertQueue);
    
    // 4. Activar sesión para que se generen alertas
    AlertTask::setSessionActive(true);
    
    Serial.println("\n=== TAREAS INICIADAS ===");
    Serial.println("  SensorTask   - Core 0 - Prioridad 4 - Lee sensores cada 5s");
    Serial.println("  AlertTask    - Core 0 - Prioridad 3 - LED y alertas cada 2s");
    Serial.println("==========================================\n");
}

void loop() {
    // Recibir y mostrar alertas (solo para debug)
    DisplayAlert alert;
    if (xQueueReceive(alertQueue, &alert, 0) == pdTRUE) {
        Serial.printf("[MAIN] Alerta recibida: %s - %s\n", alert.type, alert.message);
    }
    
    delay(100);
}