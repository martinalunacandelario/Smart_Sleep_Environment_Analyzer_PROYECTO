// ============================================================================
// StorageTask.cpp - Implementación de la tarea de almacenamiento
// ============================================================================

#include "StorageTask.h"
#include "Config.h"

// ============================================================================
// DEFINICIÓN DE CONSTANTES LOCALES
// ============================================================================

#define STORAGE_TASK_PRIORITY 0      // MUY BAJA
#define STORAGE_TASK_STACK 8192       // Stack grande para operaciones SD
#define MAX_SESSIONS_IN_MEMORY 50     // Máximo de sesiones en caché
#define MAX_HISTORY_ITEMS 100         // Máximo de items en historial

// Directorios
#define SESSIONS_DIR "/sessions"
#define DATA_DIR "/data"

// Archivos
#define SESSIONS_FILE "/sessions/sessions.csv"
#define RANKING_FILE "/sessions/ranking.csv"
#define CONFIG_FILE "/config.txt"

// Tiempo entre comprobaciones de archivo (1 hora)
#define FILE_CHECK_INTERVAL_MS 3600000

// ============================================================================
// ESTRUCTURA LOCAL PARA HISTORIAL
// ============================================================================

struct HistoryItem {
    char date[20];
    int duration;
    int score;
    float avgCO2;
    float avgTemp;
    float avgHum;
    float avgLight;
    int alerts;
};

static HistoryItem _history[MAX_HISTORY_ITEMS];
static int _historyCount = 0;

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================

TaskHandle_t StorageTask::_taskHandle = nullptr;
QueueHandle_t StorageTask::_commandQueue = nullptr;
bool StorageTask::_sdReady = false;
int StorageTask::_sessionCount = 0;
int StorageTask::_dataPointCount = 0;
String StorageTask::_currentDateFile = "";
unsigned long StorageTask::_lastFileCheck = 0;

// ============================================================================
// start() - Punto de entrada público para iniciar la tarea
// ============================================================================

void StorageTask::start(QueueHandle_t commandQueue) {
    _commandQueue = commandQueue;
    
    // Inicializar SD
    initSD();
    
    // Crear directorios y archivos
    if (_sdReady) {
        createCSVHeaders();
        loadHistory();
    }
    
    // Crear la tarea FreeRTOS en el Core 0
    xTaskCreatePinnedToCore(
        taskFunction,           // Función de la tarea
        "StorageTask",          // Nombre
        STORAGE_TASK_STACK,     // Stack size
        nullptr,                // Parámetros
        STORAGE_TASK_PRIORITY,  // Prioridad MUY BAJA (0)
        &_taskHandle,           // Manejador
        0                       // Core 0
    );
}

// ============================================================================
// isSDReady() - Devuelve si la tarjeta SD está disponible
// ============================================================================

bool StorageTask::isSDReady() {
    return _sdReady;
}

// ============================================================================
// getSessionCount() - Devuelve número de sesiones guardadas
// ============================================================================

int StorageTask::getSessionCount() {
    return _sessionCount;
}

// ============================================================================
// taskFunction() - Bucle principal de la tarea
// ============================================================================

void StorageTask::taskFunction(void* pvParams) {
    StorageCommand cmd;
    
    while (true) {
        // Esperar comandos (bloqueante pero prioridad baja)
        if (xQueueReceive(_commandQueue, &cmd, portMAX_DELAY) == pdTRUE) {
            
            switch (cmd.type) {
                case STORAGE_SAVE_SESSION:
                    saveSession(cmd);
                    break;
                    
                case STORAGE_SAVE_DATAPOINT:
                    saveDataPoint(cmd.dataPoint);
                    break;
                    
                case STORAGE_LOAD_HISTORY:
                    loadHistory();
                    break;
                    
                default:
                    break;
            }
        }
        
        // Comprobar cambio de día para nuevo archivo (cada hora)
        if (_sdReady && (millis() - _lastFileCheck) > FILE_CHECK_INTERVAL_MS) {
            _lastFileCheck = millis();
            String newDateFile = getDateString();
            if (newDateFile != _currentDateFile) {
                _currentDateFile = newDateFile;
                Serial.printf("[Storage] Nuevo archivo diario: %s\n", _currentDateFile.c_str());
            }
        }
        
        // Pequeña pausa para no saturar
        vTaskDelay(10);
    }
}

// ============================================================================
// initSD() - Inicializa la tarjeta microSD
// ============================================================================

void StorageTask::initSD() {
    Serial.print("[Storage] Inicializando microSD... ");
    
    // Inicializar SPI para SD
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
    
    if (SD.begin(SD_CS)) {
        _sdReady = true;
        Serial.println("OK");
        
        // Mostrar información de la tarjeta
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        Serial.printf("  Tamaño: %llu MB\n", cardSize);
        Serial.printf("  Tipo: %s\n", SD.cardType() == CARD_MMC ? "MMC" : 
                      SD.cardType() == CARD_SDSC ? "SDSC" : 
                      SD.cardType() == CARD_SDHC ? "SDHC" : "DESCONOCIDO");
        
    } else {
        _sdReady = false;
        Serial.println("ERROR - Sin tarjeta SD");
    }
}

// ============================================================================
// createCSVHeaders() - Crea archivos CSV con cabeceras
// ============================================================================

void StorageTask::createCSVHeaders() {
    if (!_sdReady) return;
    
    // Crear directorio de sesiones
    if (!SD.exists(SESSIONS_DIR)) {
        SD.mkdir(SESSIONS_DIR);
        Serial.println("[Storage] Directorio /sessions creado");
    }
    
    // Crear directorio de datos
    if (!SD.exists(DATA_DIR)) {
        SD.mkdir(DATA_DIR);
        Serial.println("[Storage] Directorio /data creado");
    }
    
    // Crear archivo de sesiones si no existe
    if (!SD.exists(SESSIONS_FILE)) {
        File f = SD.open(SESSIONS_FILE, FILE_WRITE);
        if (f) {
            f.println("ID,Fecha,Duracion(min),Score,CO2_avg,Temp_avg,Hum_avg,Luz_avg,Alertas,Interpretacion");
            f.close();
            Serial.println("[Storage] Archivo sessions.csv creado");
        }
    }
    
    // Crear archivo de ranking si no existe
    if (!SD.exists(RANKING_FILE)) {
        File f = SD.open(RANKING_FILE, FILE_WRITE);
        if (f) {
            f.println("Rank,ID,Fecha,Score,Duracion");
            f.close();
            Serial.println("[Storage] Archivo ranking.csv creado");
        }
    }
}

// ============================================================================
// saveSession() - Guarda una sesión completa en SD
// ============================================================================

void StorageTask::saveSession(const StorageCommand& cmd) {
    if (!_sdReady) {
        Serial.println("[Storage] Error: No se puede guardar, SD no disponible");
        return;
    }
    
    // Calcular Sleep Score
    int score = 0;
    
    // CO2 (máx 40)
    if (cmd.avgCO2 < 800) score += 40;
    else if (cmd.avgCO2 < 1000) score += 32;
    else if (cmd.avgCO2 < 1400) score += 20;
    else if (cmd.avgCO2 < 1800) score += 10;
    
    // Temperatura (máx 25)
    if (cmd.avgTemp >= 18 && cmd.avgTemp <= 22) score += 25;
    else if (cmd.avgTemp < 24) score += 18;
    else if (cmd.avgTemp < 26) score += 10;
    
    // Humedad (máx 20)
    if (cmd.avgHum >= 40 && cmd.avgHum <= 60) score += 20;
    else if ((cmd.avgHum >= 30 && cmd.avgHum < 40) || (cmd.avgHum > 60 && cmd.avgHum <= 70)) score += 12;
    
    // Luz (máx 15)
    if (cmd.avgLight < 5) score += 15;
    else if (cmd.avgLight < 20) score += 8;
    
    // Interpretación
    String interpretation;
    if (score >= 85) interpretation = "Optimo";
    else if (score >= 70) interpretation = "Bueno";
    else if (score >= 50) interpretation = "Aceptable";
    else if (score >= 30) interpretation = "Desfavorable";
    else interpretation = "Critico";
    
    // Duración en minutos
    int duration = (cmd.sessionEnd - cmd.sessionStart) / 60000;
    
    // Fecha y hora
    String timestamp = getTimestamp();
    
    // Guardar en sessions.csv
    File f = SD.open(SESSIONS_FILE, FILE_APPEND);
    if (f) {
        f.printf("%s,%s,%d,%d,%.0f,%.1f,%.0f,%.0f,%d,%s\n",
                 cmd.sessionId,
                 timestamp.c_str(),
                 duration,
                 score,
                 cmd.avgCO2,
                 cmd.avgTemp,
                 cmd.avgHum,
                 cmd.avgLight,
                 cmd.alertCount,
                 interpretation.c_str());
        f.close();
        
        Serial.printf("[Storage] Sesión %s guardada - Score: %d - Duración: %d min\n", 
                      cmd.sessionId, score, duration);
    } else {
        Serial.println("[Storage] Error al guardar sesión");
    }
    
    // Actualizar ranking
    updateRanking(cmd.sessionId, timestamp.c_str(), score, duration);
    
    // Guardar en caché
    if (_sessionCount < MAX_SESSIONS_IN_MEMORY) {
        _history[_sessionCount].duration = duration;
        _history[_sessionCount].score = score;
        _history[_sessionCount].avgCO2 = cmd.avgCO2;
        _history[_sessionCount].avgTemp = cmd.avgTemp;
        _history[_sessionCount].avgHum = cmd.avgHum;
        _history[_sessionCount].avgLight = cmd.avgLight;
        _history[_sessionCount].alerts = cmd.alertCount;
        strcpy(_history[_sessionCount].date, timestamp.c_str());
        _sessionCount++;
    }
    
    _sessionCount++;
}

// ============================================================================
// saveDataPoint() - Guarda un punto de datos individual
// ============================================================================

void StorageTask::saveDataPoint(const DataPoint& point) {
    if (!_sdReady) return;
    
    // Crear nombre de archivo por fecha
    String filename = String(DATA_DIR) + "/" + getDateString() + ".csv";
    
    // Verificar si es nuevo día
    if (filename != _currentDateFile) {
        _currentDateFile = filename;
        
        // Crear archivo con cabecera si no existe
        if (!SD.exists(filename)) {
            File f = SD.open(filename, FILE_WRITE);
            if (f) {
                f.println("Timestamp,CO2,Temp,Humedad,Luz,Estado");
                f.close();
            }
        }
    }
    
    // Guardar punto de datos
    File f = SD.open(filename, FILE_APPEND);
    if (f) {
        f.printf("%lu,%.0f,%.1f,%.0f,%.0f,%d\n",
                 point.timestamp,
                 point.co2,
                 point.temperature,
                 point.humidity,
                 point.light,
                 point.state);
        f.close();
        
        _dataPointCount++;
        if (_dataPointCount % 100 == 0) {
            Serial.printf("[Storage] %d puntos de datos guardados\n", _dataPointCount);
        }
    }
}

// ============================================================================
// loadHistory() - Carga el historial desde SD a memoria
// ============================================================================

void StorageTask::loadHistory() {
    if (!_sdReady) {
        Serial.println("[Storage] No se puede cargar historial, SD no disponible");
        return;
    }
    
    if (!SD.exists(SESSIONS_FILE)) {
        Serial.println("[Storage] No hay historial previo");
        return;
    }
    
    File f = SD.open(SESSIONS_FILE, FILE_READ);
    if (!f) {
        Serial.println("[Storage] Error al abrir sessions.csv");
        return;
    }
    
    _sessionCount = 0;
    _historyCount = 0;
    
    // Saltar cabecera
    f.readStringUntil('\n');
    
    while (f.available() && _historyCount < MAX_HISTORY_ITEMS) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;
        
        // Parsear CSV (ID,Fecha,Duracion,Score,CO2,Temp,Hum,Luz,Alertas,Interpretacion)
        int comma1 = line.indexOf(',');
        int comma2 = line.indexOf(',', comma1 + 1);
        int comma3 = line.indexOf(',', comma2 + 1);
        int comma4 = line.indexOf(',', comma3 + 1);
        int comma5 = line.indexOf(',', comma4 + 1);
        int comma6 = line.indexOf(',', comma5 + 1);
        int comma7 = line.indexOf(',', comma6 + 1);
        int comma8 = line.indexOf(',', comma7 + 1);
        
        if (comma1 > 0 && comma2 > 0 && comma3 > 0) {
            String id = line.substring(0, comma1);
            String date = line.substring(comma1 + 1, comma2);
            int duration = line.substring(comma2 + 1, comma3).toInt();
            int score = line.substring(comma3 + 1, comma4).toInt();
            float avgCO2 = line.substring(comma4 + 1, comma5).toFloat();
            float avgTemp = line.substring(comma5 + 1, comma6).toFloat();
            float avgHum = line.substring(comma6 + 1, comma7).toFloat();
            float avgLight = line.substring(comma7 + 1, comma8).toFloat();
            int alerts = line.substring(comma8 + 1, line.indexOf(',', comma8 + 1)).toInt();
            
            strcpy(_history[_historyCount].date, date.c_str());
            _history[_historyCount].duration = duration;
            _history[_historyCount].score = score;
            _history[_historyCount].avgCO2 = avgCO2;
            _history[_historyCount].avgTemp = avgTemp;
            _history[_historyCount].avgHum = avgHum;
            _history[_historyCount].avgLight = avgLight;
            _history[_historyCount].alerts = alerts;
            
            _historyCount++;
            _sessionCount++;
        }
    }
    f.close();
    
    Serial.printf("[Storage] Historial cargado: %d sesiones\n", _sessionCount);
}

// ============================================================================
// updateRanking() - Actualiza el archivo de ranking
// ============================================================================

void StorageTask::updateRanking(const char* id, const char* date, int score, int duration) {
    if (!_sdReady) return;
    
    // Leer ranking actual
    struct RankingItem {
        char id[16];
        char date[20];
        int score;
        int duration;
    };
    
    RankingItem items[100];
    int itemCount = 0;
    
    if (SD.exists(RANKING_FILE)) {
        File f = SD.open(RANKING_FILE, FILE_READ);
        if (f) {
            f.readStringUntil('\n'); // Saltar cabecera
            while (f.available() && itemCount < 100) {
                String line = f.readStringUntil('\n');
                line.trim();
                if (line.length() == 0) continue;
                
                int comma1 = line.indexOf(',');
                int comma2 = line.indexOf(',', comma1 + 1);
                int comma3 = line.indexOf(',', comma2 + 1);
                int comma4 = line.indexOf(',', comma3 + 1);
                
                if (comma1 > 0) {
                    strcpy(items[itemCount].id, line.substring(comma1 + 1, comma2).c_str());
                    strcpy(items[itemCount].date, line.substring(comma2 + 1, comma3).c_str());
                    items[itemCount].score = line.substring(comma3 + 1, comma4).toInt();
                    items[itemCount].duration = line.substring(comma4 + 1).toInt();
                    itemCount++;
                }
            }
            f.close();
        }
    }
    
    // Agregar nueva sesión
    strcpy(items[itemCount].id, id);
    strcpy(items[itemCount].date, date);
    items[itemCount].score = score;
    items[itemCount].duration = duration;
    itemCount++;
    
    // Ordenar por score (mayor a menor)
    for (int i = 0; i < itemCount - 1; i++) {
        for (int j = i + 1; j < itemCount; j++) {
            if (items[i].score < items[j].score) {
                RankingItem temp = items[i];
                items[i] = items[j];
                items[j] = temp;
            }
        }
    }
    
    // Guardar ranking actualizado (solo top 10)
    File f = SD.open(RANKING_FILE, FILE_WRITE);
    if (f) {
        f.println("Rank,ID,Fecha,Score,Duracion");
        for (int i = 0; i < min(10, itemCount); i++) {
            f.printf("%d,%s,%s,%d,%d\n", 
                     i + 1, items[i].id, items[i].date, items[i].score, items[i].duration);
        }
        f.close();
        Serial.printf("[Storage] Ranking actualizado - Top score: %d\n", items[0].score);
    }
}

// ============================================================================
// getTimestamp() - Devuelve timestamp formateado
// ============================================================================

String StorageTask::getTimestamp() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char buffer[20];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
        return String(buffer);
    }
    return String(millis() / 1000);
}

// ============================================================================
// getDateString() - Devuelve fecha para nombre de archivo
// ============================================================================

String StorageTask::getDateString() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char buffer[11];
        strftime(buffer, sizeof(buffer), "%Y%m%d", &timeinfo);
        return String(buffer);
    }
    return String(millis() / 86400000);
}