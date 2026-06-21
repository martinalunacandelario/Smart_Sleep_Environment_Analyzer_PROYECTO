/**
 * OTAManager.h - Gestión de actualizaciones OTA (Over-The-Air)
 * 
 * Permite actualizar el firmware del ESP32 por WiFi sin usar cable USB.
 * Uso: OTAManager::begin() en setup() y OTAManager::handle() en loop()
 */

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

class OTAManager {
public:
    /**
     * Inicia el servicio OTA
     * @param hostname Nombre del dispositivo (ej: "SmartSleep")
     *                 Se accede como "SmartSleep.local" en la red
     */
    static void begin(const char* hostname = "SmartSleep");

    /**
     * Procesa peticiones OTA. Llamar periódicamente en loop() o en tarea
     */
    static void handle();

    /**
     * Devuelve true si hay una actualización OTA en curso
     */
    static bool isRunning();

private:
    static bool _initialized;
    static bool _otaRunning;
    static unsigned long _otaStartTime;
};

#endif // OTA_MANAGER_H