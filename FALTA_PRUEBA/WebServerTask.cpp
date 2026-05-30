// ============================================================================
// WebServerTask.cpp - Implementación de la tarea del servidor web
// ============================================================================

#include "WebServerTask.h"
#include "Config.h"

// ============================================================================
// DEFINICIÓN DE CONSTANTES LOCALES
// ============================================================================

#define HTTP_PORT 80
#define WIFI_RECONNECT_INTERVAL_MS 30000
#define WIFI_TIMEOUT_MS 10000
#define WEBSERVER_TASK_PRIORITY 2
#define WEBSERVER_TASK_STACK 8192

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================

TaskHandle_t WebServerTask::_taskHandle = nullptr;
WebServer* WebServerTask::_server = nullptr;
QueueHandle_t WebServerTask::_storageQueue = nullptr;
QueueHandle_t WebServerTask::_sensorQueue = nullptr;

SensorData WebServerTask::_currentData;
bool WebServerTask::_sessionActive = false;
unsigned long WebServerTask::_sessionStartTime = 0;
int WebServerTask::_lastSleepScore = 0;
int WebServerTask::_sessionCount = 0;

bool WebServerTask::_wifiConnected = false;
String WebServerTask::_localIP = "";
unsigned long WebServerTask::_lastWiFiReconnect = 0;
unsigned long WebServerTask::_wifiStartTime = 0;

// ============================================================================
// start() - Punto de entrada público para iniciar la tarea
// ============================================================================

void WebServerTask::start(QueueHandle_t storageQueue, QueueHandle_t sensorQueue) {
    _storageQueue = storageQueue;
    _sensorQueue = sensorQueue;
    
    initWiFi();
    setupRoutes();
    
    xTaskCreatePinnedToCore(
        taskFunction,
        "WebServerTask",
        WEBSERVER_TASK_STACK,
        nullptr,
        WEBSERVER_TASK_PRIORITY,
        &_taskHandle,
        1  // Core 1
    );
}

// ============================================================================
// setSessionActive() - Actualiza estado de sesión
// ============================================================================

void WebServerTask::setSessionActive(bool active, unsigned long startTime) {
    _sessionActive = active;
    if (active) {
        _sessionStartTime = startTime;
    }
}

// ============================================================================
// getLastScore() - Devuelve el último Sleep Score
// ============================================================================

int WebServerTask::getLastScore() {
    return _lastSleepScore;
}

// ============================================================================
// getLocalIP() - Devuelve la IP actual
// ============================================================================

String WebServerTask::getLocalIP() {
    return _localIP;
}

// ============================================================================
// isWiFiConnected() - Devuelve si WiFi está conectado
// ============================================================================

bool WebServerTask::isWiFiConnected() {
    return _wifiConnected;
}

// ============================================================================
// taskFunction() - Bucle principal de la tarea
// ============================================================================

void WebServerTask::taskFunction(void* pvParams) {
    while (true) {
        // Leer datos de sensores si hay cola disponible
        if (_sensorQueue != nullptr) {
            SensorData newData;
            if (xQueueReceive(_sensorQueue, &newData, 0) == pdTRUE) {
                _currentData = newData;
            }
        }
        
        // Atender peticiones del servidor web
        if (_server != nullptr) {
            _server->handleClient();
        }
        
        // Verificar conexión WiFi (reconectar si es necesario)
        if (WiFi.getMode() == WIFI_STA && (millis() - _lastWiFiReconnect) > WIFI_RECONNECT_INTERVAL_MS) {
            _lastWiFiReconnect = millis();
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("[WebTask] WiFi desconectado, reconectando...");
                WiFi.disconnect();
                delay(100);
                WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
                _wifiStartTime = millis();
            } else if (!_wifiConnected) {
                _wifiConnected = true;
                _localIP = WiFi.localIP().toString();
                Serial.printf("[WebTask] WiFi reconectado! IP: %s\n", _localIP.c_str());
            }
        }
        
        delay(10);
    }
}

// ============================================================================
// initWiFi() - Inicializa la conexión WiFi
// ============================================================================

void WebServerTask::initWiFi() {
    Serial.println("\n[WebTask] Inicializando WiFi...");
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    Serial.printf("  Conectando a: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    _wifiStartTime = millis();
    
    while (WiFi.status() != WL_CONNECTED && (millis() - _wifiStartTime) < WIFI_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        _wifiConnected = true;
        _localIP = WiFi.localIP().toString();
        Serial.println("\n  ✅ WiFi Conectado!");
        Serial.printf("  IP: %s\n", _localIP.c_str());
        Serial.printf("  RSSI: %d dBm\n", WiFi.RSSI());
    } else {
        Serial.println("\n  ❌ No se pudo conectar a WiFi");
        Serial.println("  Creando Access Point...");
        
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASSWORD);
        _wifiConnected = false;
        _localIP = WiFi.softAPIP().toString();
        
        Serial.printf("  ✅ AP Creado: %s\n", AP_SSID);
        Serial.printf("  Contraseña: %s\n", AP_PASSWORD);
        Serial.printf("  IP: %s\n", _localIP.c_str());
    }
    
    Serial.println("-----------------------------\n");
}

// ============================================================================
// setupRoutes() - Configura todas las rutas del servidor web
// ============================================================================

void WebServerTask::setupRoutes() {
    _server = new WebServer(HTTP_PORT);
    
    _server->on("/", handleRoot);
    _server->on("/api/data", handleData);
    _server->on("/api/history", handleHistory);
    _server->on("/api/start", handleStart);
    _server->on("/api/end", handleEnd);
    _server->on("/api/status", handleStatus);
    _server->on("/api/wifi/scan", handleWiFiScan);
    _server->on("/api/wifi/config", handleWiFiConfig);
    _server->onNotFound(handleNotFound);
    
    _server->begin();
    Serial.println("[WebTask] Servidor web iniciado en puerto 80");
    Serial.println("[WebTask] Accede desde navegador: http://" + _localIP);
}

// ============================================================================
// handleRoot() - Página principal HTML
// ============================================================================

void WebServerTask::handleRoot() {
    _server->send(200, "text/html", generateHTML());
}

// ============================================================================
// handleData() - API: datos en tiempo real
// ============================================================================

void WebServerTask::handleData() {
    _server->send(200, "application/json", getDataJSON());
}

// ============================================================================
// handleHistory() - API: historial de sesiones
// ============================================================================

void WebServerTask::handleHistory() {
    _server->send(200, "application/json", getHistoryJSON());
}

// ============================================================================
// handleStart() - API: iniciar sesión
// ============================================================================

void WebServerTask::handleStart() {
    if (!_sessionActive) {
        _sessionActive = true;
        _sessionStartTime = millis();
        Serial.println("[WebTask] Sesión iniciada desde web");
        _server->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Sesion iniciada\"}");
    } else {
        _server->send(200, "application/json", "{\"status\":\"error\",\"message\":\"Ya hay una sesion activa\"}");
    }
}

// ============================================================================
// handleEnd() - API: finalizar sesión
// ============================================================================

void WebServerTask::handleEnd() {
    if (_sessionActive) {
        _sessionActive = false;
        
        int score = calculateSleepScore(_currentData);
        _lastSleepScore = score;
        
        // Enviar comando a StorageTask para guardar la sesión
        if (_storageQueue != nullptr) {
            StorageCommand cmd;
            cmd.type = STORAGE_SAVE_SESSION;
            snprintf(cmd.sessionId, sizeof(cmd.sessionId), "S%03d", _sessionCount + 1);
            cmd.sessionStart = _sessionStartTime;
            cmd.sessionEnd = millis();
            cmd.alertCount = 0;
            cmd.avgCO2 = _currentData.co2;
            cmd.avgTemp = _currentData.temperature;
            cmd.avgHum = _currentData.humidity;
            cmd.avgLight = _currentData.light;
            xQueueSend(_storageQueue, &cmd, 0);
            _sessionCount++;
        }
        
        Serial.printf("[WebTask] Sesión finalizada - Score: %d\n", score);
        _server->send(200, "application/json", 
            "{\"status\":\"ok\",\"message\":\"Sesion finalizada\",\"score\":" + String(score) + "}");
    } else {
        _server->send(200, "application/json", "{\"status\":\"error\",\"message\":\"No hay sesion activa\"}");
    }
}

// ============================================================================
// handleStatus() - API: estado del sistema
// ============================================================================

void WebServerTask::handleStatus() {
    _server->send(200, "application/json", getStatusJSON());
}

// ============================================================================
// handleWiFiScan() - API: escanear redes WiFi
// ============================================================================

void WebServerTask::handleWiFiScan() {
    String json = "{\"networks\":[";
    
    Serial.println("[WebTask] Escaneando redes WiFi...");
    int n = WiFi.scanNetworks();
    
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"encryption\":" + String(WiFi.encryptionType(i));
        json += "}";
    }
    
    json += "]}";
    _server->send(200, "application/json", json);
    WiFi.scanDelete();
}

// ============================================================================
// handleWiFiConfig() - API: configurar nueva red WiFi
// ============================================================================

void WebServerTask::handleWiFiConfig() {
    if (_server->hasArg("ssid") && _server->hasArg("password")) {
        String ssid = _server->arg("ssid");
        String password = _server->arg("password");
        
        Serial.printf("[WebTask] Nueva configuración WiFi: %s\n", ssid.c_str());
        
        WiFi.begin(ssid.c_str(), password.c_str());
        _wifiStartTime = millis();
        _wifiConnected = false;
        
        _server->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Conectando a " + ssid + "\"}");
    } else {
        _server->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Faltan ssid o password\"}");
    }
}

// ============================================================================
// handleNotFound() - 404
// ============================================================================

void WebServerTask::handleNotFound() {
    _server->send(404, "application/json", "{\"status\":\"error\",\"message\":\"Recurso no encontrado\"}");
}

// ============================================================================
// generateHTML() - Genera la página web completa
// ============================================================================

String WebServerTask::generateHTML() {
    String wifiMode = _wifiConnected ? "STA" : "AP";
    String wifiInfo = _wifiConnected ? 
        "Red: " + String(WIFI_SSID) + " | IP: " + _localIP :
        "Red: " + String(AP_SSID) + " | IP: " + _localIP;
    
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=yes">
    <title>Smart Sleep Analyzer</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            color: #eee;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            padding: 20px;
            min-height: 100vh;
        }
        .container { max-width: 800px; margin: 0 auto; }
        h1 { text-align: center; margin-bottom: 30px; font-size: 1.8em; }
        h1 span { color: #4ecdc4; }
        .card {
            background: rgba(255,255,255,0.08);
            backdrop-filter: blur(10px);
            border-radius: 20px;
            padding: 20px;
            margin-bottom: 20px;
            border: 1px solid rgba(255,255,255,0.1);
        }
        .card h2 {
            color: #4ecdc4;
            margin-bottom: 15px;
            font-size: 1.2em;
            border-left: 3px solid #4ecdc4;
            padding-left: 12px;
        }
        .sensor-grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 15px;
        }
        .sensor-card {
            background: rgba(0,0,0,0.3);
            border-radius: 15px;
            padding: 15px;
            text-align: center;
        }
        .sensor-label { font-size: 0.9em; color: #aaa; margin-bottom: 5px; }
        .sensor-value { font-size: 2em; font-weight: bold; }
        .sensor-unit { font-size: 0.7em; color: #aaa; }
        .state-optimo { color: #4ecdc4; }
        .state-regular { color: #ffe66d; }
        .state-critico { color: #ff6b6b; }
        .state-indicator {
            display: inline-block;
            width: 16px;
            height: 16px;
            border-radius: 50%;
            margin-right: 8px;
        }
        .indicator-optimo { background-color: #4ecdc4; box-shadow: 0 0 10px #4ecdc4; }
        .indicator-regular { background-color: #ffe66d; box-shadow: 0 0 10px #ffe66d; }
        .indicator-critico { background-color: #ff6b6b; box-shadow: 0 0 10px #ff6b6b; }
        .session-control { display: flex; gap: 15px; justify-content: center; flex-wrap: wrap; }
        button {
            background: #4ecdc4;
            border: none;
            padding: 12px 24px;
            border-radius: 10px;
            font-size: 16px;
            font-weight: bold;
            color: #1a1a2e;
            cursor: pointer;
            transition: transform 0.2s;
        }
        button:hover { transform: scale(1.02); opacity: 0.9; }
        .btn-end { background: #ff6b6b; color: white; }
        .btn-refresh { background: #2c3e50; color: white; }
        table { width: 100%; border-collapse: collapse; }
        th, td { padding: 10px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); }
        th { color: #4ecdc4; }
        tr:hover { background: rgba(255,255,255,0.05); }
        .score { font-size: 2.5em; text-align: center; font-weight: bold; }
        .wifi-info { font-size: 0.8em; color: #4ecdc4; word-break: break-all; }
        .footer { text-align: center; margin-top: 20px; color: #666; font-size: 0.8em; }
        @media (max-width: 600px) {
            .sensor-value { font-size: 1.5em; }
            button { padding: 10px 16px; font-size: 14px; }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>😴 <span>Smart Sleep</span> Analyzer</h1>
        
        <div class="card">
            <h2>📊 Datos en tiempo real</h2>
            <div class="sensor-grid">
                <div class="sensor-card">
                    <div class="sensor-label">CO₂</div>
                    <div class="sensor-value" id="co2">--</div>
                    <div class="sensor-unit">ppm</div>
                </div>
                <div class="sensor-card">
                    <div class="sensor-label">🌡️ Temperatura</div>
                    <div class="sensor-value" id="temp">--</div>
                    <div class="sensor-unit">°C</div>
                </div>
                <div class="sensor-card">
                    <div class="sensor-label">💧 Humedad</div>
                    <div class="sensor-value" id="hum">--</div>
                    <div class="sensor-unit">%</div>
                </div>
                <div class="sensor-card">
                    <div class="sensor-label">💡 Luz</div>
                    <div class="sensor-value" id="light">--</div>
                    <div class="sensor-unit">lux</div>
                </div>
            </div>
            <div style="text-align: center; margin-top: 15px;" id="state">
                <span class="state-indicator indicator-optimo"></span>
                <span>Cargando...</span>
            </div>
        </div>
        
        <div class="card">
            <h2>🎮 Control de sesión</h2>
            <div id="session-status" style="text-align: center; margin-bottom: 15px;"></div>
            <div class="session-control">
                <button onclick="startSession()">▶ Iniciar sesión</button>
                <button class="btn-end" onclick="endSession()">⏹️ Finalizar sesión</button>
            </div>
        </div>
        
        <div class="card">
            <h2>🎯 Último Sleep Score</h2>
            <div class="score" id="score">--</div>
            <div id="score-interp" style="text-align: center;">--</div>
        </div>
        
        <div class="card">
            <h2>📜 Historial de sesiones</h2>
            <div style="overflow-x: auto;">
                <table id="history-table">
                    <thead><tr><th>Fecha</th><th>Duración</th><th>Score</th></tr></thead>
                    <tbody><tr><td colspan="3">Cargando...</td></tr></tbody>
                </table>
            </div>
        </div>
        
        <div class="card">
            <h2>📡 Estado WiFi</h2>
            <div class="wifi-info" id="wifi-info">)" + wifiInfo + R"rawliteral(</div>
            <div class="wifi-info" id="wifi-mode">Modo: )" + wifiMode + R"rawliteral(</div>
        </div>
        
        <div class="footer">
            <button class="btn-refresh" onclick="refreshAll()">⟳ Actualizar todo</button>
            <p>Smart Sleep Analyzer v3.0</p>
        </div>
    </div>
    
    <script>
        function refreshData() {
            fetch('/api/data')
                .then(r => r.json())
                .then(d => {
                    document.getElementById('co2').innerHTML = d.co2;
                    document.getElementById('temp').innerHTML = d.temp;
                    document.getElementById('hum').innerHTML = d.hum;
                    document.getElementById('light').innerHTML = d.light;
                    
                    let cls = d.state == 0 ? 'optimo' : (d.state == 1 ? 'regular' : 'critico');
                    let txt = d.state == 0 ? 'ÓPTIMO' : (d.state == 1 ? 'REGULAR' : 'CRÍTICO');
                    let ind = d.state == 0 ? 'indicator-optimo' : (d.state == 1 ? 'indicator-regular' : 'indicator-critico');
                    
                    document.getElementById('state').innerHTML = 
                        '<span class="state-indicator ' + ind + '"></span>' +
                        '<span class="state-' + cls + '">' + txt + '</span>';
                    
                    let sessionHtml = d.sessionActive ? 
                        '<span style="color:#4ecdc4">🔴 SESIÓN ACTIVA - ' + d.duration + ' min</span>' : 
                        '<span>⚪ Sin sesión activa</span>';
                    document.getElementById('session-status').innerHTML = sessionHtml;
                    
                    document.getElementById('score').innerHTML = d.lastScore;
                    document.getElementById('score-interp').innerHTML = d.scoreInterp;
                });
        }
        
        function refreshHistory() {
            fetch('/api/history')
                .then(r => r.json())
                .then(data => {
                    let html = '';
                    data.forEach(s => {
                        let cls = s.score >= 85 ? 'state-optimo' : (s.score >= 50 ? 'state-regular' : 'state-critico');
                        html += `<tr><td>${s.date}</td><td>${s.duration} min</td><td class="${cls}"><strong>${s.score}</strong></td></tr>`;
                    });
                    if (html === '') html = '<tr><td colspan="3">No hay sesiones guardadas</td></tr>';
                    document.querySelector('#history-table tbody').innerHTML = html;
                });
        }
        
        function refreshAll() { refreshData(); refreshHistory(); }
        function startSession() { fetch('/api/start').then(() => refreshData()); }
        function endSession() { fetch('/api/end').then(() => refreshData()); }
        
        refreshAll();
        setInterval(refreshData, 3000);
        setInterval(refreshHistory, 10000);
    </script>
</body>
</html>
)rawliteral";
}

// ============================================================================
// getDataJSON() - Genera JSON con datos actuales
// ============================================================================

String WebServerTask::getDataJSON() {
    int score = calculateSleepScore(_currentData);
    String scoreInterp = getScoreInterpretation(score);
    
    String json = "{";
    json += "\"co2\":" + String(_currentData.co2) + ",";
    json += "\"temp\":" + String(_currentData.temperature) + ",";
    json += "\"hum\":" + String(_currentData.humidity) + ",";
    json += "\"light\":" + String(_currentData.light) + ",";
    json += "\"state\":" + String(_currentData.state) + ",";
    json += "\"sessionActive\":" + String(_sessionActive ? "true" : "false") + ",";
    json += "\"duration\":" + String(_sessionActive ? (millis() - _sessionStartTime) / 60000 : 0) + ",";
    json += "\"lastScore\":" + String(_lastSleepScore) + ",";
    json += "\"scoreInterp\":\"" + scoreInterp + "\"";
    json += "}";
    return json;
}

// ============================================================================
// getHistoryJSON() - Genera JSON con historial
// ============================================================================

String WebServerTask::getHistoryJSON() {
    // Por ahora retorna array vacío
    // En implementación completa, leería de SD
    return "[]";
}

// ============================================================================
// getStatusJSON() - Genera JSON con estado del sistema
// ============================================================================

String WebServerTask::getStatusJSON() {
    String json = "{";
    json += "\"wifiConnected\":" + String(_wifiConnected ? "true" : "false") + ",";
    json += "\"localIP\":\"" + _localIP + "\",";
    json += "\"sessionActive\":" + String(_sessionActive ? "true" : "false") + ",";
    json += "\"uptime\":" + String(millis() / 1000);
    json += "}";
    return json;
}

// ============================================================================
// calculateSleepScore() - Calcula puntuación de sueño
// ============================================================================

int WebServerTask::calculateSleepScore(const SensorData& data) {
    int score = 0;
    
    // CO2 (máx 40)
    if (data.co2 < 800) score += 40;
    else if (data.co2 < 1000) score += 32;
    else if (data.co2 < 1400) score += 20;
    else if (data.co2 < 1800) score += 10;
    
    // Temperatura (máx 25)
    if (data.temperature >= 18 && data.temperature <= 22) score += 25;
    else if (data.temperature < 24) score += 18;
    else if (data.temperature < 26) score += 10;
    
    // Humedad (máx 20)
    if (data.humidity >= 40 && data.humidity <= 60) score += 20;
    else if ((data.humidity >= 30 && data.humidity < 40) || (data.humidity > 60 && data.humidity <= 70)) score += 12;
    
    // Luz (máx 15)
    if (data.light < 5) score += 15;
    else if (data.light < 20) score += 8;
    
    return score;
}

// ============================================================================
// getScoreInterpretation() - Interpretación del Sleep Score
// ============================================================================

String WebServerTask::getScoreInterpretation(int score) {
    if (score >= 85) return "Óptimo";
    if (score >= 70) return "Bueno";
    if (score >= 50) return "Aceptable";
    if (score >= 30) return "Desfavorable";
    return "Crítico";
}