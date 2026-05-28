// ============================================================================
// SensorTask.h - Tarea de lectura de sensores (Prioridad ALTA)
// ============================================================================
// Funciones: Lee SCD41 (CO2, Temp, Hum) y BH1750 (Luz) cada 5 segundos
// Comunicación: Envía datos a otras tareas mediante cola FreeRTOS
// Prioridad: ALTA (4) - No puede ser bloqueada por otras tareas
// ============================================================================

#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <Arduino.h>
#include <Wire.h>
#include <FreeRTOS.h>
#include <queue.h>

// ============================================================================
// ESTRUCTURA DE DATOS QUE SE ENVÍA A OTRAS TAREAS
// ============================================================================

struct SensorData {
    float co2 = 0;           // Dióxido de carbono (ppm)
    float temperature = 0;   // Temperatura (°C)
    float humidity = 0;      // Humedad relativa (%)
    float light = 0;         // Intensidad lumínica (lux)
    int state = 0;           // 0=OPTIMO, 1=REGULAR, 2=CRITICO
    unsigned long timestamp = 0;
    bool valid = false;
};

// ============================================================================
// CLASE SENSOR TASK
// ============================================================================

class SensorTask {
public:
    // ------------------------------------------------------------------------
    // start() - Inicia la tarea
    // Recibe la cola donde enviará los datos leídos
    // ------------------------------------------------------------------------
    static void start(QueueHandle_t outputQueue);
    
    // ------------------------------------------------------------------------
    // getDataQueue() - Devuelve la cola de datos (para otras tareas)
    // ------------------------------------------------------------------------
    static QueueHandle_t getDataQueue();
    
private:
    // ------------------------------------------------------------------------
    // taskFunction() - Bucle principal de la tarea FreeRTOS
    // ------------------------------------------------------------------------
    static void taskFunction(void* pvParams);
    
    // ------------------------------------------------------------------------
    // readSensors() - Lee físicamente todos los sensores
    // ------------------------------------------------------------------------
    static void readSensors(SensorData& data);
    
    // ------------------------------------------------------------------------
    // Funciones de lectura individuales (comunicación I2C)
    // ------------------------------------------------------------------------
    static float readCO2();          // Lee CO2 del SCD41
    static float readTemperature();  // Lee temperatura del SCD41
    static float readHumidity();     // Lee humedad del SCD41
    static float readLight();        // Lee lux del BH1750
    
    // ------------------------------------------------------------------------
    // calculateState() - Determina estado según umbrales
    // ------------------------------------------------------------------------
    static int calculateState(SensorData& data);
    
    // ------------------------------------------------------------------------
    // Inicialización de sensores
    // ------------------------------------------------------------------------
    static void initSCD41();
    static void initBH1750();
    
    // ------------------------------------------------------------------------
    // Miembros estáticos
    // ------------------------------------------------------------------------
    static TaskHandle_t _taskHandle;      // Manejador de la tarea
    static QueueHandle_t _sensorQueue;    // Cola para enviar datos
    static TwoWire _i2c;                  // Bus I2C dedicado
};

#endif // SENSOR_TASK_H