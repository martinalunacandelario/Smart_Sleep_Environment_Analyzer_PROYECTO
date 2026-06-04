#ifndef WEB_SERVER_TASK_H
#define WEB_SERVER_TASK_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <freertos/task.h>
#include "SensorTask.h"
#include "../SessionManager.h"  // Para controlar sesiones sin colas

class WebServerTask {
public:
    // Ya no recibe colas: usa SessionManager para controlar sesiones
    static void start();

    // Llamado desde SensorTask para mantener datos actualizados en la web
    static void updateCurrentData(const SensorData &data);

private:
    static TaskHandle_t _taskHandle;
    static WebServer    server;
    static SensorData   _currentData;  // Últimos datos recibidos de SensorTask

    static void setupAccessPoint();
    static void taskFunction(void* pvParams);

    // Handlers de la API REST
    static void handleRoot();
    static void handleApiStatus();
    static void handleApiSession();        // POST /api/session → llama SessionManager
    static void handleApiSessions();
    static void handleApiSessionStats();
    static void handleApiSessionAlerts();
    static void handleApiSessionData();
    static void handleNotFound();
};

#endif