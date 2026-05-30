#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <Arduino.h>                // Base de Arduino (incluye FreeRTOS)
#include <freertos/task.h>          // Para crear tareas FreeRTOS
#include <freertos/queue.h>         // Para usar colas

// Estructura que contendrá una lectura completa de sensores
struct SensorData {
    unsigned long timestamp;   // Momento de la lectura (millis())
    float co2;                 // ppm
    float temperature;         // °C
    float humidity;            // %
    float light;               // lux
};

class SensorTask {
public:
    // Inicia la tarea y le asigna una cola donde publicará los datos
    static void start(QueueHandle_t outputQueue);
    
    // Devuelve el manejador de la cola (para que otras tareas puedan leer)
    static QueueHandle_t getDataQueue();
    
private:
    static TaskHandle_t _taskHandle;     // Manejador de la tarea FreeRTOS
    static QueueHandle_t _sensorQueue;   // Cola donde se publican los datos
    
    static void taskFunction(void* pvParams);  // Función principal de la tarea
    static bool readSensors(SensorData &data); // Lee sensores y rellena estructura
};

#endif