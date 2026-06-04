#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <Arduino.h>
#include <freertos/task.h>
#include <freertos/queue.h>

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
    // Inicia la tarea: recibe TRES colas de datos y la cola de comandos de sesión
    static void start(QueueHandle_t queueForDisplay, QueueHandle_t queueForAlert, QueueHandle_t queueForStorage, QueueHandle_t cmdQueue);

    // Devuelve la cola de Display por compatibilidad
    static QueueHandle_t getDataQueue();

private:
    static TaskHandle_t  _taskHandle;        // Manejador de la tarea FreeRTOS
    static QueueHandle_t _queueForDisplay;   // Cola exclusiva para DisplayTask
    static QueueHandle_t _queueForAlert;     // Cola exclusiva para AlertTask
    static QueueHandle_t _queueForStorage;   // Cola exclusiva para StorageTask (NUEVA)
    static QueueHandle_t _cmdQueue;          // Cola de comandos para saber si la sesión está activa

    static void taskFunction(void* pvParams);  // Función principal de la tarea
    static bool readSensors(SensorData &data); // Lee sensores y rellena la estructura
};

#endif