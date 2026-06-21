#include <Arduino.h>
#include "tasks/task_Sensor.h"
#include "tasks/task_Display.h"
#include "tasks/task_Button.h"
#include "tasks/task_Alert.h"
#include "tasks/task_Storage.h"
#include "tasks/task_Analysis.h"
#include "tasks/task_WebServer.h"
#include "SessionManager.h"
#include "network/NTPManager.h"
#include "network/OTAManager.h"

// ============================================================================
// COLAS DE DATOS DE SENSORES (una por consumidor)
// ============================================================================
QueueHandle_t sensorQueueForDisplay = nullptr;  // SensorTask → DisplayTask
QueueHandle_t sensorQueueForAlert   = nullptr;  // SensorTask → AlertTask
QueueHandle_t sensorQueueForStorage = nullptr;  // SensorTask → StorageTask

// ============================================================================
// COLAS DE COMANDOS DE SESIÓN (una por consumidor, gestionadas por SessionManager)
// ============================================================================
QueueHandle_t sessionQueueDisplay  = nullptr;  // SessionManager → DisplayTask
QueueHandle_t sessionQueueSensor   = nullptr;  // SessionManager → SensorTask
QueueHandle_t sessionQueueStorage  = nullptr;  // SessionManager → StorageTask
QueueHandle_t sessionQueueAlert    = nullptr;  // SessionManager → AlertTask
QueueHandle_t sessionQueueAnalysis = nullptr;  // SessionManager → AnalysisTask

// ============================================================================
// COLA DE RECOMENDACIONES
// ============================================================================
QueueHandle_t recommendationQueue  = nullptr;  // AlertTask → DisplayTask

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n=== Smart Sleep Environment Analyzer ===");

    SessionManager::init();

    // ================================================================
    // FIX: NTP PRIMERO. Esto es lo que realmente inicializa la pila
    // WiFi del ESP32 (WiFi.mode() / WiFi.begin() / softAP()).
    // OTA NO puede inicializarse antes de esto, porque ArduinoOTA
    // depende de mDNS y de la interfaz WiFi ya activa. Si se llama
    // antes, el ESP32 crashea y entra en boot loop.
    // ================================================================
    if (NTPManager::begin()) {
        Serial.println("[NTP] ✅ Red configurada correctamente");
        if (NTPManager::isTimeSynced()) {
            Serial.printf("[NTP] Hora NTP: %s\n", NTPManager::getCurrentDateTime().c_str());
        }
    } else {
        Serial.println("[NTP] ⚠️ No hay WiFi. La hora no será real.");
        Serial.println("[NTP] El sistema funcionará con tiempo desde encendido (millis) como fallback.");
    }

    // ================================================================
    // FIX: OTA AHORA, con el WiFi ya levantado por NTPManager::begin()
    // ================================================================
    OTAManager::begin("SmartSleep");

    // ========================================================================
    // COLAS DE DATOS
    // ========================================================================
    sensorQueueForDisplay = xQueueCreate(10, sizeof(SensorData));
    if (sensorQueueForDisplay == nullptr) {
        Serial.println("Error al crear sensorQueueForDisplay"); while(1);
    }

    sensorQueueForAlert = xQueueCreate(10, sizeof(SensorData));
    if (sensorQueueForAlert == nullptr) {
        Serial.println("Error al crear sensorQueueForAlert"); while(1);
    }

    sensorQueueForStorage = xQueueCreate(10, sizeof(SensorData));
    if (sensorQueueForStorage == nullptr) {
        Serial.println("Error al crear sensorQueueForStorage"); while(1);
    }

    // ========================================================================
    // COLAS DE COMANDOS DE SESIÓN
    // ========================================================================
    sessionQueueDisplay = xQueueCreate(5, sizeof(SessionCommand));
    if (sessionQueueDisplay == nullptr) {
        Serial.println("Error al crear sessionQueueDisplay"); while(1);
    }

    sessionQueueSensor = xQueueCreate(5, sizeof(SessionCommand));
    if (sessionQueueSensor == nullptr) {
        Serial.println("Error al crear sessionQueueSensor"); while(1);
    }

    sessionQueueStorage = xQueueCreate(5, sizeof(SessionCommand));
    if (sessionQueueStorage == nullptr) {
        Serial.println("Error al crear sessionQueueStorage"); while(1);
    }

    sessionQueueAlert = xQueueCreate(5, sizeof(SessionCommand));
    if (sessionQueueAlert == nullptr) {
        Serial.println("Error al crear sessionQueueAlert"); while(1);
    }

    sessionQueueAnalysis = xQueueCreate(5, sizeof(SessionCommand));
    if (sessionQueueAnalysis == nullptr) {
        Serial.println("Error al crear sessionQueueAnalysis"); while(1);
    }

    recommendationQueue = xQueueCreate(5, sizeof(Recommendation));
    if (recommendationQueue == nullptr) {
        Serial.println("Error al crear recommendationQueue"); while(1);
    }

    // ========================================================================
    // SUSCRIBIR COLAS AL SESSION MANAGER
    // ========================================================================
    SessionManager::subscribe(sessionQueueDisplay);
    SessionManager::subscribe(sessionQueueSensor);
    SessionManager::subscribe(sessionQueueStorage);
    SessionManager::subscribe(sessionQueueAlert);
    SessionManager::subscribe(sessionQueueAnalysis);

    // ========================================================================
    // INICIO DE TAREAS
    // ========================================================================
    SensorTask::start(sensorQueueForDisplay, sensorQueueForAlert,
                      sensorQueueForStorage, sessionQueueSensor);

    DisplayTask::start(sensorQueueForDisplay, sessionQueueDisplay, recommendationQueue);

    ButtonTask::start();

    AlertTask::start(sensorQueueForAlert, recommendationQueue,
                     StorageTask::getSessionCounterPtr(), sessionQueueAlert);

    StorageTask::start(sensorQueueForStorage, sessionQueueStorage);

    AnalysisTask::start(sessionQueueAnalysis, StorageTask::getSessionCounterPtr());

    WebServerTask::start();
}

void loop() {
    // ================================================================
    // OTA fallback: ya se gestiona también dentro de WebServerTask,
    // pero lo dejamos aquí también por seguridad.
    // ================================================================
   OTAManager::handle();
    
    vTaskDelay(pdMS_TO_TICKS(1000));
}