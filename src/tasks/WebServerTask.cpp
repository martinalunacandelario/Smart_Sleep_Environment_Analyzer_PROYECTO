#include "WebServerTask.h"
#include "../../include/config.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================
TaskHandle_t WebServerTask::_taskHandle = nullptr;
WebServer WebServerTask::server(WEB_SERVER_PORT);
SensorData WebServerTask::_currentData = {0};

// ============================================================================
// start() - Ya no recibe cola, usa SessionManager
// ============================================================================
void WebServerTask::start() {
    setupAccessPoint();

    server.on("/", handleRoot);
    server.on("/api/status", handleApiStatus);
    server.on("/api/session", HTTP_POST, handleApiSession);
    server.on("/api/sessions", handleApiSessions);
    server.on("/api/session/stats", handleApiSessionStats);
    server.on("/api/session/alerts", handleApiSessionAlerts);
    server.on("/api/session/data", handleApiSessionData);
    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("[Web] Servidor web iniciado");
    Serial.printf("[Web] Accede a: http://%s\n", WiFi.softAPIP().toString().c_str());

    xTaskCreatePinnedToCore(
        taskFunction,
        "WebServerTask",
        WEB_TASK_STACK,
        nullptr,
        WEB_TASK_PRIORITY,
        &_taskHandle,
        1
    );
}

// ============================================================================
// setupAccessPoint() - Configura el ESP32 como Access Point
// ============================================================================
void WebServerTask::setupAccessPoint() {
    Serial.println("[Web] Configurando Access Point...");
    WiFi.mode(WIFI_AP);
    bool result = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, AP_HIDDEN);
    if (result) {
        Serial.println("[Web] Access Point creado correctamente");
        Serial.printf("[Web] SSID: %s\n", AP_SSID);
        Serial.printf("[Web] Contraseña: %s\n", AP_PASSWORD);
        Serial.printf("[Web] IP del ESP32: %s\n", WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println("[Web] Error al crear el Access Point");
    }
}

// ============================================================================
// taskFunction() - Bucle principal del servidor web
// ============================================================================
void WebServerTask::taskFunction(void* pvParams) {
    while (true) {
        server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ============================================================================
// handleApiStatus() - Usa SessionManager para el estado real de sesión
// ============================================================================
void WebServerTask::handleApiStatus() {
    StaticJsonDocument<256> doc;
    doc["co2"] = _currentData.co2;
    doc["temperature"] = _currentData.temperature;
    doc["humidity"] = _currentData.humidity;
    doc["light"] = _currentData.light;

    String status = "DESCONOCIDO";
    if (_currentData.co2 > 0) {
        bool malo = false, regular = false;
        if (_currentData.co2 > CO2_ACCEPTABLE_MAX) malo = true;
        else if (_currentData.co2 > CO2_GOOD_MAX) regular = true;
        if (_currentData.temperature < TEMP_GOOD_MIN ||
            _currentData.temperature > TEMP_ACCEPTABLE_MAX) malo = true;
        else if (_currentData.temperature > TEMP_GOOD_MAX) regular = true;
        if (_currentData.humidity < HUM_ACCEPTABLE_MIN1 ||
            _currentData.humidity > HUM_ACCEPTABLE_MAX2) malo = true;
        else if ((_currentData.humidity >= HUM_ACCEPTABLE_MIN1 && _currentData.humidity < HUM_GOOD_MIN) ||
                 (_currentData.humidity > HUM_GOOD_MAX && _currentData.humidity <= HUM_ACCEPTABLE_MAX2)) regular = true;
        if (_currentData.light >= LIGHT_ACCEPTABLE_MAX) malo = true;
        else if (_currentData.light >= LIGHT_GOOD_MAX) regular = true;
        if (malo) status = "MALO";
        else if (regular) status = "ACEPTABLE";
        else status = "OPTIMO";
    }
    doc["status"] = status;

    // CLAVE: estado real desde SessionManager, no una variable local que puede estar desincronizada
    doc["sessionActive"] = SessionManager::isSessionActive();

    unsigned long secs = millis() / 1000;
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d",
            (int)(secs / 3600) % 24,
            (int)(secs / 60) % 60,
            (int)(secs % 60));
    doc["datetime"] = timeStr;

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

// ============================================================================
// handleApiSession() - Una sola llamada notifica a todas las tareas
// ============================================================================
void WebServerTask::handleApiSession() {
    if (server.method() != HTTP_POST) {
        server.send(405, "application/json", "{\"error\":\"Method not allowed\"}");
        return;
    }

    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    bool sessionActive = doc["sessionActive"];

    // CLAVE: una sola llamada notifica a Display, Sensor, Storage y Alert
    if (sessionActive) {
        SessionManager::startSession();
    } else {
        SessionManager::stopSession();
    }

    StaticJsonDocument<64> responseDoc;
    responseDoc["success"] = true;
    responseDoc["sessionActive"] = SessionManager::isSessionActive();

    String response;
    serializeJson(responseDoc, response);
    server.send(200, "application/json", response);
}

// ============================================================================
// updateCurrentData() - Actualiza los datos para /api/status
// ============================================================================
void WebServerTask::updateCurrentData(const SensorData &data) {
    _currentData = data;
}

// ============================================================================
// handleRoot() - Sirve la página principal HTML (con Chart.js y gráficas)
// ============================================================================
void WebServerTask::handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=yes">
    <title>Smart Sleep Environment Analyzer</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #0f0c29 0%, #302b63 50%, #24243e 100%);
            color: #eee;
            min-height: 100vh;
            padding: 20px;
        }
        
        .container { max-width: 1200px; margin: 0 auto; }
        
        .header { text-align: center; margin-bottom: 30px; }
        .header h1 {
            font-size: 2rem;
            background: linear-gradient(135deg, #00b4db, #0083b0);
            -webkit-background-clip: text;
            background-clip: text;
            color: transparent;
            margin-bottom: 10px;
        }
        .header p { color: #aaa; font-size: 0.9rem; }
        
        .card {
            background: rgba(255, 255, 255, 0.08);
            backdrop-filter: blur(10px);
            border-radius: 20px;
            padding: 20px;
            margin-bottom: 20px;
            border: 1px solid rgba(255, 255, 255, 0.1);
            transition: transform 0.3s;
        }
        .card:hover { transform: translateY(-3px); }
        .card h2 {
            margin-bottom: 15px;
            color: #00b4db;
            font-size: 1.2rem;
            border-left: 4px solid #00b4db;
            padding-left: 12px;
        }
        
        .grid-2 { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; }
        .grid-4 { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 20px; }
        
        .sensor-card {
            background: rgba(0, 0, 0, 0.3);
            border-radius: 15px;
            padding: 15px;
            text-align: center;
            transition: all 0.3s;
        }
        .sensor-card:hover { transform: translateY(-5px); background: rgba(0, 180, 219, 0.2); }
        .sensor-card .value { font-size: 2rem; font-weight: bold; color: #00b4db; }
        .sensor-card .unit { font-size: 0.8rem; color: #888; }
        .sensor-card .label { margin-top: 10px; font-size: 0.85rem; color: #aaa; }
        
        .status {
            display: inline-block;
            padding: 8px 16px;
            border-radius: 30px;
            font-weight: bold;
            font-size: 0.9rem;
        }
        .status-optimal { background: linear-gradient(135deg, #00c853, #00a844); color: #fff; box-shadow: 0 0 15px rgba(0,200,83,0.3); }
        .status-acceptable { background: linear-gradient(135deg, #ffc107, #ff9800); color: #333; }
        .status-critical { background: linear-gradient(135deg, #d32f2f, #c62828); color: #fff; box-shadow: 0 0 15px rgba(211,47,47,0.3); }
        .status-off { background: #555; color: #fff; }
        
        .btn {
            padding: 10px 24px;
            border: none;
            border-radius: 40px;
            cursor: pointer;
            font-size: 1rem;
            font-weight: bold;
            transition: all 0.3s;
            margin: 5px;
        }
        .btn-primary { background: linear-gradient(135deg, #00b4db, #0083b0); color: #fff; }
        .btn-primary:hover { transform: scale(1.05); box-shadow: 0 5px 20px rgba(0,180,219,0.4); }
        .btn-danger { background: linear-gradient(135deg, #d32f2f, #c62828); color: #fff; }
        .btn-danger:hover { transform: scale(1.05); box-shadow: 0 5px 20px rgba(211,47,47,0.4); }
        
        .session-controls { text-align: center; margin: 20px 0; }
        
        .session-list { max-height: 300px; overflow-y: auto; }
        .session-item {
            background: rgba(255, 255, 255, 0.05);
            border-radius: 10px;
            padding: 12px 15px;
            margin-bottom: 8px;
            cursor: pointer;
            transition: all 0.3s;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .session-item:hover { background: rgba(0, 180, 219, 0.2); transform: translateX(5px); }
        .session-info { display: flex; flex-direction: column; }
        .session-date { font-size: 0.85rem; color: #aaa; }
        .session-score { font-weight: bold; color: #00b4db; font-size: 1.1rem; }
        
        .modal {
            display: none;
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(0, 0, 0, 0.95);
            z-index: 1000;
            justify-content: center;
            align-items: center;
            overflow-y: auto;
        }
        .modal-content {
            background: linear-gradient(135deg, #1a1a2e, #16213e);
            border-radius: 20px;
            max-width: 95%;
            width: 900px;
            max-height: 90vh;
            overflow-y: auto;
            padding: 25px;
            position: relative;
            border: 1px solid rgba(0, 180, 219, 0.3);
        }
        .modal-content h2 { color: #00b4db; margin-bottom: 20px; }
        .modal-content h3 { color: #00b4db; font-size: 1rem; margin: 20px 0 10px 0; }
        .close {
            position: absolute;
            top: 15px;
            right: 20px;
            font-size: 28px;
            cursor: pointer;
            color: #888;
            transition: color 0.3s;
        }
        .close:hover { color: #fff; }
        
        .stat-detail {
            background: rgba(0, 0, 0, 0.3);
            border-radius: 10px;
            padding: 12px;
            margin: 10px 0;
        }
        
        .chart-container {
            background: rgba(0, 0, 0, 0.3);
            border-radius: 10px;
            padding: 15px;
            margin: 15px 0;
        }
        canvas { max-height: 200px; width: 100%; }
        
        .alert-list { list-style: none; padding: 0; }
        .alert-list li {
            background: rgba(211, 47, 47, 0.2);
            margin: 8px 0;
            padding: 10px;
            border-radius: 8px;
            border-left: 3px solid #d32f2f;
        }
        
        .loading { text-align: center; padding: 30px; color: #888; }
        
        .tab-buttons { display: flex; gap: 10px; margin: 15px 0; flex-wrap: wrap; }
        .tab-btn {
            background: rgba(255,255,255,0.1);
            border: none;
            padding: 8px 16px;
            border-radius: 20px;
            cursor: pointer;
            color: #eee;
            transition: all 0.3s;
        }
        .tab-btn.active { background: #00b4db; color: #fff; }
        .tab-btn:hover { background: rgba(0,180,219,0.5); }
        .tab-content { display: none; }
        .tab-content.active { display: block; }
        
        ::-webkit-scrollbar { width: 8px; }
        ::-webkit-scrollbar-track { background: #1a1a2e; border-radius: 10px; }
        ::-webkit-scrollbar-thumb { background: #00b4db; border-radius: 10px; }
        
        @media (max-width: 768px) {
            body { padding: 15px; }
            .header h1 { font-size: 1.5rem; }
            .sensor-card .value { font-size: 1.3rem; }
            .grid-4 { grid-template-columns: repeat(2, 1fr); }
            .modal-content { width: 95%; padding: 15px; }
        }
    </style>
    <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
    <script>
        let updateInterval = null;
        let charts = {};
        
        async function updateRealtimeData() {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();
                document.getElementById('co2_value').textContent = data.co2 !== undefined ? Math.round(data.co2) : '--';
                document.getElementById('temp_value').textContent = data.temperature !== undefined ? data.temperature.toFixed(1) : '--';
                document.getElementById('hum_value').textContent = data.humidity !== undefined ? Math.round(data.humidity) : '--';
                document.getElementById('light_value').textContent = data.light !== undefined ? Math.round(data.light) : '--';
                
                const statusEl = document.getElementById('status');
                statusEl.textContent = data.status || 'DESCONOCIDO';
                statusEl.className = 'status';
                if (data.status === 'OPTIMO') statusEl.classList.add('status-optimal');
                else if (data.status === 'ACEPTABLE') statusEl.classList.add('status-acceptable');
                else if (data.status === 'MALO') statusEl.classList.add('status-critical');
                else statusEl.classList.add('status-off');
                
                const sessionEl = document.getElementById('session_status');
                sessionEl.textContent = data.sessionActive ? 'ACTIVA' : 'INACTIVA';
                sessionEl.className = 'status';
                if (data.sessionActive) sessionEl.classList.add('status-optimal');
                else sessionEl.classList.add('status-off');
            } catch (error) { console.error('Error:', error); }
        }
        
        async function startSession() {
            try {
                const response = await fetch('/api/session', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ sessionActive: true })
                });
                const result = await response.json();
                if (result.success) { updateRealtimeData(); loadSessions(); }
            } catch (error) { console.error('Error:', error); }
        }
        
        async function stopSession() {
            try {
                const response = await fetch('/api/session', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ sessionActive: false })
                });
                const result = await response.json();
                if (result.success) { updateRealtimeData(); loadSessions(); }
            } catch (error) { console.error('Error:', error); }
        }
        
        async function loadSessions() {
            try {
                const response = await fetch('/api/sessions');
                const sessions = await response.json();
                const listEl = document.getElementById('session-list');
                listEl.innerHTML = '';
                if (!sessions || sessions.length === 0) {
                    listEl.innerHTML = '<div class="loading">📭 No hay sesiones registradas</div>';
                    return;
                }
                sessions.sort((a, b) => b.id - a.id);
                sessions.forEach(session => {
                    const item = document.createElement('div');
                    item.className = 'session-item';
                    item.innerHTML = `<div class="session-info"><span>Sesión #${session.id}</span><span class="session-date">${session.date || ''}</span></div><span class="session-score">⭐ ${session.sleepScore || '--'}</span>`;
                    item.onclick = () => showSessionDetail(session.id);
                    listEl.appendChild(item);
                });
            } catch (error) { console.error('Error:', error); }
        }
        
        async function showSessionDetail(sessionId) {
            const modal = document.getElementById('session-modal');
            const content = document.getElementById('modal-content');
            content.innerHTML = '<div class="loading">⏳ Cargando datos de la sesión...</div>';
            modal.style.display = 'flex';
            
            try {
                const [statsRes, alertsRes, dataRes] = await Promise.all([
                    fetch(`/api/session/stats?id=${sessionId}`),
                    fetch(`/api/session/alerts?id=${sessionId}`),
                    fetch(`/api/session/data?id=${sessionId}`)
                ]);
                const stats = await statsRes.json();
                const alerts = await alertsRes.json();
                const timeline = await dataRes.json();
                
                let durationText = '--';
                if (stats.duration) {
                    const horas = Math.floor(stats.duration / 3600);
                    const minutos = Math.floor((stats.duration % 3600) / 60);
                    durationText = `${horas}h ${minutos}min`;
                }
                
                let alertsHtml = '';
                if (alerts.alerts && alerts.alerts.length > 0) {
                    alerts.alerts.forEach(alert => {
                        alertsHtml += `<li><strong>⏰ ${alert.time}</strong> → ${alert.type}: ${alert.message}</li>`;
                    });
                } else { alertsHtml = '<li>✅ No hay alertas registradas</li>'; }
                
                let bestHourText = '--';
                if (stats.bestHour && stats.bestHour.start) {
                    const startMin = Math.floor(stats.bestHour.start / 60);
                    const startSec = stats.bestHour.start % 60;
                    const endMin = Math.floor(stats.bestHour.end / 60);
                    const endSec = stats.bestHour.end % 60;
                    bestHourText = `${startMin.toString().padStart(2,'0')}:${startSec.toString().padStart(2,'0')} - ${endMin.toString().padStart(2,'0')}:${endSec.toString().padStart(2,'0')}`;
                }
                
                content.innerHTML = `
                    <h2>📋 Detalle de Sesión #${sessionId}</h2>
                    <div class="stat-detail">
                        <h3>📊 Información General</h3>
                        <p><strong>Duración:</strong> ${durationText}</p>
                        <p><strong>Sleep Score:</strong> <span style="color:#00b4db; font-size:1.8rem; font-weight:bold;">${stats.sleepScore || '--'}</span>/100</p>
                        <p><strong>Interpretación:</strong> ${stats.interpretation || '--'}</p>
                    </div>
                    <div class="stat-detail">
                        <h3>🌡️ Estadísticas Ambientales</h3>
                        <p><strong>CO₂:</strong> Media: ${stats.co2?.avg || '--'} ppm | Máx: ${stats.co2?.max || '--'} | Mín: ${stats.co2?.min || '--'}</p>
                        <p><strong>Temperatura:</strong> Media: ${stats.temperature?.avg || '--'} °C | Máx: ${stats.temperature?.max || '--'} | Mín: ${stats.temperature?.min || '--'}</p>
                        <p><strong>Humedad:</strong> Media: ${stats.humidity?.avg || '--'} % | Máx: ${stats.humidity?.max || '--'} | Mín: ${stats.humidity?.min || '--'}</p>
                        <p><strong>Luz:</strong> Media: ${stats.light?.avg || '--'} lux | Máx: ${stats.light?.max || '--'} | Mín: ${stats.light?.min || '--'}</p>
                    </div>
                    <div class="stat-detail">
                        <h3>⭐ Mejor Franja Horaria</h3>
                        <p>${bestHourText}</p>
                    </div>
                    <div class="stat-detail">
                        <h3>⚠️ Alertas Generadas</h3>
                        <ul class="alert-list">${alertsHtml}</ul>
                    </div>
                    <div class="stat-detail">
                        <h3>📈 Sleep Timeline</h3>
                        <div class="tab-buttons">
                            <button class="tab-btn active" onclick="switchTab('co2')">CO₂</button>
                            <button class="tab-btn" onclick="switchTab('temp')">Temperatura</button>
                            <button class="tab-btn" onclick="switchTab('hum')">Humedad</button>
                            <button class="tab-btn" onclick="switchTab('light')">Luz</button>
                        </div>
                        <div id="tab-co2" class="tab-content active"><div class="chart-container"><canvas id="chart-co2"></canvas></div></div>
                        <div id="tab-temp" class="tab-content"><div class="chart-container"><canvas id="chart-temp"></canvas></div></div>
                        <div id="tab-hum" class="tab-content"><div class="chart-container"><canvas id="chart-hum"></canvas></div></div>
                        <div id="tab-light" class="tab-content"><div class="chart-container"><canvas id="chart-light"></canvas></div></div>
                    </div>
                `;
                
                if (timeline.timestamps && timeline.timestamps.length > 0) {
                    const labels = timeline.timestamps.map(ts => {
                        const horas = Math.floor(ts / 3600);
                        const minutos = Math.floor((ts % 3600) / 60);
                        return `${horas.toString().padStart(2,'0')}:${minutos.toString().padStart(2,'0')}`;
                    });
                    
                    if (window.charts) {
                        Object.values(window.charts).forEach(chart => { if (chart) chart.destroy(); });
                    }
                    window.charts = {};
                    
                    window.charts.co2 = new Chart(document.getElementById('chart-co2'), {
                        type: 'line', data: { labels: labels, datasets: [{ label: 'CO₂ (ppm)', data: timeline.co2 || [], borderColor: '#00b4db', backgroundColor: 'rgba(0,180,219,0.1)', fill: true, tension: 0.3 }] },
                        options: { responsive: true, maintainAspectRatio: true, plugins: { legend: { labels: { color: '#eee' } } }, scales: { x: { ticks: { color: '#aaa', maxRotation: 45, autoSkip: true, maxTicksLimit: 12 }, title: { display: true, text: 'Hora', color: '#aaa' } }, y: { ticks: { color: '#aaa' }, title: { display: true, text: 'ppm', color: '#aaa' } } } }
                    });
                    window.charts.temp = new Chart(document.getElementById('chart-temp'), {
                        type: 'line', data: { labels: labels, datasets: [{ label: 'Temperatura (°C)', data: timeline.temperature || [], borderColor: '#ff9800', backgroundColor: 'rgba(255,152,0,0.1)', fill: true, tension: 0.3 }] },
                        options: { responsive: true, maintainAspectRatio: true, plugins: { legend: { labels: { color: '#eee' } } }, scales: { x: { ticks: { color: '#aaa', maxRotation: 45, autoSkip: true, maxTicksLimit: 12 }, title: { display: true, text: 'Hora', color: '#aaa' } }, y: { ticks: { color: '#aaa' }, title: { display: true, text: '°C', color: '#aaa' } } } }
                    });
                    window.charts.hum = new Chart(document.getElementById('chart-hum'), {
                        type: 'line', data: { labels: labels, datasets: [{ label: 'Humedad (%)', data: timeline.humidity || [], borderColor: '#4caf50', backgroundColor: 'rgba(76,175,80,0.1)', fill: true, tension: 0.3 }] },
                        options: { responsive: true, maintainAspectRatio: true, plugins: { legend: { labels: { color: '#eee' } } }, scales: { x: { ticks: { color: '#aaa', maxRotation: 45, autoSkip: true, maxTicksLimit: 12 }, title: { display: true, text: 'Hora', color: '#aaa' } }, y: { ticks: { color: '#aaa' }, title: { display: true, text: '%', color: '#aaa' } } } }
                    });
                    window.charts.light = new Chart(document.getElementById('chart-light'), {
                        type: 'line', data: { labels: labels, datasets: [{ label: 'Luz (lux)', data: timeline.light || [], borderColor: '#ffc107', backgroundColor: 'rgba(255,193,7,0.1)', fill: true, tension: 0.3 }] },
                        options: { responsive: true, maintainAspectRatio: true, plugins: { legend: { labels: { color: '#eee' } } }, scales: { x: { ticks: { color: '#aaa', maxRotation: 45, autoSkip: true, maxTicksLimit: 12 }, title: { display: true, text: 'Hora', color: '#aaa' } }, y: { ticks: { color: '#aaa' }, title: { display: true, text: 'lux', color: '#aaa' } } } }
                    });
                } else {
                    document.querySelectorAll('.tab-content').forEach(el => el.innerHTML = '<div class="loading">📊 No hay datos suficientes para mostrar gráficas</div>');
                }
            } catch (error) {
                content.innerHTML = '<div class="loading">❌ Error al cargar los datos de la sesión</div>';
            }
        }
        
        function switchTab(tab) {
            document.querySelectorAll('.tab-content').forEach(el => el.classList.remove('active'));
            document.querySelectorAll('.tab-btn').forEach(el => el.classList.remove('active'));
            document.getElementById(`tab-${tab}`).classList.add('active');
            event.target.classList.add('active');
            setTimeout(() => {
                if (window.charts && window.charts[tab]) window.charts[tab].resize();
            }, 100);
        }
        
        function closeModal() { document.getElementById('session-modal').style.display = 'none'; }
        
        document.addEventListener('keydown', (e) => { if (e.key === 'Escape') closeModal(); });
        document.addEventListener('DOMContentLoaded', () => {
            updateRealtimeData();
            loadSessions();
            updateInterval = setInterval(updateRealtimeData, 3000);
        });
    </script>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>😴 Smart Sleep Environment Analyzer</h1>
            <p>Monitoriza y mejora la calidad de tu descanso</p>
        </div>
        
        <div class="session-controls">
            <button class="btn btn-primary" onclick="startSession()">▶ INICIAR SESIÓN</button>
            <button class="btn btn-danger" onclick="stopSession()">⏹ FINALIZAR SESIÓN</button>
        </div>
        
        <div class="card">
            <h2>📡 Datos Ambientales en Tiempo Real</h2>
            <div class="grid-4">
                <div class="sensor-card"><div class="value" id="co2_value">--</div><div class="unit">ppm</div><div class="label">CO₂</div></div>
                <div class="sensor-card"><div class="value" id="temp_value">--</div><div class="unit">°C</div><div class="label">Temperatura</div></div>
                <div class="sensor-card"><div class="value" id="hum_value">--</div><div class="unit">%</div><div class="label">Humedad</div></div>
                <div class="sensor-card"><div class="value" id="light_value">--</div><div class="unit">lux</div><div class="label">Iluminación</div></div>
            </div>
        </div>
        
        <div class="grid-2">
            <div class="card"><h2>🎯 Estado Ambiental</h2><div style="text-align:center; padding:15px;"><div class="value" id="status" class="status">--</div></div></div>
            <div class="card"><h2>🔘 Estado de Sesión</h2><div style="text-align:center; padding:15px;"><div class="value" id="session_status" class="status">--</div></div></div>
        </div>
        
        <div class="grid-2">
            <div class="card"><h2>🏆 Ranking de Sesiones</h2><div class="session-list" id="session-list"><div class="loading">⏳ Cargando...</div></div></div>
            <div class="card"><h2>📋 Historial de Sesiones</h2><div class="session-list" id="session-list-historial"><div class="loading">⏳ Cargando...</div></div><small style="color:#888; display:block; margin-top:10px;">💡 Haz clic en una sesión para ver los detalles completos y gráficas</small></div>
        </div>
    </div>
    
    <div id="session-modal" class="modal" onclick="closeModal()">
        <div class="modal-content" onclick="event.stopPropagation()">
            <span class="close" onclick="closeModal()">&times;</span>
            <div id="modal-content"><div class="loading">⏳ Cargando...</div></div>
        </div>
    </div>
</body>
</html>
    )rawliteral";
    
    server.send(200, "text/html", html);
}

// ============================================================================
// handleApiSessions() - Lista todas las sesiones disponibles
// ============================================================================
void WebServerTask::handleApiSessions() {
    StaticJsonDocument<4096> doc;
    JsonArray sessions = doc.to<JsonArray>();
    File root = SD.open(SD_BASE_PATH);
    if (root) {
        File file = root.openNextFile();
        while (file) {
            String name = file.name();
            if (name.endsWith("_stats.json")) {
                File statsFile = SD.open(name.c_str(), FILE_READ);
                if (statsFile) {
                    StaticJsonDocument<512> statsDoc;
                    if (!deserializeJson(statsDoc, statsFile)) {
                        JsonObject session = sessions.createNestedObject();
                        session["id"] = statsDoc["sessionId"];
                        session["sleepScore"] = statsDoc["sleepScore"];
                        int firstUnderscore = name.indexOf("_");
                        int secondUnderscore = name.indexOf("_", firstUnderscore + 1);
                        if (firstUnderscore >= 0 && secondUnderscore > firstUnderscore) {
                            session["date"] = "Sesión #" + name.substring(firstUnderscore + 1, secondUnderscore);
                        } else {
                            session["date"] = "Sesión #" + String(statsDoc["sessionId"].as<unsigned long>());
                        }
                    }
                    statsFile.close();
                }
            }
            file = root.openNextFile();
        }
        root.close();
    }
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

// ============================================================================
// handleApiSessionStats() - Devuelve estadísticas de una sesión específica
// ============================================================================
void WebServerTask::handleApiSessionStats() {
    String sessionId = server.arg("id");
    if (sessionId.length() == 0) {
        server.send(400, "application/json", "{\"error\":\"Missing session id\"}");
        return;
    }
    char filePath[64];
    snprintf(filePath, sizeof(filePath), "%s/session_%s_stats.json", SD_BASE_PATH, sessionId.c_str());
    File statsFile = SD.open(filePath, FILE_READ);
    if (!statsFile) {
        server.send(404, "application/json", "{\"error\":\"Session not found\"}");
        return;
    }
    String content = statsFile.readString();
    statsFile.close();
    server.send(200, "application/json", content);
}

// ============================================================================
// handleApiSessionAlerts() - Devuelve alertas de una sesión específica
// ============================================================================
void WebServerTask::handleApiSessionAlerts() {
    String sessionId = server.arg("id");
    if (sessionId.length() == 0) {
        server.send(400, "application/json", "{\"error\":\"Missing session id\"}");
        return;
    }
    char filePath[64];
    snprintf(filePath, sizeof(filePath), "%s/session_%s_alerts.json", SD_BASE_PATH, sessionId.c_str());
    File alertsFile = SD.open(filePath, FILE_READ);
    if (!alertsFile) {
        server.send(200, "application/json", "{\"sessionId\":" + sessionId + ",\"alerts\":[]}");
        return;
    }
    String content = alertsFile.readString();
    alertsFile.close();
    server.send(200, "application/json", content);
}

// ============================================================================
// handleApiSessionData() - Devuelve los datos del CSV en JSON para gráficas
// ============================================================================
void WebServerTask::handleApiSessionData() {
    String sessionId = server.arg("id");
    if (sessionId.length() == 0) {
        server.send(400, "application/json", "{\"error\":\"Missing session id\"}");
        return;
    }
    
    File root = SD.open(SD_BASE_PATH);
    String csvFile = "";
    if (root) {
        File file = root.openNextFile();
        while (file) {
            String name = file.name();
            if (name.indexOf("session_" + sessionId) >= 0 && name.endsWith(".csv")) {
                csvFile = name;
                break;
            }
            file = root.openNextFile();
        }
        root.close();
    }
    
    if (csvFile.length() == 0) {
        server.send(404, "application/json", "{\"error\":\"Data file not found\"}");
        return;
    }
    
    File dataFile = SD.open(csvFile.c_str(), FILE_READ);
    if (!dataFile) {
        server.send(404, "application/json", "{\"error\":\"Cannot open data file\"}");
        return;
    }
    
    StaticJsonDocument<32768> doc;
    JsonArray timestamps = doc.createNestedArray("timestamps");
    JsonArray co2Values = doc.createNestedArray("co2");
    JsonArray tempValues = doc.createNestedArray("temperature");
    JsonArray humValues = doc.createNestedArray("humidity");
    JsonArray lightValues = doc.createNestedArray("light");
    
    bool headerSkipped = false;
    while (dataFile.available()) {
        String line = dataFile.readStringUntil('\n');
        if (line.length() == 0) continue;
        if (line.startsWith("#")) continue;
        if (!headerSkipped && line.startsWith("timestamp_ms")) {
            headerSkipped = true;
            continue;
        }
        if (!headerSkipped) continue;
        
        int idx1 = line.indexOf(',');
        int idx2 = line.indexOf(',', idx1 + 1);
        int idx3 = line.indexOf(',', idx2 + 1);
        int idx4 = line.indexOf(',', idx3 + 1);
        
        if (idx1 < 0 || idx2 < 0 || idx3 < 0 || idx4 < 0) continue;
        
        unsigned long ts = line.substring(0, idx1).toInt();
        float co2 = line.substring(idx1 + 1, idx2).toFloat();
        float temp = line.substring(idx2 + 1, idx3).toFloat();
        float hum = line.substring(idx3 + 1, idx4).toFloat();
        float light = line.substring(idx4 + 1).toFloat();
        
        timestamps.add(ts);
        co2Values.add(co2);
        tempValues.add(temp);
        humValues.add(hum);
        lightValues.add(light);
    }
    dataFile.close();
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

// ============================================================================
// handleNotFound() - Maneja rutas no encontradas
// ============================================================================
void WebServerTask::handleNotFound() {
    server.send(404, "text/plain", "404: Not Found");
}

// ============================================================================
// updateCurrentData() - Actualiza los datos para /api/status
// ============================================================================
void WebServerTask::updateCurrentData(const SensorData &data) {
    _currentData = data;
}