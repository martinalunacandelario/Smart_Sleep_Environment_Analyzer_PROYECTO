#include "ButtonTask.h"
#include "../../include/config.h"          // Para definir el pin y constantes si las hubiera

// Pin del pulsador (según especificación: GPIO25)
#define BUTTON_PIN  25

// Tiempo de antirrebote (debounce) en milisegundos
#define DEBOUNCE_DELAY_MS  50

// Prioridad de la tarea (baja, ya que es solo entrada de usuario)
#define BUTTON_TASK_PRIORITY  2

// Tamaño de la pila
#define BUTTON_TASK_STACK     2048

// Variables estáticas
TaskHandle_t ButtonTask::_taskHandle = nullptr;
QueueHandle_t ButtonTask::_cmdQueue = nullptr;

// ------------------------------------------------------------------
// start() - Inicializa el botón y crea la tarea
// ------------------------------------------------------------------
void ButtonTask::start(QueueHandle_t cmdQueue) {
    _cmdQueue = cmdQueue;

    // Configurar el pin del botón como entrada con pull-up interna
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Crear la tarea FreeRTOS en el núcleo 1 (prioridad baja)
    xTaskCreatePinnedToCore(
        taskFunction,
        "ButtonTask",
        BUTTON_TASK_STACK,
        nullptr,
        BUTTON_TASK_PRIORITY,
        &_taskHandle,
        1
    );
}

// ------------------------------------------------------------------
// taskFunction() - Bucle principal: detecta pulsaciones y envía comandos
// ------------------------------------------------------------------
void ButtonTask::taskFunction(void* pvParams) {
    // Estado anterior del botón (inicialmente HIGH porque no está presionado)
    int lastButtonState = HIGH;
    // Momento del último cambio de estado (para debounce)
    unsigned long lastDebounceTime = 0;
    // Estado actual después del antirrebote
    int stableButtonState = HIGH;

    // Estado de la sesión local (para alternar)
    bool sessionActive = false;

    while (true) {
        // Leer el estado actual del botón (LOW = presionado, HIGH = soltado)
        int reading = digitalRead(BUTTON_PIN);

        // Si el estado ha cambiado, reiniciar el temporizador de rebote
        if (reading != lastButtonState) {
            lastDebounceTime = millis();
        }

        // Si ha pasado el tiempo de debounce, considerar el estado como estable
        if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
            // Si el estado estable ha cambiado respecto al anterior
            if (reading != stableButtonState) {
                stableButtonState = reading;

                // Solo nos interesa la transición de HIGH -> LOW (presionado)
                if (stableButtonState == LOW) {
                    // Alternar el estado de sesión
                    sessionActive = !sessionActive;

                    // Crear el comando para DisplayTask
                    DisplayCommand cmd;
                    cmd.sessionActive = sessionActive;

                    // Enviar el comando a la cola (no bloqueante, timeout 0)
                    xQueueSend(_cmdQueue, &cmd, 0);

                    // Mensaje de depuración por Serial
                    Serial.printf("[Button] Sesión %s\n", sessionActive ? "INICIADA" : "FINALIZADA");
                }
            }
        }

        // Guardar la última lectura para la próxima iteración
        lastButtonState = reading;

        // Pequeña pausa para no saturar la CPU (10 ms)
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}