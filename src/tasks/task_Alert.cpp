#include "task_Alert.h"
#include "../../include/config.h"
#include "../network/NTPManager.h"  
#include <SPI.h>
#include <SD.h>

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================
TaskHandle_t  AlertTask::_taskHandle      = nullptr;
QueueHandle_t AlertTask::_sensorQueue     = nullptr;
QueueHandle_t AlertTask::_recQueue        = nullptr;
QueueHandle_t AlertTask::_cmdQueue        = nullptr;
unsigned long* AlertTask::_sessionCounter = nullptr;

bool          AlertTask::_sessionActive    = false;
unsigned long AlertTask::_sessionStartTime = 0;

File AlertTask::_alertsFile;
bool AlertTask::_alertsFileOpen = false;
unsigned long AlertTask::_lastSessionId = 0;  // ✅ NUEVO: Inicialización

// ============================================================================
// start() - Inicializa pines y crea la tarea
// ============================================================================
void AlertTask::start(QueueHandle_t sensorQueue,
                      QueueHandle_t recQueue,
                      unsigned long* sessionCounter,
                      QueueHandle_t cmdQueue) {
    _sensorQueue     = sensorQueue;
    _recQueue        = recQueue;
    _sessionCounter  = sessionCounter;
    _cmdQueue        = cmdQueue;

    pinMode(LED_RED_PIN,    OUTPUT);
    pinMode(LED_GREEN_PIN,  OUTPUT);
    pinMode(LED_YELLOW_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    digitalWrite(LED_RED_PIN,    LOW);
    digitalWrite(LED_GREEN_PIN,  LOW);
    digitalWrite(LED_YELLOW_PIN, LOW);

    xTaskCreatePinnedToCore(
        taskFunction,
        "task_Alert",
        ALERT_TASK_STACK,
        nullptr,
        ALERT_TASK_PRIORITY,
        &_taskHandle,
        1
    );
}

// ============================================================================
// taskFunction() - Bucle principal
// ============================================================================
void AlertTask::taskFunction(void* pvParams) {
    SensorData   data;
    AlertCommand cmd;
    static int lastState = -1;

    while (true) {
        if (xQueueReceive(_cmdQueue, &cmd, 0) == pdTRUE) {
            _sessionActive = cmd.sessionActive;

            if (_sessionActive) {
                _sessionStartTime = millis();
                vTaskDelay(pdMS_TO_TICKS(100));
                createAlertsFile();
                setLedState(0);
                Serial.println("[Alert] Sesion iniciada");
            } else {
                closeAlertsFile();
                digitalWrite(LED_RED_PIN,    LOW);
                digitalWrite(LED_GREEN_PIN,  LOW);
                digitalWrite(LED_YELLOW_PIN, LOW);
                Serial.println("[Alert] Sesion finalizada");
            }
        }

        if (_sessionActive && xQueueReceive(_sensorQueue, &data, 0) == pdTRUE) {
            int globalState = getGlobalState(data);

            if (globalState != lastState) {
                const char* estadoTexto;
                switch (globalState) {
                    case 0: estadoTexto = "BUENO (Verde)"; break;
                    case 1: estadoTexto = "REGULAR (Amarillo)"; break;
                    case 2: estadoTexto = "MALO (Rojo)"; break;
                    default: estadoTexto = "DESCONOCIDO"; break;
                }
                Serial.printf("[Alert] Estado cambiado a: %s\n", estadoTexto);
                lastState = globalState;
            }

            setLedState(globalState);

            if (globalState == 2) {
                playAlarm();

                const char* rec = getRecommendation(data);
                if (rec != nullptr) {
                    Recommendation recom;
                    strncpy(recom.message, rec, sizeof(recom.message) - 1);
                    recom.message[sizeof(recom.message) - 1] = '\0';
                    recom.duration = RECOMMENDATION_DURATION_MS;
                    xQueueSend(_recQueue, &recom, 0);

                    const char* type = "";
                    if      (getCo2State(data.co2) == 2)         type = "CO2";
                    else if (getTempState(data.temperature) == 2) type = "Temperatura";
                    else if (getHumState(data.humidity) == 2)     type = "Humedad";
                    else if (getLightState(data.light) == 2)      type = "Iluminacion";

                    saveAlertToSD(type, rec);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// getCurrentTimeString() - Obtiene la hora actual (REAL o relativa)
// ============================================================================
String AlertTask::getCurrentTimeString() {
    // ================================================================
    // USAR HORA REAL DESDE NTP SI ESTÁ DISPONIBLE
    // ================================================================
    if (NTPManager::isTimeSynced()) {
        return NTPManager::getCurrentTime();  // "HH:MM:SS"
    }
    
    // ================================================================
    // FALLBACK: Tiempo desde inicio de sesión
    // ================================================================
    unsigned long elapsed = (millis() - _sessionStartTime) / 1000;
    unsigned long horas   = elapsed / 3600;
    unsigned long minutos = (elapsed % 3600) / 60;
    char timeStr[10];
    snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu", horas, minutos);
    return String(timeStr);
}

// ============================================================================
// createAlertsFile() - Crea el archivo JSON al iniciar sesión (VERSIÓN CORREGIDA)
// ============================================================================
void AlertTask::createAlertsFile() {
    // ============================================================
    // 🔥 PASO 1: SIEMPRE forzar cierre del archivo anterior
    // ============================================================
    if (_alertsFileOpen) {
        Serial.println("[Alert] ⚠️ Cerrando archivo anterior...");
        if (_alertsFile) {
            _alertsFile.println("\n  ]");
            _alertsFile.println("}");
            _alertsFile.close();
        }
        _alertsFileOpen = false;
    }

    // ============================================================
    // PASO 2: Verificar que el contador existe
    // ============================================================
    if (_sessionCounter == nullptr) {
        Serial.println("[Alert] ❌ _sessionCounter es NULL");
        return;
    }

    // ============================================================
    // PASO 3: Crear el archivo NUEVO con el número de sesión actual
    // ============================================================
    char filePath[64];
    snprintf(filePath, sizeof(filePath), "%s/session_%03lu_alerts.json",
             SD_BASE_PATH, *_sessionCounter);

    Serial.printf("[Alert] 📂 Creando archivo: %s\n", filePath);

    _alertsFile = SD.open(filePath, FILE_WRITE);
    if (_alertsFile) {
        _alertsFile.println("{");
        _alertsFile.printf("  \"sessionId\": %lu,\n", *_sessionCounter);
        _alertsFile.println("  \"alerts\": [");
        _alertsFileOpen = true;
        _lastSessionId = *_sessionCounter;  // ✅ Guardar ID de esta sesión
        Serial.printf("[Alert] ✅ Archivo creado: %s\n", filePath);
    } else {
        Serial.printf("[Alert] ❌ Error al crear archivo: %s\n", filePath);
    }
}

// ============================================================================
// closeAlertsFile() - Cierra el archivo JSON al finalizar sesión
// ============================================================================
void AlertTask::closeAlertsFile() {
    if (_alertsFileOpen && _alertsFile) {
        Serial.println("[Alert] 📝 Cerrando archivo...");
        _alertsFile.println("\n  ]");
        _alertsFile.println("}");
        _alertsFile.close();
        _alertsFileOpen = false;
        Serial.println("[Alert] ✅ Archivo de alertas cerrado");
    } else {
        Serial.println("[Alert] ⚠️ Archivo ya estaba cerrado");
    }
}

// ============================================================================
// saveAlertToSD() - Guarda una alerta en el archivo JSON (VERSIÓN CORREGIDA)
// ============================================================================
void AlertTask::saveAlertToSD(const char* type, const char* message) {
    // ============================================================
    // 🔥 PASO 1: Verificar que el archivo está abierto
    // ============================================================
    if (!_alertsFileOpen) {
        Serial.println("[Alert] ⚠️ Archivo cerrado, creando nuevo...");
        createAlertsFile();
        if (!_alertsFileOpen) {
            Serial.println("[Alert] ❌ No se pudo crear archivo");
            return;
        }
    }

    // ============================================================
    // 🔥 PASO 2: Verificar que la sesión NO ha cambiado
    // ============================================================
    if (_lastSessionId != *_sessionCounter) {
        Serial.printf("[Alert] ⚠️ Sesión cambió (%lu → %lu), recreando archivo...\n",
                      _lastSessionId, *_sessionCounter);
        createAlertsFile();  // Esto cierra el anterior y crea el nuevo
        if (!_alertsFileOpen) return;
    }

    // ============================================================
    // PASO 3: Si llegamos aquí, el archivo es el correcto
    // ============================================================
    if (!_alertsFile || _sessionCounter == nullptr) return;

    static bool firstAlert = true;
    static unsigned long cachedSessionId = 0;

    // Resetear firstAlert si cambió la sesión
    if (cachedSessionId != *_sessionCounter) {
        firstAlert = true;
        cachedSessionId = *_sessionCounter;
    }

    // Añadir coma si no es la primera alerta
    if (!firstAlert) {
        _alertsFile.println(",");
    }
    firstAlert = false;

    // Guardar la alerta
    String timeStr = getCurrentTimeString();
    unsigned long elapsed = millis() - _sessionStartTime;

    _alertsFile.printf("    {\n");
    _alertsFile.printf("      \"timestamp_ms\": %lu,\n", elapsed);
    _alertsFile.printf("      \"time\": \"%s\",\n", timeStr.c_str());
    _alertsFile.printf("      \"type\": \"%s\",\n", type);
    _alertsFile.printf("      \"message\": \"%s\"\n", message);
    _alertsFile.printf("    }");
    _alertsFile.flush();

    Serial.printf("[Alert] 💾 Alerta guardada: %s - %s (Hora: %s)\n", type, message, timeStr.c_str());
}

// ============================================================================
// getCo2State()
// ============================================================================
int AlertTask::getCo2State(float co2) {
    if (co2 < CO2_GOOD_MAX)        return 0;
    if (co2 <= CO2_ACCEPTABLE_MAX) return 1;
    return 2;
}

// ============================================================================
// getTempState()
// ============================================================================
int AlertTask::getTempState(float temp) {
    if (temp >= TEMP_GOOD_MIN && temp <= TEMP_GOOD_MAX) return 0;
    if (temp <= TEMP_ACCEPTABLE_MAX)                    return 1;
    return 2;
}

// ============================================================================
// getHumState()
// ============================================================================
int AlertTask::getHumState(float hum) {
    if (hum >= HUM_GOOD_MIN && hum <= HUM_GOOD_MAX) return 0;
    if ((hum >= HUM_ACCEPTABLE_MIN1 && hum <= HUM_ACCEPTABLE_MAX1) ||
        (hum >= HUM_ACCEPTABLE_MIN2 && hum <= HUM_ACCEPTABLE_MAX2)) return 1;
    return 2;
}

// ============================================================================
// getLightState()
// ============================================================================
int AlertTask::getLightState(float light) {
    if (light < LIGHT_GOOD_MAX)       return 0;
    if (light < LIGHT_ACCEPTABLE_MAX) return 1;
    return 2;
}

// ============================================================================
// getGlobalState() - Devuelve el peor estado de todas las variables
// ============================================================================
int AlertTask::getGlobalState(SensorData &data) {
    int states[4];
    states[0] = getCo2State(data.co2);
    states[1] = getTempState(data.temperature);
    states[2] = getHumState(data.humidity);
    states[3] = getLightState(data.light);

    int worst = 0;
    for (int i = 0; i < 4; i++) {
        if (states[i] > worst) worst = states[i];
    }
    return worst;
}

// ============================================================================
// setLedState() - 0=Verde, 1=Amarillo, 2=Rojo
// ============================================================================
void AlertTask::setLedState(int state) {
    digitalWrite(LED_RED_PIN,    LOW);
    digitalWrite(LED_GREEN_PIN,  LOW);
    digitalWrite(LED_YELLOW_PIN, LOW);

    switch (state) {
        case 0: digitalWrite(LED_GREEN_PIN,  HIGH); break;
        case 1: digitalWrite(LED_YELLOW_PIN, HIGH); break;
        case 2: digitalWrite(LED_RED_PIN,    HIGH); break;
    }
}

// ============================================================================
// playAlarm() - Melodía de alarma
// ============================================================================
void AlertTask::playAlarm() {
    tone(BUZZER_PIN, 523);  vTaskDelay(pdMS_TO_TICKS(150));
    noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(40));
    tone(BUZZER_PIN, 659);  vTaskDelay(pdMS_TO_TICKS(150));
    noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(40));
    tone(BUZZER_PIN, 784);  vTaskDelay(pdMS_TO_TICKS(150));
    noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(40));
    tone(BUZZER_PIN, 1047); vTaskDelay(pdMS_TO_TICKS(300));
    noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(150));

    for (int i = 0; i < 2; i++) {
        tone(BUZZER_PIN, 1800); vTaskDelay(pdMS_TO_TICKS(180));
        noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(60));
        tone(BUZZER_PIN, 2400); vTaskDelay(pdMS_TO_TICKS(180));
        noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(60));
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    tone(BUZZER_PIN, 880);  vTaskDelay(pdMS_TO_TICKS(400));
    noTone(BUZZER_PIN);
}

// ============================================================================
// getRecommendation() - Prioridad: CO2 > Temperatura > Humedad > Luz
// ============================================================================
const char* AlertTask::getRecommendation(SensorData &data) {
    if (getCo2State(data.co2) == 2) {
        return "Ventilar la habitacion";
    }
    if (getTempState(data.temperature) == 2) {
        return (data.temperature > TEMP_ACCEPTABLE_MAX)
            ? "Reducir temperatura"
            : "Aumentar temperatura";
    }
    if (getHumState(data.humidity) == 2) {
        return (data.humidity > HUM_ACCEPTABLE_MAX2)
            ? "Reducir humedad"
            : "Aumentar humedad";
    }
    if (getLightState(data.light) == 2) {
        return "Reducir iluminacion";
    }
    return nullptr;
}