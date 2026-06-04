#ifndef BUTTON_TASK_H
#define BUTTON_TASK_H

#include <Arduino.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "DisplayTask.h"  // Para tener DisplayCommand

class ButtonTask {
public:
    // Recibe cuatro colas: Display, Sensor, Storage y Alert
    static void start(QueueHandle_t cmdQueueDisplay,
                      QueueHandle_t cmdQueueSensor,
                      QueueHandle_t cmdQueueStorage,
                      QueueHandle_t cmdQueueAlert);

private:
    static TaskHandle_t  _taskHandle;          // Manejador de la tarea FreeRTOS
    static QueueHandle_t _cmdQueueDisplay;     // Cola de comandos para DisplayTask
    static QueueHandle_t _cmdQueueSensor;      // Cola de comandos para SensorTask
    static QueueHandle_t _cmdQueueStorage;     // Cola de comandos para StorageTask
    static QueueHandle_t _cmdQueueAlert;       // Cola de comandos para AlertTask

    static void taskFunction(void* pvParams);  // Bucle principal
};

#endif