// ============================================================================
// StorageTask.h - Tarea de almacenamiento en microSD (Prioridad MUY BAJA)
// ============================================================================
// Funciones: Guarda sesiones, almacena estadísticas, registra históricos,
// guarda rankings y gestiona archivos del sistema
// Comunicación: Recibe comandos de WebServerTask y loop()
// Prioridad: MUY BAJA (0) - No debe bloquear tareas críticas
// ============================================================================

#ifndef STORAGE_TASK_H
#define STORAGE_TASK_H

#include <Arduino.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <SPI.h>
#include <SD.h>
#include "DataStructures.h"

// ============================================================================
// CLASE STORAGE TASK
// ============================================================================

class StorageTask {
public:
    // ------------------------------------------------------------------------
    // start() - Inicia la tarea de almacenamiento
    // Recibe la cola donde recibe comandos
    // ------------------------------------------------------------------------
    static void start(QueueHandle_t commandQueue);
    
    // ------------------------------------------------------------------------
    // isSDReady() - Devuelve si la tarjeta SD está disponible
    // ------------------------------------------------------------------------
    static bool isSDReady();
    
    // ------------------------------------------------------------------------
    // getSessionCount() - Devuelve número de sesiones guardadas
    // ------------------------------------------------------------------------
    static int getSessionCount();
    
private:
    // ------------------------------------------------------------------------
    // taskFunction() - Bucle principal de la tarea FreeRTOS
    // ------------------------------------------------------------------------
    static void taskFunction(void* pvParams);
    
    // ------------------------------------------------------------------------
    // initSD() - Inicializa la tarjeta microSD
    // ------------------------------------------------------------------------
    static void initSD();
    
    // ------------------------------------------------------------------------
    // createCSVHeaders() - Crea archivos CSV con cabeceras si no existen
    // ------------------------------------------------------------------------
    static void createCSVHeaders();
    
    // ------------------------------------------------------------------------
    // saveSession() - Guarda una sesión completa en SD
    // ------------------------------------------------------------------------
    static void saveSession(const StorageCommand& cmd);
    
    // ------------------------------------------------------------------------
    // saveDataPoint() - Guarda un punto de datos individual
    // ------------------------------------------------------------------------
    static void saveDataPoint(const DataPoint& point);
    
    // ------------------------------------------------------------------------
    // loadHistory() - Carga el historial desde SD a memoria
    // ------------------------------------------------------------------------
    static void loadHistory();
    
    // ------------------------------------------------------------------------
    // getTimestamp() - Devuelve timestamp formateado
    // ------------------------------------------------------------------------
    static String getTimestamp();
    
    // ------------------------------------------------------------------------
    // getDateString() - Devuelve fecha para nombre de archivo
    // ------------------------------------------------------------------------
    static String getDateString();
    
    // ------------------------------------------------------------------------
    // Miembros estáticos
    // ------------------------------------------------------------------------
    static TaskHandle_t _taskHandle;          // Manejador de la tarea
    static QueueHandle_t _commandQueue;       // Cola para recibir comandos
    static bool _sdReady;                     // Estado de la tarjeta SD
    static int _sessionCount;                 // Número de sesiones guardadas
    static int _dataPointCount;               // Contador de puntos guardados
    static String _currentDateFile;           // Archivo diario actual
    static unsigned long _lastFileCheck;      // Última comprobación de archivo
};

#endif // STORAGE_TASK_H