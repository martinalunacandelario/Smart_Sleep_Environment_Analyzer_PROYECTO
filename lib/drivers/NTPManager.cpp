#include "NTPManager.h"
#include "../../include/config.h"

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================
bool NTPManager::_timeSynced = false;
unsigned long NTPManager::_lastSync = 0;
unsigned long NTPManager::_epoch = 0;

// ============================================================================
// begin() - Conecta WiFi (STA) y crea el Access Point (AP) simultáneamente
// ============================================================================
bool NTPManager::begin() {
    Serial.println("[NTP] Iniciando configuración de red...");

    // Modo dual: STA (cliente, para internet/NTP) + AP (red propia del ESP32)
    WiFi.mode(WIFI_AP_STA);

    Serial.printf("[NTP] Conectando a WiFi: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Esperamos hasta 15s (30 x 500ms) a que conecte el STA
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    // Canal por defecto (fallback) si el STA no llega a conectar
    int apChannel = AP_CHANNEL;

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[NTP] ✅ WiFi conectado!");
        Serial.printf("[NTP] IP (STA): %s\n", WiFi.localIP().toString().c_str());

        // *** FIX CLAVE ***
        // El radio WiFi del ESP32 es único: AP y STA deben ir en el MISMO canal.
        // En vez de forzar AP_CHANNEL (fijo en config.h), leemos el canal real
        // al que se ha conectado el STA y se lo pasamos al AP.
        apChannel = WiFi.channel();
        Serial.printf("[NTP] Canal STA detectado: %d\n", apChannel);
    } else {
        Serial.println("\n[NTP] ⚠️ No se pudo conectar a WiFi");
        Serial.println("[NTP] La hora no se sincronizará. Usará millis() como fallback.");
    }

    // Creamos el AP usando el canal correcto (el del STA, si conectó)
    bool apOk = WiFi.softAP(AP_SSID, AP_PASSWORD, apChannel, AP_HIDDEN);
    if (apOk) {
        Serial.printf("[NTP] ✅ AP creado: %s (canal %d)\n", AP_SSID, apChannel);
        Serial.printf("[NTP] IP (AP): %s\n", WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println("[NTP] ⚠️ Error al crear el AP");
    }

    // Comprobamos de nuevo el estado del STA, por si softAP() lo afectó
    Serial.printf("[NTP] Estado WiFi tras crear AP: %d (3 = WL_CONNECTED)\n", WiFi.status());

    if (WiFi.status() == WL_CONNECTED) {
        syncNTP();
    }

    return WiFi.status() == WL_CONNECTED;
}

// ============================================================================
// syncNTP() - Sincroniza la hora con servidores NTP
// ============================================================================
bool NTPManager::syncNTP() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[NTP] ⚠️ No hay WiFi para sincronizar NTP");
        return false;
    }

    Serial.println("[NTP] Sincronizando hora NTP...");

    configTime(TIMEZONE_OFFSET, DAYLIGHT_OFFSET, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

    // Esperamos hasta 10s a que el sistema reciba una hora válida
    int attempts = 0;
    while (time(nullptr) < 100000 && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (time(nullptr) >= 100000) {
        _timeSynced = true;
        _lastSync = millis();
        _epoch = time(nullptr);

        Serial.println("\n[NTP] ✅ Hora NTP sincronizada!");
        Serial.printf("[NTP] Hora actual: %s\n", getCurrentDateTime().c_str());
        return true;
    } else {
        Serial.println("\n[NTP] ⚠️ No se pudo sincronizar NTP");
        Serial.println("[NTP] Prueba con servidores NTP por IP si tu hotspot bloquea DNS/UDP:");
        Serial.println("[NTP]   configTime(TIMEZONE_OFFSET, DAYLIGHT_OFFSET, \"162.159.200.1\");");
        return false;
    }
}

// ============================================================================
// FUNCIONES PARA OBTENER FECHA Y HORA
// ============================================================================
String NTPManager::getCurrentDateTime() {
    if (!_timeSynced) return "Sin NTP";
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buffer);
}

String NTPManager::getCurrentDate() {
    if (!_timeSynced) return "Sin fecha";
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char buffer[16];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);
    return String(buffer);
}

String NTPManager::getCurrentTime() {
    if (!_timeSynced) return "Sin hora";
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char buffer[16];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
    return String(buffer);
}

String NTPManager::getCurrentTimeHM() {
    if (!_timeSynced) return "Sin hora";
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char buffer[6];
    strftime(buffer, sizeof(buffer), "%H:%M", &timeinfo);
    return String(buffer);
}

unsigned long NTPManager::getCurrentEpoch() {
    if (!_timeSynced) return 0;
    _epoch = time(nullptr);
    return _epoch;
}

bool NTPManager::isTimeSynced() {
    return _timeSynced;
}

String NTPManager::getLocalIP() {
    return WiFi.localIP().toString();
}

String NTPManager::getAPIP() {
    return WiFi.softAPIP().toString();
}