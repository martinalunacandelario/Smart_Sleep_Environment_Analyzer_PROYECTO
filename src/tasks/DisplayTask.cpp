// ============================================================================
// DisplayTask.cpp - Implementación de la tarea de visualización OLED
// ============================================================================

#include "DisplayTask.h"
#include "Config.h"

// ============================================================================
// DEFINICIÓN DE CONSTANTES LOCALES
// ============================================================================

// Dimensiones OLED SH1106
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGES 8
#define OLED_CHARS_PER_LINE 21

// Dirección I2C
#define OLED_ADDR 0x3C

// Prioridad de la tarea
#define DISPLAY_TASK_PRIORITY 1

// Tamaño de la pila
#define DISPLAY_TASK_STACK 4096

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================

TaskHandle_t DisplayTask::_taskHandle = nullptr;
QueueHandle_t DisplayTask::_sensorQueue = nullptr;
QueueHandle_t DisplayTask::_alertQueue = nullptr;

SensorData DisplayTask::_currentData;
bool DisplayTask::_showingAlert = false;
unsigned long DisplayTask::_alertEndTime = 0;
char DisplayTask::_lastAlertMessage[64] = "";
bool DisplayTask::_sessionActive = false;
unsigned long DisplayTask::_sessionStart = 0;

// ============================================================================
// start() - Punto de entrada público para iniciar la tarea
// ============================================================================

void DisplayTask::start(QueueHandle_t sensorQueue, QueueHandle_t alertQueue) {
    // Guardar las colas
    _sensorQueue = sensorQueue;
    _alertQueue = alertQueue;
    
    // Inicializar bus I2C para la OLED
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);  // Máxima velocidad para OLED
    
    // Inicializar la pantalla OLED
    initOLED();
    clearDisplay();
    
    // Mostrar mensaje de bienvenida
    writeLine(0, "SmartSleep");
    writeLine(1, "Analyzer v3.0");
    writeLine(2, "Iniciando...");
    writeLine(3, "");
    
    // Crear la tarea FreeRTOS en el Core 1
    xTaskCreatePinnedToCore(
        taskFunction,           // Función de la tarea
        "DisplayTask",          // Nombre
        DISPLAY_TASK_STACK,     // Stack size
        nullptr,                // Parámetros
        DISPLAY_TASK_PRIORITY,  // Prioridad baja (1)
        &_taskHandle,           // Manejador
        1                       // Core 1
    );
}

// ============================================================================
// taskFunction() - Bucle principal de la tarea
// ============================================================================

void DisplayTask::taskFunction(void* pvParams) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    SensorData newData;
    DisplayAlert newAlert;
    
    while (true) {
        // 1. Recibir nuevos datos de sensores (no bloqueante)
        if (xQueueReceive(_sensorQueue, &newData, 0) == pdTRUE) {
            _currentData = newData;
        }
        
        // 2. Recibir alertas (prioridad alta - se muestran inmediatamente)
        if (xQueueReceive(_alertQueue, &newAlert, 0) == pdTRUE) {
            showAlert(newAlert);
        }
        
        // 3. Si la alerta ha expirado, volver a pantalla normal
        if (_showingAlert && millis() > _alertEndTime) {
            _showingAlert = false;
            clearDisplay();
        }
        
        // 4. Actualizar pantalla
        if (!_showingAlert) {
            updateDisplay();
        }
        
        // 5. Esperar hasta el próximo ciclo (1 segundo)
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(DISPLAY_INTERVAL_MS));
    }
}

// ============================================================================
// initOLED() - Inicializa la pantalla OLED SH1106
// ============================================================================

void DisplayTask::initOLED() {
    delay(50);
    
    // Secuencia de inicialización SH1106
    sendCommand(0xAE);  // Display OFF
    
    sendCommand(0xD5);  // Oscillator Frequency
    sendCommand(0x80);
    
    sendCommand(0xA8);  // Multiplex ratio
    sendCommand(0x3F);
    
    sendCommand(0xD3);  // Display offset
    sendCommand(0x00);
    
    sendCommand(0x40);  // Start line
    
    sendCommand(0xAD);  // DC-DC On
    sendCommand(0x8B);
    
    sendCommand(0x8D);  // Charge pump
    sendCommand(0x14);
    
    sendCommand(0x20);  // Memory mode
    sendCommand(0x00);
    
    sendCommand(0x21);  // Column address
    sendCommand(0x00);
    sendCommand(0x7F);
    
    sendCommand(0xB0);  // Page address
    
    sendCommand(0x81);  // Contrast
    sendCommand(0x80);
    
    sendCommand(0xA4);  // Resume to RAM content
    sendCommand(0xA6);  // Normal display
    
    sendCommand(0xAF);  // Display ON
    
    delay(50);
}

// ============================================================================
// sendCommand() - Envía un comando I2C a la OLED
// ============================================================================

void DisplayTask::sendCommand(uint8_t cmd) {
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(0x00);  // Co=0, D/C=0 (comando)
    Wire.write(cmd);
    Wire.endTransmission();
    delay(1);
}

// ============================================================================
// sendData() - Envía datos I2C a la OLED
// ============================================================================

void DisplayTask::sendData(uint8_t* data, int len) {
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(0x40);  // Co=0, D/C=1 (datos)
    for (int i = 0; i < len; i++) {
        Wire.write(data[i]);
    }
    Wire.endTransmission();
}

// ============================================================================
// clearDisplay() - Limpia toda la pantalla OLED
// ============================================================================

void DisplayTask::clearDisplay() {
    // Limpiar todas las páginas (8 páginas para 64 pixeles)
    for (int page = 0; page < OLED_PAGES; page++) {
        sendCommand(0xB0 + page);  // Set page
        sendCommand(0x00);          // Lower column
        sendCommand(0x10);          // Higher column
        
        // Enviar 128 bytes de 0x00 (negro) para cada página
        uint8_t clean[OLED_WIDTH];
        memset(clean, 0, OLED_WIDTH);
        sendData(clean, OLED_WIDTH);
    }
}

// ============================================================================
// writeLine() - Escribe texto en una línea de la pantalla
// ============================================================================

void DisplayTask::writeLine(int line, const char* text) {
    if (line < 0 || line > 3) return;
    
    // Truncar texto a 21 caracteres
    String str = String(text);
    if (str.length() > OLED_CHARS_PER_LINE) {
        str = str.substring(0, OLED_CHARS_PER_LINE);
    }
    
    // Mostrar también por Serial para debug
    Serial.printf("[OLED L%d] %s\n", line, text);
    
    // En implementación real con librería gráfica:
    // display.setCursor(2, line * 16);
    // display.print(str);
}

// ============================================================================
// updateDisplay() - Actualiza la pantalla con datos normales
// ============================================================================

void DisplayTask::updateDisplay() {
    char line0[22];
    char line1[22];
    char line2[22];
    char line3[22];
    
    // Línea 0: CO2 y Temperatura
    snprintf(line0, sizeof(line0), "CO2:%.0f T:%.1f", 
             _currentData.co2, _currentData.temperature);
    
    // Línea 1: Humedad y Luz
    snprintf(line1, sizeof(line1), "H:%.0f%% L:%.0f", 
             _currentData.humidity, _currentData.light);
    
    // Línea 2: Estado ambiental
    snprintf(line2, sizeof(line2), "ESTADO: %s", getStateString(_currentData.state));
    
    // Línea 3: Estado de sesión o IP
    if (_sessionActive) {
        int minutos = (millis() - _sessionStart) / 60000;
        snprintf(line3, sizeof(line3), "SESION: %d min", minutos);
    } else {
        snprintf(line3, sizeof(line3), "SISTEMA: ACTIVO");
    }
    
    // Limpiar y escribir
    clearDisplay();
    writeLine(0, line0);
    writeLine(1, line1);
    writeLine(2, line2);
    writeLine(3, line3);
}

// ============================================================================
// showAlert() - Muestra una alerta temporal en pantalla
// ============================================================================

void DisplayTask::showAlert(const DisplayAlert& alert) {
    // Guardar que estamos mostrando una alerta
    _showingAlert = true;
    _alertEndTime = millis() + ALERT_DURATION_MS;
    strcpy(_lastAlertMessage, alert.message);
    
    // Limpiar pantalla
    clearDisplay();
    
    // Mostrar alerta según el tipo
    if (strcmp(alert.type, "ALERTA") == 0) {
        writeLine(0, "!!! ALERTA CRITICA !!!");
        writeLine(1, alert.message);
        writeLine(2, "");
        writeLine(3, "ATENCION!");
    }
    else if (strcmp(alert.type, "RECOMENDACION") == 0) {
        writeLine(0, "RECOMENDACION:");
        writeLine(1, alert.message);
        writeLine(2, "");
        writeLine(3, "");
    }
    else {
        writeLine(0, "NOTIFICACION:");
        writeLine(1, alert.message);
        writeLine(2, "");
        writeLine(3, "");
    }
    
    Serial.printf("[Display] Alerta: %s - %s\n", alert.type, alert.message);
}

// ============================================================================
// getStateString() - Convierte estado numérico a texto
// ============================================================================

const char* DisplayTask::getStateString(int state) {
    switch (state) {
        case 0: return "OPTIMO";
        case 1: return "REGULAR";
        case 2: return "CRITICO";
        default: return "DESCONOCIDO";
    }
}