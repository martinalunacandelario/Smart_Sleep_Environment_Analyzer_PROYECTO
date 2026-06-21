#include "task_Button.h"
#include "../../include/config.h"

#define BUTTON_PIN            25   // GPIO25 — pulsador físico
#define DEBOUNCE_DELAY_MS     50   // Tiempo de antirrebote en ms
#define BUTTON_TASK_PRIORITY   2   // Prioridad baja
#define BUTTON_TASK_STACK   2048   // Tamaño de pila

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================
TaskHandle_t ButtonTask::_taskHandle = nullptr;

// ============================================================================
// start() - Configura el pin e inicia la tarea
// ============================================================================
void ButtonTask::start() {
    // Configurar pin como entrada con pull-up interna
    // (LOW = presionado, HIGH = soltado)
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    xTaskCreatePinnedToCore(
        taskFunction,
        "task_Button",
        BUTTON_TASK_STACK,
        nullptr,
        BUTTON_TASK_PRIORITY,
        &_taskHandle,
        1   // Núcleo 1
    );
}

// ============================================================================
// taskFunction() - Detecta pulsaciones y delega en SessionManager
// SessionManager notifica automáticamente a Display, Sensor, Storage y Alert
// ============================================================================
void ButtonTask::taskFunction(void* pvParams) {
    int lastButtonState   = HIGH;  // Último estado leído
    int stableButtonState = HIGH;  // Estado estable tras antirrebote
    unsigned long lastDebounceTime = 0;

    while (true) {
        int reading = digitalRead(BUTTON_PIN);

        // Si el estado ha cambiado, reiniciar temporizador de rebote
        if (reading != lastButtonState) {
            lastDebounceTime = millis();
        }

        // Si ha pasado el tiempo de debounce, estado considerado estable
        if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
            if (reading != stableButtonState) {
                stableButtonState = reading;

                // Solo reaccionar en transición HIGH → LOW (botón presionado)
                if (stableButtonState == LOW) {
                    // Delegar en SessionManager: él actualiza el estado
                    // y notifica a todas las tareas suscritas automáticamente
                    if (SessionManager::isSessionActive()) {
                        SessionManager::stopSession();
                    } else {
                        SessionManager::startSession();
                    }
                }
            }
        }

        lastButtonState = reading;
        vTaskDelay(pdMS_TO_TICKS(10));  // Revisar cada 10 ms
    }
}