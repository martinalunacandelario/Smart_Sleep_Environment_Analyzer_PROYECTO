// ============================================================================
// AlertTask.h - Tarea de gestión de alertas (Prioridad ALTA)
// ============================================================================
// Funciones: Controla LED RGB, activación del buzzer, detección de estados
// críticos, creación de avisos y recomendaciones automáticas
// Comunicación: Recibe datos de SensorTask, envía alertas a DisplayTask
// Prioridad: ALTA (3) - Las alertas deben procesarse rápidamente
// ============================================================================

#ifndef ALERT_TASK_H
#define ALERT_TASK_H

#include <Arduino.h>
#include <FreeRTOS.h>
#include <queue.h>
#include "DataStructures.h"

// ============================================================================
// CLASE ALERT TASK
// ============================================================================

class AlertTask {
public:
    // ------------------------------------------------------------------------
    // start() - Inicia la tarea de alertas
    // Recibe: 
    //   - inputQueue: cola de donde recibe los datos de sensores
    //   - outputQueue: cola donde envía las alertas a DisplayTask
    // ------------------------------------------------------------------------
    static void start(QueueHandle_t inputQueue, QueueHandle_t outputQueue);
    
    // ------------------------------------------------------------------------
    // setSessionActive() - Notifica si hay sesión activa
    // ------------------------------------------------------------------------
    static void setSessionActive(bool active);
    
    // ------------------------------------------------------------------------
    // getAlertQueue() - Devuelve la cola de alertas
    // ------------------------------------------------------------------------
    static QueueHandle_t getAlertQueue();
    
private:
    // ------------------------------------------------------------------------
    // taskFunction() - Bucle principal de la tarea FreeRTOS
    // ------------------------------------------------------------------------
    static void taskFunction(void* pvParams);
    
    // ------------------------------------------------------------------------
    // updateLED() - Actualiza el color del LED RGB según el estado
    // ------------------------------------------------------------------------
    static void updateLED(const SensorData& data);
    
    // ------------------------------------------------------------------------
    // checkAlerts() - Verifica condiciones y genera alertas
    // ------------------------------------------------------------------------
    static void checkAlerts(const SensorData& data);
    
    // ------------------------------------------------------------------------
    // sendAlert() - Envía una alerta a DisplayTask
    // ------------------------------------------------------------------------
    static void sendAlert(const char* type, const char* message);
    
    // ------------------------------------------------------------------------
    // buzzerBeep() - Activa el buzzer durante un breve periodo
    // ------------------------------------------------------------------------
    static void buzzerBeep();
    
    // ------------------------------------------------------------------------
    // Miembros estáticos
    // ------------------------------------------------------------------------
    static TaskHandle_t _taskHandle;      // Manejador de la tarea
    static QueueHandle_t _sensorQueue;    // Cola para recibir datos de sensores
    static QueueHandle_t _alertQueue;     // Cola para enviar alertas
    static bool _sessionActive;           // Estado de la sesión
    static unsigned long _lastAlertTime;  // Último tiempo de alerta (cooldown)
    static String _lastAlertType;         // Último tipo de alerta
    static int _lastState;                // Último estado para detectar cambios
};

#endif // ALERT_TASK_H