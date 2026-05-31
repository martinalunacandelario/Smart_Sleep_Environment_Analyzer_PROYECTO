#include "DisplayTask.h"
#include "../../include/config.h"          // Constantes globales (pines, intervalos, umbrales)
#include <U8g2lib.h>                       // Librería para pantallas OLED
#include <Wire.h>                          // Comunicación I2C

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================
TaskHandle_t DisplayTask::_taskHandle = nullptr;   // Handle de la tarea FreeRTOS (inicialmente vacío)
QueueHandle_t DisplayTask::_sensorQueue = nullptr; // Cola de entrada de datos de sensores
QueueHandle_t DisplayTask::_cmdQueue = nullptr;    // Cola de entrada de comandos (inicio/fin sesión)

U8G2_SH1106_128X64_NONAME_F_HW_I2C DisplayTask::_display(U8G2_R0, U8X8_PIN_NONE); // Objeto pantalla OLED SH1106 128x64 por I2C, sin pin de reset

SensorData DisplayTask::_currentData = {0};        // Últimos datos recibidos de los sensores (todo a 0 al inicio)
bool DisplayTask::_sessionActive = false;           // Indica si hay una sesión de sueño activa
unsigned long DisplayTask::_sessionEndTime = 0;    // Timestamp en ms cuando debe apagarse la pantalla tras sesión
bool DisplayTask::_displayOn = true;               // Estado actual de la pantalla (true = encendida)

// ============================================================================
// start() - Inicializa la pantalla y crea la tarea
// ============================================================================
void DisplayTask::start(QueueHandle_t sensorQueue, QueueHandle_t cmdQueue) {
    _sensorQueue = sensorQueue;   // Guardar referencia a la cola de sensores
    _cmdQueue = cmdQueue;         // Guardar referencia a la cola de comandos

    Wire.begin(I2C_SDA, I2C_SCL);  // Inicializar bus I2C con los pines definidos en config.h
    Wire.setClock(400000);          // Configurar velocidad I2C a 400 kHz (modo fast)

    _display.begin();                        // Inicializar la pantalla OLED
    _display.setFont(u8g2_font_6x10_tf);    // Fuente de 6x10 píxeles, suficiente para 6 líneas en 64px
    _display.setFlipMode(0);                 // Sin rotación de pantalla
    _display.setPowerSave(0);               // Encender la pantalla (0 = activa, 1 = apagada)
    _displayOn = true;                       // Sincronizar el flag de estado

    // Mostrar pantalla de bienvenida mientras el sistema arranca
    _display.clearBuffer();          // Limpiar el buffer en RAM
    _display.setCursor(5, 20);       // Posición: x=5, y=20
    _display.print("SmartSleep");    // Nombre del proyecto
    _display.setCursor(5, 40);       // Posición: x=5, y=40
    _display.print("Iniciando...");  // Mensaje de arranque
    _display.sendBuffer();           // Volcar buffer a la pantalla física
    delay(2000);                     // Esperar 2 segundos para que se lea el mensaje

    // Crear la tarea FreeRTOS fijada al núcleo 1 (núcleo 0 reservado para WiFi/BT)
    xTaskCreatePinnedToCore(
        taskFunction,           // Función que ejecutará la tarea
        "DisplayTask",          // Nombre identificativo de la tarea
        DISPLAY_TASK_STACK,     // Tamaño del stack en bytes (definido en config.h)
        nullptr,                // Parámetros pasados a la tarea (ninguno)
        DISPLAY_TASK_PRIORITY,  // Prioridad de la tarea (definida en config.h)
        &_taskHandle,           // Handle para poder controlar la tarea después
        1                       // Núcleo donde se ejecuta (núcleo 1)
    );
}

// ============================================================================
// taskFunction() - Bucle principal
// ============================================================================
void DisplayTask::taskFunction(void* pvParams) {
    TickType_t lastWakeTime = xTaskGetTickCount(); // Marca de tiempo para el ciclo periódico exacto
    SensorData newData;   // Buffer temporal para recibir datos de sensores
    DisplayCommand cmd;   // Buffer temporal para recibir comandos

    while (true) {
        // 1. Intentar recibir nuevos datos de sensores sin bloquear la tarea
        if (xQueueReceive(_sensorQueue, &newData, 0) == pdTRUE) {
            _currentData = newData;   // Actualizar los datos actuales si llegó algo nuevo
        }

        // 2. Intentar recibir un comando de inicio o fin de sesión sin bloquear
        if (xQueueReceive(_cmdQueue, &cmd, 0) == pdTRUE) {
            _sessionActive = cmd.sessionActive;   // Actualizar estado de sesión
            if (!_sessionActive) {
                // Sesión terminada: programar apagado de pantalla tras el tiempo definido
                _sessionEndTime = millis() + DISPLAY_POST_SESSION_DURATION_MS;
            } else {
                // Sesión iniciada: encender pantalla si estaba apagada
                if (!_displayOn) {
                    _display.setPowerSave(0);   // Encender pantalla físicamente
                    _displayOn = true;           // Actualizar flag de estado
                }
                _sessionEndTime = 0;    // Cancelar cualquier apagado pendiente
                updateDisplay();        // Forzar actualización inmediata para mostrar el nuevo estado
            }
        }

        // 3. Comprobar si ha llegado el momento de apagar la pantalla tras la sesión
        if (!_sessionActive && _sessionEndTime != 0 && millis() > _sessionEndTime) {
            if (_displayOn) {
                _display.setPowerSave(1);   // Apagar pantalla físicamente (modo ahorro de energía)
                _displayOn = false;          // Actualizar flag de estado
            }
            _sessionEndTime = 0;   // Resetear para no volver a entrar en esta condición
        }

        // 4. Redibujar la pantalla solo si está encendida
        if (_displayOn) {
            updateDisplay();
        }

        // 5. Esperar hasta el siguiente ciclo respetando el intervalo exacto
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(DISPLAY_INTERVAL_MS));
    }
}

// ============================================================================
// updateDisplay() - Dibuja todo en buffer y lo vuelca de golpe (sin parpadeo)
// ============================================================================
void DisplayTask::updateDisplay() {
    char buf[32];   // Buffer auxiliar para formatear cadenas con snprintf

    _display.clearBuffer();   // Limpiar buffer en RAM (la pantalla física no cambia aún)

    // --- Hora en esquina superior derecha (y=9, no baja con el resto) ---
    unsigned long secs = millis() / 1000;   // Convertir milisegundos a segundos totales
    snprintf(buf, sizeof(buf), "%02d:%02d", (int)(secs / 3600) % 24, (int)(secs / 60) % 60); // Formatear HH:MM
    _display.setCursor(85, 9);   // Posición: esquina superior derecha
    _display.print(buf);         // Imprimir la hora en el buffer

    // --- Estado de calidad del ambiente (y=19) ---
    _display.setCursor(5, 19);        // Posición: inicio de línea 2
    _display.print("Estado: ");       // Etiqueta fija
    _display.print(getQualityString()); // Resultado evaluado: "BUENO", "REGULAR" o "MALO"

    // --- Concentración de CO2 (y=29) ---
    snprintf(buf, sizeof(buf), "CO2: %.0f ppm", _currentData.co2); // Formatear sin decimales
    _display.setCursor(5, 29);   // Posición: inicio de línea 3
    _display.print(buf);         // Imprimir CO2 en el buffer

    // --- Temperatura (y=39) ---
    snprintf(buf, sizeof(buf), "Temp: %.1f C", _currentData.temperature); // Formatear con 1 decimal
    _display.setCursor(5, 39);   // Posición: inicio de línea 4
    _display.print(buf);         // Imprimir temperatura en el buffer

    // --- Humedad relativa (y=49) ---
    snprintf(buf, sizeof(buf), "Hum: %.0f %%", _currentData.humidity); // %% produce un % literal en snprintf
    _display.setCursor(5, 49);   // Posición: inicio de línea 5
    _display.print(buf);         // Imprimir humedad en el buffer

    // --- Nivel de luz y estado de sesión (y=59) ---
    snprintf(buf, sizeof(buf), "Lux: %.0f   S:%s", _currentData.light, _sessionActive ? "ON " : "OFF"); // ON/OFF según sesión
    _display.setCursor(5, 59);   // Posición: inicio de línea 6 (límite inferior de la pantalla)
    _display.print(buf);         // Imprimir luz y estado de sesión en el buffer

    _display.sendBuffer();   // Volcar todo el buffer a la pantalla de golpe → sin parpadeo
}

// ============================================================================
// getQualityString() - Evalúa umbrales y devuelve texto de calidad
// ============================================================================
const char* DisplayTask::getQualityString() {
    bool malo = false;     // Flag: algún parámetro supera el umbral crítico
    bool regular = false;  // Flag: algún parámetro supera el umbral aceptable pero no el crítico

    // Evaluar CO2: por encima de ACCEPTABLE_MAX es malo, por encima de GOOD_MAX es regular
    if (_currentData.co2 > CO2_ACCEPTABLE_MAX) malo = true;
    else if (_currentData.co2 > CO2_GOOD_MAX) regular = true;

    // Evaluar temperatura: fuera del rango aceptable es malo, fuera del bueno es regular
    if (_currentData.temperature < TEMP_GOOD_MIN || _currentData.temperature > TEMP_ACCEPTABLE_MAX) malo = true;
    else if (_currentData.temperature > TEMP_GOOD_MAX) regular = true;

    // Evaluar humedad: fuera del rango aceptable es malo, en zona intermedia es regular
    if (_currentData.humidity < HUM_ACCEPTABLE_MIN1 || _currentData.humidity > HUM_ACCEPTABLE_MAX2) malo = true;
    else if ((_currentData.humidity >= HUM_ACCEPTABLE_MIN1 && _currentData.humidity < HUM_GOOD_MIN) ||
             (_currentData.humidity > HUM_GOOD_MAX && _currentData.humidity <= HUM_ACCEPTABLE_MAX2)) regular = true;

    // Evaluar luz: por encima de ACCEPTABLE_MAX es malo, por encima de GOOD_MAX es regular
    if (_currentData.light >= LIGHT_ACCEPTABLE_MAX) malo = true;
    else if (_currentData.light >= LIGHT_GOOD_MAX) regular = true;

    // Devolver el peor estado encontrado (malo tiene prioridad sobre regular)
    if (malo) return "MALO";
    if (regular) return "REGULAR";
    return "BUENO";   // Si ningún flag se activó, la calidad es buena
}