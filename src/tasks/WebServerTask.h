

#ifndef WEB_SERVER_TASK_H
#define WEB_SERVER_TASK_H


#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "SensorTask.h"
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
};


#endif
