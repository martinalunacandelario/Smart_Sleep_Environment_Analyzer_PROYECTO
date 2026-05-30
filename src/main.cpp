#include <Arduino.h>                     // Incluye la librería estándar de Arduino (Serial, delay, etc.)
#include "tasks/SensorTask.h"           // Incluye la cabecera de la tarea de sensores (SensorData, SensorTask)

QueueHandle_t sensorDataQueue = nullptr; // Cola de FreeRTOS para pasar datos de sensores (inicialmente nula)

void setup() {                          // Función de configuración (se ejecuta una vez al inicio)
    Serial.begin(115200);               // Inicia la comunicación serie a 115200 baudios
    delay(2000);                        // Espera 2 segundos para que el monitor serie se estabilice
    Serial.println("\n=== Smart Sleep Environment Analyzer ==="); // Imprime mensaje de inicio

    // Crea una cola con capacidad para 10 elementos de tipo SensorData
    sensorDataQueue = xQueueCreate(10, sizeof(SensorData));
    if (sensorDataQueue == nullptr) {   // Si no se pudo crear la cola (error)
        Serial.println("Error al crear la cola de sensores"); // Mensaje de error
        while(1);                       // Bucle infinito (detiene la ejecución)
    }

    SensorTask::start(sensorDataQueue); // Inicia la tarea de sensores, pasándole la cola
}   // Fin de setup

void loop() {                           // Función principal (se ejecuta repetidamente)
    vTaskDelay(pdMS_TO_TICKS(1000));   // Retrasa esta tarea (el loop principal) durante 1 segundo (libera CPU)
}   // Fin de loop