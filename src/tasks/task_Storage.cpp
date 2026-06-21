#include "task_Storage.h"
#include "../../include/config.h"
#include "../../lib/drivers/NTPManager.h"  // <-- CAMBIADO
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
unsigned long StorageTask::_sessionCounter = 0;

// ============================================================================
// start() - Inicializa la microSD y crea la tarea
// ============================================================================
void StorageTask::start(QueueHandle_t sensorQueue, QueueHandle_t cmdQueue) {
    _sensorQueue = sensorQueue;
    _cmdQueue = cmdQueue;
    
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
    
    if (!initSD()) {
        Serial.println("[Storage] ERROR: No se pudo inicializar la tarjeta SD");
    } else {
        Serial.println("[Storage] Tarjeta SD inicializada correctamente");
        readSessionCounter();
    }
    
    xTaskCreatePinnedToCore(
        taskFunction,
        "task_Storage",
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
    if (!SD.begin(SD_CS)) {
        Serial.println("[Storage] Error al montar la tarjeta SD");
        return false;
    }
    
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("[Storage] No se detectó tarjeta SD");
        return false;
    }
    
    Serial.print("[Storage] Tarjeta SD tipo: ");
    switch (cardType) {
        case CARD_MMC: Serial.println("MMC"); break;
        case CARD_SD: Serial.println("SDSC"); break;
        case CARD_SDHC: Serial.println("SDHC"); break;
        default: Serial.println("DESCONOCIDO");
    }
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("[Storage] Tamaño: %llu MB\n", cardSize);
    
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
// getCurrentDateTime() - Obtiene fecha y hora actual (NTP o millis)
// ============================================================================
String StorageTask::getCurrentDateTime() {
    if (NTPManager::isTimeSynced()) {
        String date = NTPManager::getCurrentDate();
        String time = NTPManager::getCurrentTime();
        date.replace("-", "");
        time.replace(":", "");
        return date + "_" + time;
    }
    return String(millis());
}

// ============================================================================
// generateFileName() - Genera nombre de archivo
// ============================================================================
String StorageTask::generateFileName() {
    char filename[128];
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
    
    if (SD.exists(_currentFileName)) {
        Serial.printf("[Storage] El archivo ya existe: %s\n", _currentFileName.c_str());
        return false;
    }
    
    _currentSessionFile = SD.open(_currentFileName, FILE_WRITE);
    if (!_currentSessionFile) {
        Serial.printf("[Storage] Error al crear archivo: %s\n", _currentFileName.c_str());
        return false;
    }
    
    _currentSessionFile.println("# Sesion de monitoreo ambiental");
    _currentSessionFile.printf("# Numero de sesion: %lu\n", _sessionCounter);
    
    if (NTPManager::isTimeSynced()) {
        _currentSessionFile.printf("# Fecha: %s\n", NTPManager::getCurrentDate().c_str());
        _currentSessionFile.printf("# Hora de inicio: %s\n", NTPManager::getCurrentTime().c_str());
        _currentSessionFile.printf("# Timestamp de inicio (epoch): %lu\n", NTPManager::getCurrentEpoch());
    } else {
        _currentSessionFile.printf("# Fecha/Hora inicio: %s\n", getCurrentDateTime().c_str());
    }
    
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
        unsigned long duration = (millis() - _sessionStartTime) / 1000;
        
        _currentSessionFile.printf("\n# Fin de sesion\n");
        _currentSessionFile.printf("# Duracion: %lu segundos (%.1f minutos)\n", duration, duration / 60.0);
        
        if (NTPManager::isTimeSynced()) {
            _currentSessionFile.printf("# Hora de fin: %s\n", NTPManager::getCurrentTime().c_str());
            _currentSessionFile.printf("# Timestamp de fin (epoch): %lu\n", NTPManager::getCurrentEpoch());
        } else {
            _currentSessionFile.printf("# Hora de fin: %s\n", getCurrentDateTime().c_str());
        }
        
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
    
    unsigned long relativeTime = data.timestamp - _sessionStartTime;
    
    _currentSessionFile.printf("%lu,%.0f,%.1f,%.0f,%.0f\n",
                               relativeTime,
                               data.co2,
                               data.temperature,
                               data.humidity,
                               data.light);
    
    _currentSessionFile.flush();
}

// ============================================================================
// taskFunction() - Bucle principal de la tarea
// ============================================================================
void StorageTask::taskFunction(void* pvParams) {
    SensorData newData;
    DisplayCommand cmd;
    
    while (true) {
        if (xQueueReceive(_sensorQueue, &newData, 0) == pdTRUE) {
            if (_sessionActive && _currentSessionFile) {
                writeDataToSD(newData);
            }
        }
        
        if (xQueueReceive(_cmdQueue, &cmd, 0) == pdTRUE) {
            if (cmd.sessionActive && !_sessionActive) {
                _sessionActive = true;
                _sessionStartTime = millis();
                _sessionCounter++;
                saveSessionCounter();
                if (!createSessionFile()) {
                    Serial.println("[Storage] Error al crear archivo para la sesión");
                } else {
                    Serial.printf("[Storage] Sesión #%lu iniciada\n", _sessionCounter);
                }
            } 
            else if (!cmd.sessionActive && _sessionActive) {
                _sessionActive = false;
                closeSessionFile();
                Serial.printf("[Storage] Sesión #%lu finalizada\n", _sessionCounter);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ============================================================================
// getSessionCounterPtr() - Devuelve puntero al contador de sesiones
// ============================================================================
unsigned long* StorageTask::getSessionCounterPtr() {
    return &_sessionCounter;
}