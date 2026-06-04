#ifndef BUTTON_TASK_H
#define BUTTON_TASK_H

#include <Arduino.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "DisplayTask.h"  // Para tener DisplayCommand

class ButtonTask {
public:
    // Ahora recibe DOS colas: una para DisplayTask y otra para SensorTask
    // Así cada tarea recibe su propia copia del comando (mismo principio que con los sensores)
    static void start(QueueHandle_t cmdQueueDisplay, QueueHandle_t cmdQueueSensor);

private:
    static TaskHandle_t  _taskHandle;         // Manejador de la tarea FreeRTOS
    static QueueHandle_t _cmdQueueDisplay;    // Cola de comandos para DisplayTask
    static QueueHandle_t _cmdQueueSensor;     // Cola de comandos para SensorTask

    static void taskFunction(void* pvParams); // Bucle principal
};

#endif