#ifndef ALERT_TASK_H
#define ALERT_TASK_H

#include <Arduino.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "SensorTask.h"      // Para SensorData

// Estructura para enviar recomendaciones a DisplayTask
struct Recommendation {
    char message[64];        // Texto de la recomendación
    unsigned long duration;  // Duración en ms (0 = usar valor por defecto)
};

class AlertTask {
public:
    // Inicia la tarea: recibe cola de sensores y cola de recomendaciones
    static void start(QueueHandle_t sensorQueue, QueueHandle_t recQueue);

private:
    static TaskHandle_t _taskHandle;
    static QueueHandle_t _sensorQueue;    // Cola para recibir datos de sensores
    static QueueHandle_t _recQueue;       // Cola para enviar recomendaciones a DisplayTask

    static void taskFunction(void* pvParams);

    // Evaluar estado de cada variable
    static int getCo2State(float co2);
    static int getTempState(float temp);
    static int getHumState(float hum);
    static int getLightState(float light);

    // Obtener estado global (0=Verde, 1=Amarillo, 2=Rojo)
    static int getGlobalState(SensorData &data);

    // Controlar LED RGB según estado
    static void setLedState(int state);

    // Activar buzzer con melodía de alarma
    static void playAlarm();

    // Generar recomendación según la variable crítica
    static const char* getRecommendation(SensorData &data);
};

#endif