#include "SensorTask.h"                      // Incluye la cabecera de la propia tarea (SensorTask.h)
#include "../../lib/drivers/SCD41.h"         // Incluye el driver SCD41 desde lib/drivers/
#include "../../lib/drivers/BH1750.h"        // Incluye el driver BH1750 desde lib/drivers/
#include "../../include/config.h"            // Incluye la configuración global (pines, intervalos, etc.)

// Variables estáticas de la clase
TaskHandle_t SensorTask::_taskHandle = nullptr;   // Inicializa el manejador de la tarea FreeRTOS a nulo
QueueHandle_t SensorTask::_sensorQueue = nullptr; // Inicializa la cola de datos a nulo

// Objetos driver estáticos (usando los constructores con dirección y bus I2C)
static SCD41 scd41(SCD41_ADDR, &Wire);     // Crea objeto scd41 con dirección 0x62 y usa el bus Wire por defecto
static BH1750 bh1750(BH1750_ADDR, &Wire);  // Crea objeto bh1750 con dirección 0x23 y usa el bus Wire
static TwoWire i2c = TwoWire(0);            // Crea un objeto I2C en el puerto 0 (no se usa realmente, solo reservado)

void SensorTask::start(QueueHandle_t outputQueue) {   // Método estático para iniciar la tarea, recibe una cola
    _sensorQueue = outputQueue;              // Guarda la cola recibida en la variable estática _sensorQueue

    // Inicializar el bus I2C (aunque Wire.begin se llama dentro de los drivers, lo hacemos aquí por seguridad)
    Wire.begin(I2C_SDA, I2C_SCL);            // Inicia el bus I2C con los pines I2C_SDA e I2C_SCL definidos en config.h
    Wire.setClock(100000);                   // Configura la frecuencia del bus I2C a 100 kHz

    // Inicializar SCD41
    if (!scd41.begin()) {                    // Llama a begin() del driver SCD41; si devuelve false (error)
        Serial.println("[Sensor] Error al iniciar SCD41"); // Imprime mensaje de error
    } else {                                 // Si devuelve true (éxito)
        Serial.println("[Sensor] SCD41 OK"); // Imprime confirmación
    }

    // Inicializar BH1750
    if (!bh1750.begin()) {                   // Llama a begin() del driver BH1750; si devuelve false (error)
        Serial.println("[Sensor] Error al iniciar BH1750"); // Imprime mensaje de error
    } else {                                 // Si devuelve true (éxito)
        Serial.println("[Sensor] BH1750 OK"); // Imprime confirmación
    }

    // Crear la tarea FreeRTOS en el núcleo 0
    xTaskCreatePinnedToCore(                 // Función de FreeRTOS para crear una tarea fijada a un núcleo
        taskFunction,                        // Función que ejecutará la tarea (el bucle infinito)
        "SensorTask",                        // Nombre descriptivo de la tarea
        SENSOR_TASK_STACK,                   // Tamaño de la pila (definido en config.h)
        nullptr,                             // Parámetro adicional (ninguno)
        SENSOR_TASK_PRIORITY,                // Prioridad de la tarea (definido en config.h)
        &_taskHandle,                        // Puntero para recibir el manejador de la tarea
        0                                    // Núcleo 0 (el núcleo 1 se usa para WiFi/Bluetooth)
    );
}

QueueHandle_t SensorTask::getDataQueue() {   // Método estático para obtener la cola de datos
    return _sensorQueue;                     // Devuelve la cola donde se publican los datos
}

void SensorTask::taskFunction(void* pvParams) {   // Función principal de la tarea (se ejecuta en su propio hilo)
    TickType_t lastWakeTime = xTaskGetTickCount(); // Obtiene el tiempo actual del sistema (en ticks)
    SensorData data;                               // Variable local para almacenar una lectura

    while (true) {                                 // Bucle infinito de la tarea
        if (readSensors(data)) {                   // Intenta leer todos los sensores; si tiene éxito:
            data.timestamp = millis();             // Añade la marca de tiempo actual (ms desde inicio)
            xQueueSend(_sensorQueue, &data, 0);    // Envía los datos a la cola (sin espera)
            Serial.printf("[Sensor] CO2:%.0f ppm T:%.1f°C H:%.1f%% Luz:%.0f lux\n",
                          data.co2, data.temperature, data.humidity, data.light); // Imprime datos
        } else {                                   // Si falló la lectura de algún sensor
            Serial.println("[Sensor] Error al leer algún sensor"); // Mensaje de error
        }
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(SENSOR_INTERVAL_MS)); // Espera hasta el siguiente ciclo
    }
}

bool SensorTask::readSensors(SensorData &data) {   // Lee todos los sensores y rellena la estructura SensorData
    bool ok = true;                                // Variable que indica si todas las lecturas fueron correctas
    uint16_t co2_raw;                              // Variable para el valor crudo de CO2 (ppm)
    float temp;                                    // Variable para la temperatura (°C)
    float hum;                                     // Variable para la humedad (%)
    if (scd41.readMeasurement(co2_raw, temp, hum)) { // Si la lectura del SCD41 es exitosa
        data.co2 = co2_raw;                        // Almacena el CO2 en la estructura
        data.temperature = temp;                   // Almacena la temperatura
        data.humidity = hum;                       // Almacena la humedad
    } else {                                       // Si falla la lectura del SCD41
        data.co2 = -1;                             // Asigna valor de error
        data.temperature = -999;                   // Asigna valor de error
        data.humidity = -1;                        // Asigna valor de error
        ok = false;                                // Marca que hubo error
    }

    float lux = bh1750.readLight();                // Lee la iluminación del BH1750 (lux)
    if (lux >= 0) {                                // Si la lectura es válida (>=0)
        data.light = lux;                          // Almacena el valor de lux
    } else {                                       // Si la lectura falla (devuelve -1)
        data.light = -1;                           // Asigna valor de error
        ok = false;                                // Marca que hubo error
    }
    return ok;                                     // Devuelve true si todas las lecturas fueron correctas
}