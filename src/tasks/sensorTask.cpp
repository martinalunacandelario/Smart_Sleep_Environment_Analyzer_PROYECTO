// ============================================================================
// SensorTask.cpp - Implementación de la tarea de sensores
// ============================================================================

#include "SensorTask.h"

// ============================================================================
// DEFINICIÓN DE CONSTANTES
// ============================================================================

// Pines I2C
#define I2C_SDA 21
#define I2C_SCL 22

// Direcciones I2C
#define SCD41_ADDR 0x62    // Sensor CO2, temperatura, humedad
#define BH1750_ADDR 0x23   // Sensor de luz

// Umbrales para determinar estado ambiental
#define CO2_OPTIMAL 900
#define CO2_ACCEPTABLE 1400
#define CO2_CRITICAL 1800

#define TEMP_MIN_OPTIMAL 18.0
#define TEMP_MAX_OPTIMAL 22.0
#define TEMP_MAX_ACCEPTABLE 25.0
#define TEMP_CRITICAL 28.0

#define HUM_MIN_OPTIMAL 40.0
#define HUM_MAX_OPTIMAL 60.0
#define HUM_MIN_ACCEPTABLE 30.0
#define HUM_MAX_ACCEPTABLE 70.0

#define LIGHT_OPTIMAL 5.0
#define LIGHT_ACCEPTABLE 20.0
#define LIGHT_CRITICAL 50.0

// Intervalo de lectura (milisegundos)
#define SENSOR_INTERVAL_MS 5000

// Prioridad de la tarea
#define SENSOR_TASK_PRIORITY 4

// Tamaño de la pila
#define SENSOR_TASK_STACK 4096

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================

TaskHandle_t SensorTask::_taskHandle = nullptr;
QueueHandle_t SensorTask::_sensorQueue = nullptr;
TwoWire SensorTask::_i2c = TwoWire(0);

// ============================================================================
// start() - Punto de entrada público para iniciar la tarea
// ============================================================================

void SensorTask::start(QueueHandle_t outputQueue) {
    // Guardar la cola
    _sensorQueue = outputQueue;
    
    // Inicializar bus I2C
    _i2c.begin(I2C_SDA, I2C_SCL);
    _i2c.setClock(100000);
    
    // Inicializar sensores
    initSCD41();
    initBH1750();
    
    // Crear la tarea FreeRTOS en el Core 0
    xTaskCreatePinnedToCore(
        taskFunction,           // Función de la tarea
        "SensorTask",           // Nombre
        SENSOR_TASK_STACK,      // Stack size
        nullptr,                // Parámetros
        SENSOR_TASK_PRIORITY,   // Prioridad
        &_taskHandle,           // Manejador
        0                       // Core 0
    );
}

// ============================================================================
// getDataQueue() - Devuelve la cola de datos
// ============================================================================

QueueHandle_t SensorTask::getDataQueue() {
    return _sensorQueue;
}

// ============================================================================
// taskFunction() - Bucle principal de la tarea
// ============================================================================

void SensorTask::taskFunction(void* pvParams) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    SensorData reading;
    
    while (true) {
        // 1. Leer todos los sensores
        readSensors(reading);
        
        // 2. Añadir timestamp
        reading.timestamp = millis();
        reading.valid = true;
        
        // 3. Enviar datos a la cola
        xQueueSend(_sensorQueue, &reading, 0);
        
        // 4. Debug por Serial
        Serial.printf("[Sensor] CO2:%.0f T:%.1f H:%.0f L:%.0f Estado:%d\n",
                      reading.co2, reading.temperature, reading.humidity, 
                      reading.light, reading.state);
        
        // 5. Esperar 5 segundos
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(SENSOR_INTERVAL_MS));
    }
}

// ============================================================================
// initSCD41() - Inicializa el sensor SCD41
// ============================================================================

void SensorTask::initSCD41() {
    // Comando: start periodic measurement (0x21B1)
    _i2c.beginTransmission(SCD41_ADDR);
    _i2c.write(0x21);
    _i2c.write(0xB1);
    _i2c.endTransmission();
    delay(100);
}

// ============================================================================
// initBH1750() - Inicializa el sensor BH1750
// ============================================================================

void SensorTask::initBH1750() {
    // Comando: Power On (0x01)
    _i2c.beginTransmission(BH1750_ADDR);
    _i2c.write(0x01);
    _i2c.endTransmission();
    delay(10);
    
    // Comando: Continuos H-resolution mode (0x10)
    _i2c.beginTransmission(BH1750_ADDR);
    _i2c.write(0x10);
    _i2c.endTransmission();
    delay(10);
}

// ============================================================================
// readSensors() - Lee todos los sensores
// ============================================================================

void SensorTask::readSensors(SensorData& data) {
    data.co2 = readCO2();
    data.temperature = readTemperature();
    data.humidity = readHumidity();
    data.light = readLight();
    data.state = calculateState(data);
}

// ============================================================================
// readCO2() - Lee el valor de CO2 del SCD41
// ============================================================================

float SensorTask::readCO2() {
    // Comando: read measurement (0xEC05)
    _i2c.beginTransmission(SCD41_ADDR);
    _i2c.write(0xEC);
    _i2c.write(0x05);
    _i2c.endTransmission();
    delay(50);
    
    // Solicitar 3 bytes (CO2 high, CO2 low, CRC)
    _i2c.requestFrom(SCD41_ADDR, 3);
    if (_i2c.available() < 3) return -1;
    
    uint8_t high = _i2c.read();
    uint8_t low = _i2c.read();
    _i2c.read();  // Descartar CRC
    
    uint16_t raw = (high << 8) | low;
    return (float)raw;
}

// ============================================================================
// readTemperature() - Lee la temperatura del SCD41
// ============================================================================

float SensorTask::readTemperature() {
    // Comando: read measurement
    _i2c.beginTransmission(SCD41_ADDR);
    _i2c.write(0xEC);
    _i2c.write(0x05);
    _i2c.endTransmission();
    delay(50);
    
    // Solicitar 6 bytes (CO2:3, Temp:3)
    _i2c.requestFrom(SCD41_ADDR, 6);
    if (_i2c.available() < 6) return -1;
    
    // Saltar CO2 (3 bytes)
    _i2c.read(); _i2c.read(); _i2c.read();
    
    // Leer temperatura
    uint8_t high = _i2c.read();
    uint8_t low = _i2c.read();
    
    uint16_t raw = (high << 8) | low;
    // Fórmula: -45 + 175 * (raw / 65535)
    return -45.0f + 175.0f * raw / 65535.0f;
}

// ============================================================================
// readHumidity() - Lee la humedad del SCD41
// ============================================================================

float SensorTask::readHumidity() {
    // Comando: read measurement
    _i2c.beginTransmission(SCD41_ADDR);
    _i2c.write(0xEC);
    _i2c.write(0x05);
    _i2c.endTransmission();
    delay(50);
    
    // Solicitar 9 bytes (CO2:3, Temp:3, Hum:3)
    _i2c.requestFrom(SCD41_ADDR, 9);
    if (_i2c.available() < 9) return -1;
    
    // Saltar CO2 (3) y Temperatura (3)
    for (int i = 0; i < 6; i++) _i2c.read();
    
    // Leer humedad
    uint8_t high = _i2c.read();
    uint8_t low = _i2c.read();
    
    uint16_t raw = (high << 8) | low;
    // Fórmula: 100 * (raw / 65535)
    return 100.0f * raw / 65535.0f;
}

// ============================================================================
// readLight() - Lee la intensidad lumínica del BH1750
// ============================================================================

float SensorTask::readLight() {
    // Solicitar 2 bytes del BH1750
    _i2c.requestFrom(BH1750_ADDR, 2);
    if (_i2c.available() < 2) return -1;
    
    uint8_t high = _i2c.read();
    uint8_t low = _i2c.read();
    
    uint16_t raw = (high << 8) | low;
    // Fórmula: raw / 1.2
    return raw / 1.2f;
}

// ============================================================================
// calculateState() - Determina el estado ambiental
// ============================================================================
// Retorna:
//   0 = OPTIMO   - Todas las variables en rango óptimo
//   1 = REGULAR  - Alguna variable en rango regular
//   2 = CRITICO  - Alguna variable fuera de rango aceptable
// ============================================================================

int SensorTask::calculateState(SensorData& data) {
    // Verificar condiciones CRÍTICAS
    if (data.co2 > CO2_CRITICAL || 
        data.temperature > TEMP_CRITICAL || 
        data.light > LIGHT_CRITICAL) {
        return 2;
    }
    
    // Verificar condiciones REGULARES (aceptables pero no óptimas)
    if (data.co2 > CO2_ACCEPTABLE || 
        data.temperature > TEMP_MAX_ACCEPTABLE ||
        data.humidity < HUM_MIN_ACCEPTABLE || 
        data.humidity > HUM_MAX_ACCEPTABLE ||
        data.light > LIGHT_ACCEPTABLE) {
        return 1;
    }
    
    // Verificar condiciones REGULARES (cerca del límite)
    if (data.co2 > CO2_OPTIMAL ||
        data.temperature < TEMP_MIN_OPTIMAL || 
        data.temperature > TEMP_MAX_OPTIMAL ||
        data.humidity < HUM_MIN_OPTIMAL || 
        data.humidity > HUM_MAX_OPTIMAL ||
        data.light > LIGHT_OPTIMAL) {
        return 1;
    }
    
    // Si no se cumplió ninguna, es ÓPTIMO
    return 0;
}