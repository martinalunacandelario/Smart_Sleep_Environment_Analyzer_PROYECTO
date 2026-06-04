#include "AlertTask.h"
#include "../../include/config.h"
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
    _cmdQueue        = cmdQueue;        // Cola propia para comandos de sesión

    // Configurar pines del LED RGB como salida
    pinMode(LED_RED_PIN,    OUTPUT);
    pinMode(LED_GREEN_PIN,  OUTPUT);
    pinMode(LED_YELLOW_PIN, OUTPUT);

    // Configurar pin del buzzer como salida
    pinMode(BUZZER_PIN, OUTPUT);

    // Apagar todos los LEDs al inicio
    digitalWrite(LED_RED_PIN,    LOW);
    digitalWrite(LED_GREEN_PIN,  LOW);
    digitalWrite(LED_YELLOW_PIN, LOW);

    // Crear tarea FreeRTOS en el núcleo 1
    xTaskCreatePinnedToCore(
        taskFunction,
        "AlertTask",
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
    static int lastState = -1;  // Variable para recordar el último estado (depuración)

    while (true) {
        // 1. Comprobar si ha llegado un comando de inicio o fin de sesión
        if (xQueueReceive(_cmdQueue, &cmd, 0) == pdTRUE) {
            _sessionActive = cmd.sessionActive;

            if (_sessionActive) {
                _sessionStartTime = millis();   // Marcar inicio de sesión
                createAlertsFile();             // Crear archivo JSON de alertas
                setLedState(0);                 // LED verde al iniciar sesión
                Serial.println("[Alert] Sesion iniciada");
            } else {
                closeAlertsFile();              // Cerrar archivo JSON
                // Apagar todos los LEDs al finalizar sesión
                digitalWrite(LED_RED_PIN,    LOW);
                digitalWrite(LED_GREEN_PIN,  LOW);
                digitalWrite(LED_YELLOW_PIN, LOW);
                Serial.println("[Alert] Sesion finalizada");
            }
        }

        // 2. Solo procesar datos y activar alertas si la sesión está activa
        if (_sessionActive && xQueueReceive(_sensorQueue, &data, 0) == pdTRUE) {
            // Evaluar estado global (0=Verde, 1=Amarillo, 2=Rojo)
            int globalState = getGlobalState(data);

            // ============================================================
            // DEPURACIÓN: Mostrar cambio de estado en el monitor serie
            // ============================================================
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

            // Controlar LED RGB según el estado
            setLedState(globalState);

            // Si el estado es crítico: sonar alarma y enviar recomendación
            if (globalState == 2) {
                playAlarm();

                const char* rec = getRecommendation(data);
                if (rec != nullptr) {
                    // Enviar recomendación a DisplayTask
                    Recommendation recom;
                    strncpy(recom.message, rec, sizeof(recom.message) - 1);
                    recom.message[sizeof(recom.message) - 1] = '\0';
                    recom.duration = RECOMMENDATION_DURATION_MS;
                    xQueueSend(_recQueue, &recom, 0);

                    // Determinar tipo de alerta para el JSON
                    const char* type = "";
                    if      (getCo2State(data.co2) == 2)         type = "CO2";
                    else if (getTempState(data.temperature) == 2) type = "Temperatura";
                    else if (getHumState(data.humidity) == 2)     type = "Humedad";
                    else if (getLightState(data.light) == 2)      type = "Iluminacion";

                    saveAlertToSD(type, rec);
                }
            }
        }

        // Pausa para no saturar la CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// getCurrentTimeString() - Hora desde inicio de sesión en formato HH:MM
// ============================================================================
String AlertTask::getCurrentTimeString() {
    unsigned long elapsed = (millis() - _sessionStartTime) / 1000;
    unsigned long horas   = elapsed / 3600;
    unsigned long minutos = (elapsed % 3600) / 60;
    char timeStr[10];
    snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu", horas, minutos);
    return String(timeStr);
}

// ============================================================================
// createAlertsFile() - Crea el archivo JSON al iniciar sesión
// ============================================================================
void AlertTask::createAlertsFile() {
    if (_alertsFileOpen || _sessionCounter == nullptr) return;

    char filePath[64];
    snprintf(filePath, sizeof(filePath), "%s/session_%03lu_alerts.json",
             SD_BASE_PATH, *_sessionCounter);

    _alertsFile = SD.open(filePath, FILE_WRITE);
    if (_alertsFile) {
        _alertsFile.println("{");
        _alertsFile.printf("  \"sessionId\": %lu,\n", *_sessionCounter);
        _alertsFile.println("  \"alerts\": [");
        _alertsFileOpen = true;
        Serial.printf("[Alert] Archivo de alertas creado: %s\n", filePath);
    } else {
        Serial.println("[Alert] Error al crear archivo de alertas");
    }
}

// ============================================================================
// closeAlertsFile() - Cierra el archivo JSON al finalizar sesión
// ============================================================================
void AlertTask::closeAlertsFile() {
    if (_alertsFileOpen && _alertsFile) {
        _alertsFile.println("\n  ]");
        _alertsFile.println("}");
        _alertsFile.close();
        _alertsFileOpen = false;
        Serial.println("[Alert] Archivo de alertas cerrado");
    }
}

// ============================================================================
// saveAlertToSD() - Guarda una alerta en el archivo JSON
// ============================================================================
void AlertTask::saveAlertToSD(const char* type, const char* message) {
    if (!_alertsFileOpen || !_alertsFile) return;

    static bool firstAlert = true;

    if (!firstAlert) {
        _alertsFile.println(",");
    }
    firstAlert = false;

    String timeStr       = getCurrentTimeString();
    unsigned long elapsed = millis() - _sessionStartTime;

    _alertsFile.printf("    {\n");
    _alertsFile.printf("      \"timestamp_ms\": %lu,\n", elapsed);
    _alertsFile.printf("      \"time\": \"%s\",\n", timeStr.c_str());
    _alertsFile.printf("      \"type\": \"%s\",\n", type);
    _alertsFile.printf("      \"message\": \"%s\"\n", message);
    _alertsFile.printf("    }");
    _alertsFile.flush();

    Serial.printf("[Alert] Alerta guardada: %s - %s\n", type, message);
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
        case 0: digitalWrite(LED_GREEN_PIN,  HIGH); break;  // Óptimo → Verde
        case 1: digitalWrite(LED_YELLOW_PIN, HIGH); break;  // Regular → Amarillo
        case 2: digitalWrite(LED_RED_PIN,    HIGH); break;  // Crítico → Rojo
    }
}

// ============================================================================
// playAlarm() - Melodía: intro simpática + alerta urgente + cierre
// Usa vTaskDelay en lugar de delay() para no bloquear otras tareas del núcleo
// ============================================================================
void AlertTask::playAlarm() {
    // --- Intro ascendente (estilo videojuego) ---
    tone(BUZZER_PIN, 523);  vTaskDelay(pdMS_TO_TICKS(150));  // Do5
    noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(40));
    tone(BUZZER_PIN, 659);  vTaskDelay(pdMS_TO_TICKS(150));  // Mi5
    noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(40));
    tone(BUZZER_PIN, 784);  vTaskDelay(pdMS_TO_TICKS(150));  // Sol5
    noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(40));
    tone(BUZZER_PIN, 1047); vTaskDelay(pdMS_TO_TICKS(300));  // Do6 (nota larga)
    noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(150));

    // --- Alerta urgente (dos pares de pitidos agudos) ---
    for (int i = 0; i < 2; i++) {
        tone(BUZZER_PIN, 1800); vTaskDelay(pdMS_TO_TICKS(180));
        noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(60));
        tone(BUZZER_PIN, 2400); vTaskDelay(pdMS_TO_TICKS(180));
        noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(60));
    }

    // --- Cierre: nota de resolución ---
    vTaskDelay(pdMS_TO_TICKS(100));
    tone(BUZZER_PIN, 880);  vTaskDelay(pdMS_TO_TICKS(400));  // La5
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