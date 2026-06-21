#ifndef TASK_WEBSERVER_H
#define TASK_WEBSERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "task_Sensor.h"
#include "../SessionManager.h"

class WebServerTask {
public:
    static void start();
    static void updateCurrentData(const SensorData &data);

private:
    static TaskHandle_t _taskHandle;
    static WebServer server;
    static SensorData _currentData;
    static SemaphoreHandle_t _dataMutex;
   
    static void setupAccessPoint();
    static void taskFunction(void* pvParams);
   
    static void handleRoot();
    static void handleApiStatus();
    static void handleApiSession();
    static void handleApiSessions();
    static void handleApiSessionStats();
    static void handleApiSessionAlerts();
    static void handleApiSessionData();
    static void handleNotFound();
   
    // ========================================================================
    // NUEVA: Sirve Chart.js desde la memoria FLASH del ESP32
    // ========================================================================
    static void handleChartJs();
};

#endif // TASK_WEBSERVER_H