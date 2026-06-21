/**
 * OTAManager.cpp - Gestión de actualizaciones OTA (Over-The-Air)
 * 
 * Implementa el servicio OTA para actualizar el firmware del ESP32 por WiFi.
 * Los eventos onStart/onProgress/onEnd/onError permiten monitorizar el proceso.
 *
 * NOTA: mDNS está desactivado (setMdnsEnabled(false)) porque entra en
 * conflicto con el modo dual WIFI_AP_STA del ESP32 y provoca un crash
 * (assert xQueueSemaphoreTake) al arrancar. No hace falta mDNS porque
 * subimos el firmware por IP fija (192.168.4.1, ver platformio.ini).
 */

#include "OTAManager.h"
#include "../../include/config.h"

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================
bool OTAManager::_initialized = false;
bool OTAManager::_otaRunning = false;
unsigned long OTAManager::_otaStartTime = 0;

// ============================================================================
// begin() - Inicia el servicio OTA
// ============================================================================
void OTAManager::begin(const char* hostname) {
    if (_initialized) return;
    
    ArduinoOTA.setHostname(hostname);
    
    // Evento: inicio de actualización
    ArduinoOTA.onStart([]() {
        Serial.println("\n[OTA] Iniciando actualización...");
        _otaRunning = true;
        _otaStartTime = millis();
        
        // LED rojo durante OTA
        digitalWrite(LED_RED_PIN, HIGH);
        digitalWrite(LED_GREEN_PIN, LOW);
        digitalWrite(LED_YELLOW_PIN, LOW);
    });
    
    // Evento: fin de actualización (éxito)
    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] ¡Actualización completada!");
        _otaRunning = false;
        
        // LED verde = éxito
        digitalWrite(LED_RED_PIN, LOW);
        digitalWrite(LED_GREEN_PIN, HIGH);
        digitalWrite(LED_YELLOW_PIN, LOW);
        delay(2000);
        digitalWrite(LED_GREEN_PIN, LOW);
    });
    
    // Evento: progreso de actualización
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static unsigned int lastPercent = 0;
        unsigned int percent = (progress / (total / 100));
        if (percent != lastPercent) {
            lastPercent = percent;
            Serial.printf("[OTA] Progreso: %u%%\n", percent);
        }
    });
    
    // Evento: error durante actualización
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]: ", error);
        switch (error) {
            case OTA_AUTH_ERROR:    Serial.println("Error de autenticación"); break;
            case OTA_BEGIN_ERROR:   Serial.println("Error al iniciar"); break;
            case OTA_CONNECT_ERROR: Serial.println("Error de conexión"); break;
            case OTA_RECEIVE_ERROR: Serial.println("Error al recibir datos"); break;
            case OTA_END_ERROR:     Serial.println("Error al finalizar"); break;
        }
        _otaRunning = false;
        
        // LED rojo parpadeante = error
        for (int i = 0; i < 5; i++) {
            digitalWrite(LED_RED_PIN, HIGH);
            delay(200);
            digitalWrite(LED_RED_PIN, LOW);
            delay(200);
        }
    });
    
    // ================================================================
    // FIX: Desactivar mDNS ANTES de begin()
    // mDNS + WIFI_AP_STA causaba el crash con xQueueSemaphoreTake.
    // No lo necesitamos: subimos por IP fija (192.168.4.1).
    // ================================================================
    ArduinoOTA.setMdnsEnabled(false);  // <-- Desactivar mDNS
    ArduinoOTA.begin();                // <-- Ahora begin() sin parámetros
    _initialized = true;
    
    Serial.printf("[OTA] Iniciado (sin mDNS) en IP: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.println("[OTA] Para actualizar: conecta tu PC al AP y ejecuta:");
    Serial.println("[OTA] pio run --target upload --upload-port 192.168.4.1 -e ota");
}

// ============================================================================
// handle() - Procesa peticiones OTA (llamar periódicamente)
// ============================================================================
void OTAManager::handle() {
    if (_initialized) {
        ArduinoOTA.handle();
    }
}

// ============================================================================
// isRunning() - Devuelve true si hay actualización OTA en curso
// ============================================================================
bool OTAManager::isRunning() {
    return _otaRunning;
}
