#ifndef BUTTON_TASK_H
#define BUTTON_TASK_H

#include <Arduino.h>
#include <freertos/task.h>
#include "../SessionManager.h"  // Delega el control de sesión en SessionManager

class ButtonTask {
public:
    // Ya no recibe colas: delega en SessionManager
    static void start();

private:
    static TaskHandle_t _taskHandle;
    static void taskFunction(void* pvParams);
};

#endif