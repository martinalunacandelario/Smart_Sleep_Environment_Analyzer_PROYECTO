#include "DisplayTask.h"
#include "../../include/config.h"          // Constantes globales (pines, intervalos, umbrales)
#include <U8g2lib.h>                       // (ya incluido en .h, pero necesario aquí)
#include <Wire.h>                          // I2C

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================
TaskHandle_t DisplayTask::_taskHandle = nullptr;
QueueHandle_t DisplayTask::_sensorQueue = nullptr;
QueueHandle_t DisplayTask::_cmdQueue = nullptr;

// Constructor del objeto U8g2: dirección I2C = 0x3C (por defecto), sin pin reset
U8G2_SH1106_128X64_NONAME_F_HW_I2C DisplayTask::_display(U8G2_R0, /*reset=*/ U8X8_PIN_NONE);

SensorData DisplayTask::_currentData = {0};
bool DisplayTask::_sessionActive = false;
unsigned long DisplayTask::_sessionEndTime = 0;
bool DisplayTask::_displayOn = true;            // La pantalla comienza encendida

// ============================================================================
// start() - Inicializa la pantalla y crea la tarea
// ============================================================================
void DisplayTask::start(QueueHandle_t sensorQueue, QueueHandle_t cmdQueue) {
    _sensorQueue = sensorQueue;
    _cmdQueue = cmdQueue;

    // Inicializar el bus I2C (opcional, U8g2 lo haría igualmente)
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);

    // Inicializar la pantalla OLED
    _display.begin();
    _display.setFont(u8g2_font_ncenB08_tr);   // Fuente pequeña pero legible
    _display.setFlipMode(0);
    _display.clearDisplay();
    _display.setPowerSave(0);                // Asegurar que la pantalla esté encendida
    _displayOn = true;

    // Mensaje de bienvenida breve
    _display.setCursor(20, 20);
    _display.print("SmartSleep");
    _display.setCursor(10, 40);
    _display.print("Iniciando...");
    _display.sendBuffer();
    delay(2000);
    _display.clearDisplay();
    _display.sendBuffer();

    // Crear tarea FreeRTOS fijada al núcleo 1 (el núcleo 0 se reserva para WiFi/BT)
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
// taskFunction() - Bucle principal: recibe datos y comandos, actualiza pantalla
// ============================================================================
void DisplayTask::taskFunction(void* pvParams) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    SensorData newData;
    DisplayCommand cmd;

    while (true) {
        // 1. Recibir nuevos datos de sensores (no bloqueante)
        if (xQueueReceive(_sensorQueue, &newData, 0) == pdTRUE) {
            _currentData = newData;
        }

        // 2. Recibir comandos (inicio/fin sesión) – no bloqueante
        if (xQueueReceive(_cmdQueue, &cmd, 0) == pdTRUE) {
            _sessionActive = cmd.sessionActive;
            if (!_sessionActive) {
                // Finalizar sesión: programar apagado de pantalla después de 1 minuto
                _sessionEndTime = millis() + DISPLAY_POST_SESSION_DURATION_MS;
            } else {
                // Iniciar sesión: encender pantalla si estaba apagada
                if (!_displayOn) {
                    _display.setPowerSave(0);
                    _displayOn = true;
                }
                _sessionEndTime = 0;   // Cancelar cualquier apagado pendiente
                // Forzar actualización inmediata para mostrar "SESION ON"
                updateDisplay();
            }
        }

        // 3. Control de apagado automático tras finalizar sesión
        if (!_sessionActive && _sessionEndTime != 0 && millis() > _sessionEndTime) {
            if (_displayOn) {
                _display.setPowerSave(1);   // Apagar pantalla (modo ahorro)
                _displayOn = false;
            }
            _sessionEndTime = 0;            // No volver a apagar
        }

        // 4. Actualizar la pantalla solo si está encendida
        if (_displayOn) {
            updateDisplay();
        }

        // 5. Esperar hasta el próximo ciclo (1 segundo)
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(DISPLAY_INTERVAL_MS));
    }
}

// ============================================================================
// updateDisplay() - Dibuja la información actual en la OLED
// ============================================================================
void DisplayTask::updateDisplay() {
    _display.clearDisplay();                 // Limpiar el buffer interno

    // Dibujar una línea divisoria superior (opcional, estética)
    _display.drawLine(0, 10, 128, 10);

    // ---- Línea 1: Estado de calidad + hora mock ----
    _display.setCursor(2, 10);
    _display.print("Estado: ");
    _display.print(getQualityString());

    // Hora simulada: tiempo transcurrido desde el inicio (formato HH:MM)
    unsigned long secs = millis() / 1000;
    int minutos = (secs / 60) % 60;
    int horas = (secs / 3600) % 24;
    char timeStr[20];
    sprintf(timeStr, " %02d:%02d", horas, minutos);
    _display.setCursor(80, 10);
    _display.print(timeStr);

    // ---- Línea 2: CO2 ----
    _display.setCursor(2, 28);
    _display.print("CO2: ");
    _display.print(_currentData.co2, 0);
    _display.print(" ppm");

    // ---- Línea 3: Temperatura y humedad ----
    _display.setCursor(2, 44);
    _display.print("Temp: ");
    _display.print(_currentData.temperature, 1);
    _display.print(" C  ");
    _display.print("Hum: ");
    _display.print(_currentData.humidity, 0);
    _display.print("%");

    // ---- Línea 4: Luz y estado de sesión ----
    _display.setCursor(2, 60);
    _display.print("Lux: ");
    _display.print(_currentData.light, 0);
    if (_sessionActive) {
        _display.print("  SESION ON");
    } else {
        _display.print("  SESION OFF");
    }

    _display.sendBuffer();   // Enviar el buffer a la pantalla física
}

// ============================================================================
// getQualityString() - Evalúa los umbrales (config.h) y devuelve el texto
// ============================================================================
const char* DisplayTask::getQualityString() {
    bool malo = false;
    bool regular = false;

    // CO2
    if (_currentData.co2 > CO2_ACCEPTABLE_MAX) malo = true;
    else if (_currentData.co2 > CO2_GOOD_MAX) regular = true;

    // Temperatura
    if (_currentData.temperature < TEMP_GOOD_MIN || _currentData.temperature > TEMP_ACCEPTABLE_MAX) malo = true;
    else if (_currentData.temperature > TEMP_GOOD_MAX) regular = true;

    // Humedad
    if (_currentData.humidity < HUM_ACCEPTABLE_MIN1 || _currentData.humidity > HUM_ACCEPTABLE_MAX2) malo = true;
    else if ((_currentData.humidity >= HUM_ACCEPTABLE_MIN1 && _currentData.humidity < HUM_GOOD_MIN) ||
             (_currentData.humidity > HUM_GOOD_MAX && _currentData.humidity <= HUM_ACCEPTABLE_MAX2)) regular = true;

    // Luz
    if (_currentData.light >= LIGHT_ACCEPTABLE_MAX) malo = true;
    else if (_currentData.light >= LIGHT_GOOD_MAX) regular = true;

    if (malo) return "DESFAVORABLE";
    if (regular) return "ACEPTABLE";
    return "OPTIMO";
}