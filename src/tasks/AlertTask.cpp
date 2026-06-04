#include "AlertTask.h"
#include "../../include/config.h"
#include "DisplayTask.h"  // Para DisplayCommand (necesario para la estructura, aunque no se use)
#include <SPI.h>
#include <SD.h>

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================
TaskHandle_t AlertTask::_taskHandle = nullptr;
QueueHandle_t AlertTask::_sensorQueue = nullptr;
QueueHandle_t AlertTask::_recQueue = nullptr;
unsigned long* AlertTask::_sessionCounter = nullptr;

bool AlertTask::_sessionActive = false;
unsigned long AlertTask::_sessionStartTime = 0;

File AlertTask::_alertsFile;
bool AlertTask::_alertsFileOpen = false;

// ============================================================================
// start() - Inicializa pines y crea la tarea
// ============================================================================
void AlertTask::start(QueueHandle_t sensorQueue, QueueHandle_t recQueue, unsigned long* sessionCounter) {
    _sensorQueue = sensorQueue;
    _recQueue = recQueue;
    _sessionCounter = sessionCounter;

    // Configurar pines del LED RGB como salida
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(LED_YELLOW_PIN, OUTPUT);

    // Configurar pin del buzzer como salida
    pinMode(BUZZER_PIN, OUTPUT);

    // Apagar todos los LEDs al inicio
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, LOW);
    digitalWrite(LED_YELLOW_PIN, LOW);

    // Crear tarea FreeRTOS en el núcleo 1 (prioridad alta)
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
// getCurrentTimeString() - Devuelve la hora actual en formato HH:MM
// ============================================================================
String AlertTask::getCurrentTimeString() {
    // Tiempo transcurrido desde el inicio de la sesión (segundos)
    unsigned long elapsed = (millis() - _sessionStartTime) / 1000;
    unsigned long horas = elapsed / 3600;           // Horas transcurridas
    unsigned long minutos = (elapsed % 3600) / 60;  // Minutos transcurridos
    char timeStr[10];
    snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu", horas, minutos);
    return String(timeStr);
}

// ============================================================================
// createAlertsFile() - Crea el archivo JSON para alertas al iniciar sesión
// ============================================================================
void AlertTask::createAlertsFile() {
    if (_alertsFileOpen) return;  // Ya está abierto
    
    if (_sessionCounter == nullptr) return;  // No hay contador de sesiones
    
    char filePath[64];
    // Formato: /sessions/session_001_alerts.json
    snprintf(filePath, sizeof(filePath), "%s/session_%03lu_alerts.json", SD_BASE_PATH, *_sessionCounter);
    
    _alertsFile = SD.open(filePath, FILE_WRITE);
    if (_alertsFile) {
        // Escribir cabecera del JSON
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
        // Cerrar el array de alerts y el objeto JSON
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
    
    // Variable estática para saber si es la primera alerta (no poner coma antes)
    static bool firstAlert = true;
    
    // Si no es la primera alerta, añadir coma antes del nuevo objeto
    if (!firstAlert) {
        _alertsFile.println(",");
    }
    firstAlert = false;
    
    // Obtener tiempo actual desde inicio de sesión
    String timeStr = getCurrentTimeString();
    unsigned long elapsedMs = millis() - _sessionStartTime;
    
    // Escribir objeto de alerta en JSON
    _alertsFile.printf("    {\n");
    _alertsFile.printf("      \"timestamp_ms\": %lu,\n", elapsedMs);
    _alertsFile.printf("      \"time\": \"%s\",\n", timeStr.c_str());
    _alertsFile.printf("      \"type\": \"%s\",\n", type);
    _alertsFile.printf("      \"message\": \"%s\"\n", message);
    _alertsFile.printf("    }");
    _alertsFile.flush();  // Forzar escritura a la tarjeta
    
    Serial.printf("[Alert] Alerta guardada: %s - %s\n", type, message);
}

// ============================================================================
// taskFunction() - Bucle principal
// ============================================================================
void AlertTask::taskFunction(void* pvParams) {
    SensorData data;

    while (true) {
        // Recibir datos de sensores (solo si hay sesión activa)
        if (_sessionActive && xQueueReceive(_sensorQueue, &data, 0) == pdTRUE) {
            // 1. Evaluar estado global
            int globalState = getGlobalState(data);
            
            // 2. Controlar LED RGB
            setLedState(globalState);
            
            // 3. Si el estado es crítico (rojo)
            if (globalState == 2) {
                // Sonar alarma
                playAlarm();
                
                // Generar y enviar recomendación a DisplayTask
                const char* rec = getRecommendation(data);
                if (rec != nullptr) {
                    Recommendation recom;
                    strncpy(recom.message, rec, sizeof(recom.message) - 1);
                    recom.message[sizeof(recom.message) - 1] = '\0';
                    recom.duration = RECOMMENDATION_DURATION_MS;
                    xQueueSend(_recQueue, &recom, 0);
                    
                    // Determinar el tipo de alerta según la variable crítica
                    const char* type = "";
                    if (getCo2State(data.co2) == 2) type = "CO2";
                    else if (getTempState(data.temperature) == 2) type = "Temperatura";
                    else if (getHumState(data.humidity) == 2) type = "Humedad";
                    else if (getLightState(data.light) == 2) type = "Iluminacion";
                    
                    // Guardar alerta en archivo JSON
                    saveAlertToSD(type, rec);
                }
            }
        }
        
        // Pequeña pausa para no saturar la CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// getCo2State() - Estado del CO2 (0=bueno, 1=regular, 2=malo)
// ============================================================================
int AlertTask::getCo2State(float co2) {
    if (co2 < CO2_GOOD_MAX) return 0;
    if (co2 <= CO2_ACCEPTABLE_MAX) return 1;
    return 2;
}

// ============================================================================
// getTempState() - Estado de la temperatura (0=bueno, 1=regular, 2=malo)
// ============================================================================
int AlertTask::getTempState(float temp) {
    if (temp >= TEMP_GOOD_MIN && temp <= TEMP_GOOD_MAX) return 0;
    if (temp <= TEMP_ACCEPTABLE_MAX) return 1;
    return 2;
}

// ============================================================================
// getHumState() - Estado de la humedad (0=bueno, 1=regular, 2=malo)
// ============================================================================
int AlertTask::getHumState(float hum) {
    if (hum >= HUM_GOOD_MIN && hum <= HUM_GOOD_MAX) return 0;
    if ((hum >= HUM_ACCEPTABLE_MIN1 && hum <= HUM_ACCEPTABLE_MAX1) ||
        (hum >= HUM_ACCEPTABLE_MIN2 && hum <= HUM_ACCEPTABLE_MAX2)) return 1;
    return 2;
}

// ============================================================================
// getLightState() - Estado de la iluminación (0=bueno, 1=regular, 2=malo)
// ============================================================================
int AlertTask::getLightState(float light) {
    if (light < LIGHT_GOOD_MAX) return 0;
    if (light < LIGHT_ACCEPTABLE_MAX) return 1;
    return 2;
}

// ============================================================================
// getGlobalState() - Estado global (el peor de todos)
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
// setLedState() - Controla el LED RGB según estado
// 0 = Verde, 1 = Amarillo, 2 = Rojo
// ============================================================================
void AlertTask::setLedState(int state) {
    // Apagar todos los LEDs primero
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, LOW);
    digitalWrite(LED_YELLOW_PIN, LOW);
    
    switch (state) {
        case 0:  // Óptimo → Verde
            digitalWrite(LED_GREEN_PIN, HIGH);
            break;
        case 1:  // Regular → Amarillo
            digitalWrite(LED_YELLOW_PIN, HIGH);
            break;
        case 2:  // Crítico → Rojo
            digitalWrite(LED_RED_PIN, HIGH);
            break;
    }
}

// ============================================================================
// playAlarm() - Melodía de alarma para el buzzer (pasivo)
// ============================================================================
void AlertTask::playAlarm() {
    // 3 pitidos rápidos de 200 ms con 100 ms de pausa
    for (int i = 0; i < 3; i++) {
        tone(BUZZER_PIN, 2500);  // Frecuencia 2500 Hz (aguda)
        delay(200);
        noTone(BUZZER_PIN);
        delay(100);
    }
    // Pausa más larga y repite una vez más
    delay(300);
    for (int i = 0; i < 3; i++) {
        tone(BUZZER_PIN, 2500);
        delay(200);
        noTone(BUZZER_PIN);
        delay(100);
    }
}

// ============================================================================
// getRecommendation() - Genera recomendación según la variable crítica
// ============================================================================
const char* AlertTask::getRecommendation(SensorData &data) {
    // Prioridad: CO2 > Temperatura > Humedad > Luz
    if (getCo2State(data.co2) == 2) {
        return "Ventilar la habitacion";
    }
    if (getTempState(data.temperature) == 2) {
        if (data.temperature > TEMP_ACCEPTABLE_MAX) {
            return "Reducir temperatura";
        } else {
            return "Aumentar temperatura";
        }
    }
    if (getHumState(data.humidity) == 2) {
        if (data.humidity > HUM_ACCEPTABLE_MAX2) {
            return "Reducir humedad";
        } else {
            return "Aumentar humedad";
        }
    }
    if (getLightState(data.light) == 2) {
        return "Reducir iluminacion";
    }
    return nullptr;
}