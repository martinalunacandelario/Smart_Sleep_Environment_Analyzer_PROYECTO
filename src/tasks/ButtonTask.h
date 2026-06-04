#ifndef BUTTON_TASK_H
#define BUTTON_TASK_H

#include <Arduino.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "DisplayTask.h"  // Para tener DisplayCommand

class ButtonTask {
public:
    // Recibe tres colas: una para Display, una para Sensor y una para Storage
    static void start(QueueHandle_t cmdQueueDisplay,
                      QueueHandle_t cmdQueueSensor,
                      QueueHandle_t cmdQueueStorage);

private:
    static TaskHandle_t  _taskHandle;         // Manejador de la tarea FreeRTOS
    static QueueHandle_t _cmdQueueDisplay;    // Cola de comandos para DisplayTask
    static QueueHandle_t _cmdQueueSensor;     // Cola de comandos para SensorTask
    static QueueHandle_t _cmdQueueStorage;    // Cola de comandos para StorageTask

    static void taskFunction(void* pvParams); // Bucle principal
};

#endif