#include "SensorTask.h"
#include "../../lib/drivers/SCD41.h"
#include "../../lib/drivers/BH1750.h"
#include "../../include/config.h"

struct DisplayCommand {
    bool sessionActive;
};

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================
TaskHandle_t  SensorTask::_taskHandle        = nullptr;
QueueHandle_t SensorTask::_queueForDisplay   = nullptr;  // Cola para DisplayTask
QueueHandle_t SensorTask::_queueForAlert     = nullptr;  // Cola para AlertTask
QueueHandle_t SensorTask::_queueForStorage   = nullptr;  // Cola para StorageTask (NUEVA)
QueueHandle_t SensorTask::_cmdQueue          = nullptr;  // Cola de comandos de sesión

// Objetos driver
static SCD41  scd41(SCD41_ADDR, &Wire);
static BH1750 bh1750(BH1750_ADDR, &Wire);

// ============================================================================
// start() - Guarda las colas, inicializa sensores y crea la tarea
// ============================================================================
void SensorTask::start(QueueHandle_t queueForDisplay, QueueHandle_t queueForAlert, QueueHandle_t queueForStorage, QueueHandle_t cmdQueue) {
    _queueForDisplay = queueForDisplay;  // Cola para DisplayTask
    _queueForAlert   = queueForAlert;    // Cola para AlertTask
    _queueForStorage = queueForStorage;  // Cola para StorageTask (NUEVA)
    _cmdQueue        = cmdQueue;         // Cola de comandos

    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);

    if (!scd41.begin()) {
        Serial.println("[Sensor] Error al iniciar SCD41");
    } else {
        Serial.println("[Sensor] SCD41 OK");
    }

    if (!bh1750.begin()) {
        Serial.println("[Sensor] Error al iniciar BH1750");
    } else {
        Serial.println("[Sensor] BH1750 OK");
    }

    xTaskCreatePinnedToCore(
        taskFunction,
        "SensorTask",
        SENSOR_TASK_STACK,
        nullptr,
        SENSOR_TASK_PRIORITY,
        &_taskHandle,
        0
    );
}

QueueHandle_t SensorTask::getDataQueue() {
    return _queueForDisplay;
}

// ============================================================================
// taskFunction() - Bucle principal
// ============================================================================
void SensorTask::taskFunction(void* pvParams) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    SensorData data;
    DisplayCommand cmd;
    bool sessionActive = false;

    Serial.println("[Sensor] Esperando inicio de sesion...");

    while (true) {
        // Comprobar comandos de sesión
        if (xQueueReceive(_cmdQueue, &cmd, 0) == pdTRUE) {
            sessionActive = cmd.sessionActive;
            if (sessionActive) {
                Serial.println("[Sensor] Sesion iniciada, comenzando lecturas");
                lastWakeTime = xTaskGetTickCount();
            } else {
                Serial.println("[Sensor] Sesion finalizada, pausando lecturas");
            }
        }

        // Solo leer si sesión activa
        if (sessionActive) {
            if (readSensors(data)) {
                data.timestamp = millis();

                // Enviar a las TRES colas de datos
                xQueueSend(_queueForDisplay, &data, 0);  // Para DisplayTask
                xQueueSend(_queueForAlert,   &data, 0);  // Para AlertTask
                xQueueSend(_queueForStorage, &data, 0);  // Para StorageTask (NUEVA)

                Serial.printf("[Sensor] CO2:%.0f ppm T:%.1f C H:%.1f%% Luz:%.0f lux\n",
                              data.co2, data.temperature, data.humidity, data.light);
            } else {
                Serial.println("[Sensor] Error al leer algun sensor");
            }

            vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(SENSOR_INTERVAL_MS));
        } else {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

bool SensorTask::readSensors(SensorData &data) {
    bool ok = true;
    uint16_t co2_raw;
    float temp, hum;

    if (scd41.readMeasurement(co2_raw, temp, hum)) {
        data.co2         = co2_raw;
        data.temperature = temp;
        data.humidity    = hum;
    } else {
        data.co2         = -1;
        data.temperature = -999;
        data.humidity    = -1;
        ok = false;
    }

    float lux = bh1750.readLight();
    if (lux >= 0) {
        data.light = lux;
    } else {
        data.light = -1;
        ok = false;
    }

    return ok;
}