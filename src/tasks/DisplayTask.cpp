#include "DisplayTask.h"
#include "../../include/config.h"
#include "../../lib/drivers/NTPManager.h"  // <-- CAMBIADO
#include <U8g2lib.h>
#include <Wire.h>

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================
TaskHandle_t DisplayTask::_taskHandle = nullptr;
QueueHandle_t DisplayTask::_sensorQueue = nullptr;
QueueHandle_t DisplayTask::_cmdQueue = nullptr;
QueueHandle_t DisplayTask::_recQueue = nullptr;

U8G2_SH1106_128X64_NONAME_F_HW_I2C DisplayTask::_display(U8G2_R0, U8X8_PIN_NONE);

SensorData DisplayTask::_currentData = {0};
bool DisplayTask::_sessionActive = false;
unsigned long DisplayTask::_sessionEndTime = 0;
bool DisplayTask::_displayOn = true;

bool DisplayTask::_showingRec = false;
unsigned long DisplayTask::_recEndTime = 0;
char DisplayTask::_currentRec[64] = "";

// ============================================================================
// start() - Inicializa la pantalla y crea la tarea
// ============================================================================
void DisplayTask::start(QueueHandle_t sensorQueue, QueueHandle_t cmdQueue, QueueHandle_t recQueue) {
    _sensorQueue = sensorQueue;
    _cmdQueue = cmdQueue;
    _recQueue = recQueue;

    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);

    _display.begin();
    _display.setFont(u8g2_font_6x10_tf);
    _display.setFlipMode(0);
    _display.setPowerSave(0);
    _displayOn = true;

    _display.clearBuffer();
    _display.setCursor(5, 20);
    _display.print("SmartSleep");
    _display.setCursor(5, 40);
    _display.print("Iniciando...");
    _display.sendBuffer();
    delay(2000);

    xTaskCreatePinnedToCore(
        taskFunction,
        "DisplayTask",
        DISPLAY_TASK_STACK,
        nullptr,
        DISPLAY_TASK_PRIORITY,
        &_taskHandle,
        1
    );
}

// ============================================================================
// taskFunction() - Bucle principal
// ============================================================================
void DisplayTask::taskFunction(void* pvParams) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    SensorData newData;
    DisplayCommand cmd;
    Recommendation rec;

    while (true) {
        if (xQueueReceive(_sensorQueue, &newData, 0) == pdTRUE) {
            _currentData = newData;
        }

        if (xQueueReceive(_cmdQueue, &cmd, 0) == pdTRUE) {
            _sessionActive = cmd.sessionActive;
            if (!_sessionActive) {
                _sessionEndTime = millis() + DISPLAY_POST_SESSION_DURATION_MS;
            } else {
                if (!_displayOn) {
                    _display.setPowerSave(0);
                    _displayOn = true;
                }
                _sessionEndTime = 0;
                updateDisplay();
            }
        }

        if (xQueueReceive(_recQueue, &rec, 0) == pdTRUE) {
            _showingRec = true;
            _recEndTime = millis() + (rec.duration > 0 ? rec.duration : RECOMMENDATION_DURATION_MS);
            strncpy(_currentRec, rec.message, 63);
            _currentRec[63] = '\0';
        }

        if (!_sessionActive && _sessionEndTime != 0 && millis() > _sessionEndTime) {
            if (_displayOn) {
                _display.setPowerSave(1);
                _displayOn = false;
            }
            _sessionEndTime = 0;
        }

        if (_displayOn) {
            updateDisplay();
        }

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(DISPLAY_INTERVAL_MS));
    }
}

// ============================================================================
// updateDisplay() - Dibuja todo en buffer y lo vuelca de golpe
// ============================================================================
void DisplayTask::updateDisplay() {
    char buf[32];

    _display.clearBuffer();

    // ================================================================
    // HORA REAL desde NTP (o fallback a millis)
    // ================================================================
    if (NTPManager::isTimeSynced()) {
        String timeStr = NTPManager::getCurrentTimeHM();
        _display.setCursor(85, 9);
        _display.print(timeStr);
    } else {
        unsigned long secs = millis() / 1000;
        snprintf(buf, sizeof(buf), "%02d:%02d", (int)(secs / 3600) % 24, (int)(secs / 60) % 60);
        _display.setCursor(85, 9);
        _display.print(buf);
    }

    _display.setCursor(5, 19);
    _display.print("Estado: ");
    _display.print(getQualityString());

    snprintf(buf, sizeof(buf), "CO2: %.0f ppm", _currentData.co2);
    _display.setCursor(5, 29);
    _display.print(buf);

    snprintf(buf, sizeof(buf), "Temp: %.1f C", _currentData.temperature);
    _display.setCursor(5, 39);
    _display.print(buf);

    snprintf(buf, sizeof(buf), "Hum: %.0f %%", _currentData.humidity);
    _display.setCursor(5, 49);
    _display.print(buf);

    if (_showingRec && millis() < _recEndTime) {
        snprintf(buf, sizeof(buf), "%.21s", _currentRec);
        _display.setCursor(5, 59);
        _display.print(buf);
    } else {
        _showingRec = false;
        snprintf(buf, sizeof(buf), "Lux: %.0f   S:%s", _currentData.light, _sessionActive ? "ON " : "OFF");
        _display.setCursor(5, 59);
        _display.print(buf);
    }

    _display.sendBuffer();
}

// ============================================================================
// getQualityString() - Evalúa umbrales y devuelve texto de calidad
// ============================================================================
const char* DisplayTask::getQualityString() {
    bool malo = false;
    bool regular = false;

    if (_currentData.co2 > CO2_ACCEPTABLE_MAX) malo = true;
    else if (_currentData.co2 > CO2_GOOD_MAX) regular = true;

    if (_currentData.temperature < TEMP_GOOD_MIN || _currentData.temperature > TEMP_ACCEPTABLE_MAX) malo = true;
    else if (_currentData.temperature > TEMP_GOOD_MAX) regular = true;

    if (_currentData.humidity < HUM_ACCEPTABLE_MIN1 || _currentData.humidity > HUM_ACCEPTABLE_MAX2) malo = true;
    else if ((_currentData.humidity >= HUM_ACCEPTABLE_MIN1 && _currentData.humidity < HUM_GOOD_MIN) ||
             (_currentData.humidity > HUM_GOOD_MAX && _currentData.humidity <= HUM_ACCEPTABLE_MAX2)) regular = true;

    if (_currentData.light >= LIGHT_ACCEPTABLE_MAX) malo = true;
    else if (_currentData.light >= LIGHT_GOOD_MAX) regular = true;

    if (malo) return "MALO";
    if (regular) return "REGULAR";
    return "BUENO";
}