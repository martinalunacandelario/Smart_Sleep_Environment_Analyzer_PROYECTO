#ifndef TASK_DISPLAY_H
#define TASK_DISPLAY_H

#include <Arduino.h>                      // Base de Arduino (Serial, millis, etc.)
#include <U8g2lib.h>                     // Librería para OLED (SH1106)
#include <Wire.h>                        // Comunicación I2C
#include <freertos/task.h>               // Tareas FreeRTOS
#include <freertos/queue.h>              // Colas FreeRTOS
#include "task_Sensor.h"                  // Estructura SensorData (CO2, temp, hum, luz)
#include "task_Alert.h"                   // Estructura Recommendation (para recibir recomendaciones) ← NUEVO

// Comando que puede recibir la DisplayTask (inicio/fin sesión)
struct DisplayCommand {
    bool sessionActive;                  // true = iniciar sesión, false = finalizar
};

class DisplayTask {
public:
    // Inicia la tarea: recibe cola de sensores, cola de comandos y cola de recomendaciones ← MODIFICADO
    static void start(QueueHandle_t sensorQueue, QueueHandle_t cmdQueue, QueueHandle_t recQueue);

private:
    static TaskHandle_t _taskHandle;      // Manejador de la tarea FreeRTOS
    static QueueHandle_t _sensorQueue;    // Cola donde llegan los datos de SensorTask
    static QueueHandle_t _cmdQueue;       // Cola donde llegan comandos (inicio/fin sesión)
    static QueueHandle_t _recQueue;       // Cola donde llegan recomendaciones desde AlertTask ← NUEVO

    // Objeto de la pantalla OLED SH1106 (I2C hardware)
    static U8G2_SH1106_128X64_NONAME_F_HW_I2C _display;

    static SensorData _currentData;       // Últimos datos recibidos del sensor
    static bool _sessionActive;           // Estado actual de la sesión (ON/OFF)
    static unsigned long _sessionEndTime; // Momento (ms) para apagar pantalla tras finalizar sesión
    static bool _displayOn;               // Estado real de la pantalla (encendida/apagada)

    // Variables para mostrar recomendaciones temporales ← NUEVO
    static bool _showingRec;              // Indica si se está mostrando una recomendación
    static unsigned long _recEndTime;     // Momento (ms) en que termina de mostrarse
    static char _currentRec[64];          // Texto de la recomendación actual

    static void taskFunction(void* pvParams);   // Bucle principal de la tarea
    static void updateDisplay();                // Dibuja la información actual en la OLED
    static const char* getQualityString();      // Retorna "OPTIMO", "ACEPTABLE" o "DESFAVORABLE"
};

#endif // TASK_DISPLAY_H