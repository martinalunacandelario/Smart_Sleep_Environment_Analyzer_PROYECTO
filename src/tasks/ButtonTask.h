#ifndef BUTTON_TASK_H
#define BUTTON_TASK_H

#include <Arduino.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// Estructura que se enviará a la cola (ya definida en DisplayTask.h)
// Incluimos DisplayTask.h para tener DisplayCommand
#include "DisplayTask.h"

class ButtonTask {
public:
    // Inicia la tarea: recibe la cola donde enviar los comandos de sesión
    static void start(QueueHandle_t cmdQueue);

private:
    static TaskHandle_t _taskHandle;      // Manejador de la tarea FreeRTOS
    static QueueHandle_t _cmdQueue;       // Cola para enviar comandos (DisplayCommand)

    static void taskFunction(void* pvParams);  // Bucle principal
};

#endif