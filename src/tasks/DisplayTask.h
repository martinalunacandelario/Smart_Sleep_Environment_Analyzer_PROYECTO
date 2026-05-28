// ============================================================================
// DisplayTask.h - Tarea de visualización OLED (Prioridad BAJA)
// ============================================================================
// Funciones: Muestra datos ambientales, estado del sistema, alertas,
// recomendaciones y estado de sesiones en pantalla OLED SH1106
// Comunicación: Recibe datos de SensorTask y alertas de AlertTask
// Prioridad: BAJA (1) - Solo visualización, no crítica
// ============================================================================

#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

#include <Arduino.h>
#include <Wire.h>
#include <FreeRTOS.h>
#include <queue.h>
#include "DataStructures.h"

// ============================================================================
// CLASE DISPLAY TASK
// ============================================================================

class DisplayTask {
public:
    // ------------------------------------------------------------------------
    // start() - Inicia la tarea de display
    // Recibe:
    //   - sensorQueue: cola de donde recibe los datos de sensores
    //   - alertQueue: cola de donde recibe las alertas/recomendaciones
    // ------------------------------------------------------------------------
    static void start(QueueHandle_t sensorQueue, QueueHandle_t alertQueue);
    
private:
    // ------------------------------------------------------------------------
    // taskFunction() - Bucle principal de la tarea FreeRTOS
    // ------------------------------------------------------------------------
    static void taskFunction(void* pvParams);
    
    // ------------------------------------------------------------------------
    // initOLED() - Inicializa la pantalla OLED SH1106
    // ------------------------------------------------------------------------
    static void initOLED();
    
    // ------------------------------------------------------------------------
    // clearDisplay() - Limpia toda la pantalla OLED
    // ------------------------------------------------------------------------
    static void clearDisplay();
    
    // ------------------------------------------------------------------------
    // sendCommand() - Envía un comando I2C a la OLED
    // ------------------------------------------------------------------------
    static void sendCommand(uint8_t cmd);
    
    // ------------------------------------------------------------------------
    // sendData() - Envía datos I2C a la OLED
    // ------------------------------------------------------------------------
    static void sendData(uint8_t* data, int len);
    
    // ------------------------------------------------------------------------
    // writeLine() - Escribe texto en una línea de la pantalla
    // Líneas: 0, 1, 2, 3 (máximo 21 caracteres por línea)
    // ------------------------------------------------------------------------
    static void writeLine(int line, const char* text);
    
    // ------------------------------------------------------------------------
    // updateDisplay() - Actualiza la pantalla con datos normales
    // ------------------------------------------------------------------------
    static void updateDisplay();
    
    // ------------------------------------------------------------------------
    // showAlert() - Muestra una alerta temporal en pantalla
    // ------------------------------------------------------------------------
    static void showAlert(const DisplayAlert& alert);
    
    // ------------------------------------------------------------------------
    // getStateString() - Convierte estado numérico a texto
    // ------------------------------------------------------------------------
    static const char* getStateString(int state);
    
    // ------------------------------------------------------------------------
    // Miembros estáticos
    // ------------------------------------------------------------------------
    static TaskHandle_t _taskHandle;      // Manejador de la tarea
    static QueueHandle_t _sensorQueue;    // Cola para recibir datos de sensores
    static QueueHandle_t _alertQueue;     // Cola para recibir alertas
    
    static SensorData _currentData;       // Últimos datos recibidos
    static bool _showingAlert;            // Si se está mostrando una alerta
    static unsigned long _alertEndTime;   // Cuándo termina de mostrar la alerta
    static char _lastAlertMessage[64];    // Último mensaje de alerta
    static bool _sessionActive;           // Estado de sesión (desde fuera)
    static unsigned long _sessionStart;   // Inicio de sesión
};

#endif // DISPLAY_TASK_H