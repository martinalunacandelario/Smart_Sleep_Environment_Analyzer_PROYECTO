#include "StorageTask.h"
#include "../../include/config.h"
#include <SPI.h>
#include <SD.h>

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================
TaskHandle_t StorageTask::_taskHandle = nullptr;
QueueHandle_t StorageTask::_sensorQueue = nullptr;
QueueHandle_t StorageTask::_cmdQueue = nullptr;

bool StorageTask::_sessionActive = false;
File StorageTask::_currentSessionFile;
String StorageTask::_currentFileName = "";
unsigned long StorageTask::_sessionStartTime = 0;
unsigned long StorageTask::_sessionCounter = 0;  // Contador de sesiones

// ============================================================================
// start() - Inicializa la microSD y crea la tarea
// ============================================================================
void StorageTask::start(QueueHandle_t sensorQueue, QueueHandle_t cmdQueue) {
    _sensorQueue = sensorQueue;
    _cmdQueue = cmdQueue;
    
    // Inicializar bus SPI con los pines definidos en config.h
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
    
    // Inicializar la tarjeta SD
    if (!initSD()) {
        Serial.println("[Storage] ERROR: No se pudo inicializar la tarjeta SD");
    } else {
        Serial.println("[Storage] Tarjeta SD inicializada correctamente");
        // Leer el contador de sesiones guardado
        readSessionCounter();
    }
    
    // Crear tarea FreeRTOS en el núcleo 0 (prioridad baja)
    xTaskCreatePinnedToCore(
        taskFunction,
        "StorageTask",
        STORAGE_TASK_STACK,
        nullptr,
        STORAGE_TASK_PRIORITY,
        &_taskHandle,
        0
    );
}

// ============================================================================
// initSD() - Inicializa la tarjeta microSD
// ============================================================================
bool StorageTask::initSD() {
    // Intentar montar la tarjeta SD
    if (!SD.begin(SD_CS)) {
        Serial.println("[Storage] Error al montar la tarjeta SD");
        return false;
    }
    
    // Verificar tipo de tarjeta
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("[Storage] No se detectó tarjeta SD");
        return false;
    }
    
    // Mostrar tipo de tarjeta por Serial
    Serial.print("[Storage] Tarjeta SD tipo: ");
    switch (cardType) {
        case CARD_MMC: Serial.println("MMC"); break;
        case CARD_SD: Serial.println("SDSC"); break;
        case CARD_SDHC: Serial.println("SDHC"); break;
        default: Serial.println("DESCONOCIDO");
    }
    
    // Mostrar tamaño de la tarjeta
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("[Storage] Tamaño: %llu MB\n", cardSize);
    
    // Crear directorio base si no existe
    if (!SD.exists(SD_BASE_PATH)) {
        if (SD.mkdir(SD_BASE_PATH)) {
            Serial.printf("[Storage] Directorio creado: %s\n", SD_BASE_PATH);
        } else {
            Serial.printf("[Storage] Error al crear directorio: %s\n", SD_BASE_PATH);
        }
    }
    
    return true;
}

// ============================================================================
// readSessionCounter() - Lee el contador de sesiones desde la SD
// ============================================================================
void StorageTask::readSessionCounter() {
    char counterPath[64];
    snprintf(counterPath, sizeof(counterPath), "%s/counter.txt", SD_BASE_PATH);
    
    if (SD.exists(counterPath)) {
        File counterFile = SD.open(counterPath, FILE_READ);
        if (counterFile) {
            String content = counterFile.readString();
            _sessionCounter = content.toInt();
            counterFile.close();
            Serial.printf("[Storage] Contador de sesiones cargado: %lu\n", _sessionCounter);
        }
    } else {
        _sessionCounter = 0;
        Serial.println("[Storage] Contador de sesiones inicializado a 0");
    }
}

// ============================================================================
// saveSessionCounter() - Guarda el contador de sesiones en la SD
// ============================================================================
void StorageTask::saveSessionCounter() {
    char counterPath[64];
    snprintf(counterPath, sizeof(counterPath), "%s/counter.txt", SD_BASE_PATH);
    
    File counterFile = SD.open(counterPath, FILE_WRITE);
    if (counterFile) {
        counterFile.printf("%lu\n", _sessionCounter);
        counterFile.close();
        Serial.printf("[Storage] Contador de sesiones guardado: %lu\n", _sessionCounter);
    }
}

// ============================================================================
// getCurrentDateTime() - Obtiene fecha y hora actual
// ============================================================================
String StorageTask::getCurrentDateTime() {
    // TODO: Cuando tengamos NTP, usar hora real
    // Por ahora, usamos el timestamp de millis() como identificador único
    return String(millis());
}

// ============================================================================
// generateFileName() - Genera nombre de archivo con número de sesión y fecha
// ============================================================================
String StorageTask::generateFileName() {
    char filename[128];
    // Formato: /sessions/session_XXX_YYYYMMDD_HHMMSS.csv
    snprintf(filename, sizeof(filename), "%s/%s%03lu_%s.csv", 
             SD_BASE_PATH, 
             SD_FILENAME_PREFIX, 
             _sessionCounter,
             getCurrentDateTime().c_str());
    return String(filename);
}

// ============================================================================
// createSessionFile() - Crea un nuevo archivo CSV para la sesión
// ============================================================================
bool StorageTask::createSessionFile() {
    _currentFileName = generateFileName();
    
    // Verificar si el archivo ya existe (no debería ocurrir)
    if (SD.exists(_currentFileName)) {
        Serial.printf("[Storage] El archivo ya existe: %s\n", _currentFileName.c_str());
        return false;
    }
    
    // Crear y abrir archivo
    _currentSessionFile = SD.open(_currentFileName, FILE_WRITE);
    if (!_currentSessionFile) {
        Serial.printf("[Storage] Error al crear archivo: %s\n", _currentFileName.c_str());
        return false;
    }
    
    // Escribir cabecera CSV con información de la sesión
    _currentSessionFile.println("# Sesion de monitoreo ambiental");
    _currentSessionFile.printf("# Numero de sesion: %lu\n", _sessionCounter);
    _currentSessionFile.printf("# Fecha/Hora inicio: %s\n", getCurrentDateTime().c_str());
    _currentSessionFile.println("# Columnas: timestamp_ms,co2_ppm,temp_c,hum_percent,light_lux");
    _currentSessionFile.println("timestamp_ms,co2_ppm,temp_c,hum_percent,light_lux");
    _currentSessionFile.flush();
    
    Serial.printf("[Storage] Archivo creado: %s\n", _currentFileName.c_str());
    return true;
}

// ============================================================================
// closeSessionFile() - Cierra el archivo de la sesión actual
// ============================================================================
void StorageTask::closeSessionFile() {
    if (_currentSessionFile) {
        // Añadir información de finalización
        unsigned long duration = (millis() - _sessionStartTime) / 1000; // duración en segundos
        _currentSessionFile.printf("\n# Fin de sesion\n");
        _currentSessionFile.printf("# Duracion: %lu segundos (%.1f minutos)\n", duration, duration / 60.0);
        _currentSessionFile.printf("# Hora de fin: %s\n", getCurrentDateTime().c_str());
        
        _currentSessionFile.close();
        Serial.printf("[Storage] Archivo cerrado: %s\n", _currentFileName.c_str());
    }
}

// ============================================================================
// writeDataToSD() - Escribe una lectura de sensores en el archivo CSV
// ============================================================================
void StorageTask::writeDataToSD(const SensorData &data) {
    if (!_currentSessionFile) {
        Serial.println("[Storage] Error: Archivo no abierto");
        return;
    }
    
    // Calcular timestamp relativo desde inicio de sesión (ms)
    unsigned long relativeTime = data.timestamp - _sessionStartTime;
    
    // Escribir línea CSV: timestamp_ms,co2_ppm,temp_c,hum_percent,light_lux
    _currentSessionFile.printf("%lu,%.0f,%.1f,%.0f,%.0f\n",
                               relativeTime,
                               data.co2,
                               data.temperature,
                               data.humidity,
                               data.light);
    
    // Forzar escritura a la tarjeta (no solo buffer)
    _currentSessionFile.flush();
}

// ============================================================================
// taskFunction() - Bucle principal de la tarea
// ============================================================================
void StorageTask::taskFunction(void* pvParams) {
    SensorData newData;
    DisplayCommand cmd;
    
    while (true) {
        // 1. Recibir nuevos datos de sensores (no bloqueante)
        if (xQueueReceive(_sensorQueue, &newData, 0) == pdTRUE) {
            if (_sessionActive && _currentSessionFile) {
                writeDataToSD(newData);
            }
        }
        
        // 2. Recibir comandos de sesión (inicio/fin)
        if (xQueueReceive(_cmdQueue, &cmd, 0) == pdTRUE) {
            if (cmd.sessionActive && !_sessionActive) {
                // INICIO DE SESIÓN
                _sessionActive = true;
                _sessionStartTime = millis();
                _sessionCounter++;                    // Incrementar contador de sesiones
                saveSessionCounter();                  // Guardar en SD
                if (!createSessionFile()) {
                    Serial.println("[Storage] Error al crear archivo para la sesión");
                } else {
                    Serial.printf("[Storage] Sesión #%lu iniciada\n", _sessionCounter);
                }
            } 
            else if (!cmd.sessionActive && _sessionActive) {
                // FIN DE SESIÓN
                _sessionActive = false;
                closeSessionFile();
                Serial.printf("[Storage] Sesión #%lu finalizada\n", _sessionCounter);
            }
        }
        
        // 3. Pequeña pausa para no saturar la CPU (prioridad baja)
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ============================================================================
// getSessionCounterPtr() - Devuelve puntero al contador de sesiones
// ============================================================================
// Esta función permite que otras tareas (AlertTask, AnalysisTask) accedan al
// mismo contador de sesiones y así nombrar sus archivos con el mismo número:
//   - StorageTask: session_001.csv
//   - AlertTask:   session_001_alerts.json
//   - AnalysisTask: session_001_stats.json
// ============================================================================
unsigned long* StorageTask::getSessionCounterPtr() {
    return &_sessionCounter;
}