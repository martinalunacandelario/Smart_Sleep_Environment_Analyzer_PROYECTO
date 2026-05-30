// ============================================================================
// WebServerTask.h - Tarea del servidor web (Prioridad MEDIA)
// ============================================================================
// Funciones: Gestiona servidor web integrado, API REST, interfaz gráfica
// Comunicación: Recibe comandos de web, envía comandos a StorageTask
// Prioridad: MEDIA (2) - Debe responder rápido pero no es crítica
// ============================================================================

#ifndef WEBSERVER_TASK_H
#define WEBSERVER_TASK_H

#include <Arduino.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "DataStructures.h"

// ============================================================================
// CLASE WEBSERVER TASK
// ============================================================================

class WebServerTask {
public:
    // ------------------------------------------------------------------------
    // start() - Inicia la tarea del servidor web
    // Recibe:
    //   - storageQueue: cola donde envía comandos a StorageTask
    //   - sensorQueue: cola para leer datos de sensores (opcional)
    // ------------------------------------------------------------------------
    static void start(QueueHandle_t storageQueue, QueueHandle_t sensorQueue = nullptr);
    
    // ------------------------------------------------------------------------
    // setSessionActive() - Actualiza estado de sesión desde otras tareas
    // ------------------------------------------------------------------------
    static void setSessionActive(bool active, unsigned long startTime = 0);
    
    // ------------------------------------------------------------------------
    // getLastScore() - Devuelve el último Sleep Score calculado
    // ------------------------------------------------------------------------
    static int getLastScore();
    
    // ------------------------------------------------------------------------
    // getLocalIP() - Devuelve la IP actual (para mostrar en OLED)
    // ------------------------------------------------------------------------
    static String getLocalIP();
    
    // ------------------------------------------------------------------------
    // isWiFiConnected() - Devuelve si WiFi está conectado
    // ------------------------------------------------------------------------
    static bool isWiFiConnected();
    
private:
    // ------------------------------------------------------------------------
    // taskFunction() - Bucle principal de la tarea FreeRTOS
    // ------------------------------------------------------------------------
    static void taskFunction(void* pvParams);
    
    // ------------------------------------------------------------------------
    // initWiFi() - Inicializa la conexión WiFi (STA o AP)
    // ------------------------------------------------------------------------
    static void initWiFi();
    
    // ------------------------------------------------------------------------
    // setupRoutes() - Configura todas las rutas del servidor web
    // ------------------------------------------------------------------------
    static void setupRoutes();
    
    // ------------------------------------------------------------------------
    // Manejadores de rutas HTTP
    // ------------------------------------------------------------------------
    static void handleRoot();           // Página principal HTML
    static void handleData();           // API: datos en tiempo real
    static void handleHistory();        // API: historial de sesiones
    static void handleStart();          // API: iniciar sesión
    static void handleEnd();            // API: finalizar sesión
    static void handleStatus();         // API: estado del sistema
    static void handleWiFiScan();       // API: escanear redes WiFi
    static void handleWiFiConfig();     // API: configurar nueva WiFi
    static void handleNotFound();       // 404 - Recurso no encontrado
    
    // ------------------------------------------------------------------------
    // Funciones de generación de respuestas
    // ------------------------------------------------------------------------
    static String generateHTML();       // Genera la página web completa
    static String getDataJSON();        // Genera JSON con datos actuales
    static String getHistoryJSON();     // Genera JSON con historial
    static String getStatusJSON();      // Genera JSON con estado del sistema
    
    // ------------------------------------------------------------------------
    // calculateSleepScore() - Calcula puntuación de sueño
    // ------------------------------------------------------------------------
    static int calculateSleepScore(const SensorData& data);
    static String getScoreInterpretation(int score);
    
    // ------------------------------------------------------------------------
    // Miembros estáticos
    // ------------------------------------------------------------------------
    static TaskHandle_t _taskHandle;          // Manejador de la tarea
    static WebServer* _server;                // Servidor web
    static QueueHandle_t _storageQueue;       // Cola para StorageTask
    static QueueHandle_t _sensorQueue;        // Cola para leer sensores
    
    // Estado del sistema
    static SensorData _currentData;           // Últimos datos de sensores
    static bool _sessionActive;               // Estado de sesión
    static unsigned long _sessionStartTime;   // Inicio de sesión
    static int _lastSleepScore;               // Último Sleep Score
    static int _sessionCount;                 // Número de sesiones guardadas
    
    // WiFi
    static bool _wifiConnected;               // Estado de conexión WiFi
    static String _localIP;                   // IP local
    static unsigned long _lastWiFiReconnect;  // Último intento de reconexión
    static unsigned long _wifiStartTime;      // Tiempo de inicio de conexión
};

#endif // WEBSERVER_TASK_H