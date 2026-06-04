#include "SessionManager.h"

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================
SemaphoreHandle_t SessionManager::_mutex           = nullptr;
bool              SessionManager::_sessionActive   = false;
QueueHandle_t     SessionManager::_subscribers[SESSION_MAX_SUBSCRIBERS] = {};
int               SessionManager::_subscriberCount = 0;

// ============================================================================
// init() - Crear el mutex. Llamar una vez en setup() antes de crear tareas
// ============================================================================
void SessionManager::init() {
    _mutex = xSemaphoreCreateMutex();
    if (_mutex == nullptr) {
        Serial.println("[SessionManager] Error al crear mutex");
    } else {
        Serial.println("[SessionManager] Inicializado");
    }
}

// ============================================================================
// subscribe() - Registrar una cola para recibir notificaciones
// ============================================================================
void SessionManager::subscribe(QueueHandle_t queue) {
    if (_subscriberCount < SESSION_MAX_SUBSCRIBERS) {
        _subscribers[_subscriberCount++] = queue;
    } else {
        Serial.println("[SessionManager] ERROR: demasiados suscriptores");
    }
}

// ============================================================================
// startSession() - Iniciar sesión y notificar a todos los suscriptores
// ============================================================================
bool SessionManager::startSession() {
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;

    if (_sessionActive) {
        // Ya había sesión activa, ignorar
        xSemaphoreGive(_mutex);
        Serial.println("[SessionManager] Sesion ya estaba activa");
        return false;
    }

    _sessionActive = true;
    xSemaphoreGive(_mutex);  // Liberar ANTES de notificar para evitar deadlocks

    notifyAll(true);
    Serial.println("[SessionManager] Sesion INICIADA");
    return true;
}

// ============================================================================
// stopSession() - Finalizar sesión y notificar a todos los suscriptores
// ============================================================================
bool SessionManager::stopSession() {
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;

    if (!_sessionActive) {
        // No había sesión activa, ignorar
        xSemaphoreGive(_mutex);
        Serial.println("[SessionManager] Sesion ya estaba inactiva");
        return false;
    }

    _sessionActive = false;
    xSemaphoreGive(_mutex);  // Liberar ANTES de notificar

    notifyAll(false);
    Serial.println("[SessionManager] Sesion FINALIZADA");
    return true;
}

// ============================================================================
// isSessionActive() - Consultar estado actual de forma segura
// ============================================================================
bool SessionManager::isSessionActive() {
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    bool active = _sessionActive;
    xSemaphoreGive(_mutex);
    return active;
}

// ============================================================================
// notifyAll() - Enviar SessionCommand a todas las colas suscritas
// ============================================================================
void SessionManager::notifyAll(bool active) {
    SessionCommand cmd;
    cmd.sessionActive = active;
    for (int i = 0; i < _subscriberCount; i++) {
        if (_subscribers[i] != nullptr) {
            xQueueSend(_subscribers[i], &cmd, 0);
        }
    }
}