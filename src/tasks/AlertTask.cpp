#include "AlertTask.h"
#include "../../include/config.h"

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================
TaskHandle_t  AlertTask::_taskHandle   = nullptr;
QueueHandle_t AlertTask::_sensorQueue  = nullptr;
QueueHandle_t AlertTask::_recQueue     = nullptr;

// ============================================================================
// start() - Inicializa pines y crea la tarea
// ============================================================================
void AlertTask::start(QueueHandle_t sensorQueue, QueueHandle_t recQueue) {
    _sensorQueue = sensorQueue;  // Cola exclusiva de sensores para AlertTask
    _recQueue    = recQueue;     // Cola para enviar recomendaciones a DisplayTask

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
// taskFunction() - Bucle principal
// ============================================================================
void AlertTask::taskFunction(void* pvParams) {
    SensorData data;
    int lastState = -1;  // Último estado global (para detectar cambios)

    while (true) {
        // Intentar recibir datos de sensores sin bloquear la tarea
        if (xQueueReceive(_sensorQueue, &data, 0) == pdTRUE) {

            // 1. Evaluar estado global (0=Verde, 1=Amarillo, 2=Rojo)
            int globalState = getGlobalState(data);

            // 2. Controlar LED RGB según el estado
            setLedState(globalState);

            // 3. Si el estado es crítico (rojo): sonar alarma y enviar recomendación
            if (globalState == 2) {
                playAlarm();  // Buzzer (usa vTaskDelay internamente, no bloquea el núcleo)

                // Generar y enviar recomendación a DisplayTask
                const char* rec = getRecommendation(data);
                if (rec != nullptr) {
                    Recommendation recom;
                    strncpy(recom.message, rec, sizeof(recom.message) - 1);
                    recom.message[sizeof(recom.message) - 1] = '\0';
                    recom.duration = RECOMMENDATION_DURATION_MS;
                    xQueueSend(_recQueue, &recom, 0);
                }
            }

            // Depuración: mostrar en serie si el estado cambió
            if (globalState != lastState) {
                const char* estadoTexto;
                switch (globalState) {
                    case 0:  estadoTexto = "BUENO (Verde)";      break;
                    case 1:  estadoTexto = "REGULAR (Amarillo)"; break;
                    case 2:  estadoTexto = "MALO (Rojo)";        break;
                    default: estadoTexto = "DESCONOCIDO";        break;
                }
                Serial.printf("[Alert] Estado cambiado a: %s\n", estadoTexto);
                lastState = globalState;
            }
        }

        // Pequeña pausa para no saturar la CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// getCo2State() - 0=bueno, 1=regular, 2=malo
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
    // Apagar todos los LEDs primero
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
// playAlarm() - Melodía: intro simpática ascendente + alerta urgente + cierre
// Estructura: subida Do→Mi→Sol→Do (videojuego) → dos pares urgentes → nota final
// Usa vTaskDelay para no bloquear otras tareas FreeRTOS del mismo núcleo
// ============================================================================
void AlertTask::playAlarm() {
    // --- Intro ascendente (estilo videojuego) ---
    // Sube de Do a Do una octava arriba, dando un toque simpático antes de la alerta
    tone(BUZZER_PIN, 523);  vTaskDelay(pdMS_TO_TICKS(150));  // Do5
    noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(40));
    tone(BUZZER_PIN, 659);  vTaskDelay(pdMS_TO_TICKS(150));  // Mi5
    noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(40));
    tone(BUZZER_PIN, 784);  vTaskDelay(pdMS_TO_TICKS(150));  // Sol5
    noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(40));
    tone(BUZZER_PIN, 1047); vTaskDelay(pdMS_TO_TICKS(300));  // Do6 (nota larga, punto de tensión)
    noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(150));  // Pausa antes de la alerta

    // --- Alerta urgente (dos pares de pitidos cortos y agudos) ---
    // Alterna entre dos frecuencias altas para dar sensación de urgencia
    for (int i = 0; i < 2; i++) {
        tone(BUZZER_PIN, 1800); vTaskDelay(pdMS_TO_TICKS(180));  // Tono agudo
        noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(60));
        tone(BUZZER_PIN, 2400); vTaskDelay(pdMS_TO_TICKS(180));  // Tono más agudo
        noTone(BUZZER_PIN);     vTaskDelay(pdMS_TO_TICKS(60));
    }

    // --- Cierre: nota larga de resolución ---
    // Baja a una frecuencia media para no acabar de forma brusca
    vTaskDelay(pdMS_TO_TICKS(100));
    tone(BUZZER_PIN, 880);  vTaskDelay(pdMS_TO_TICKS(400));  // La5 (nota de cierre)
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