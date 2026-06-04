#include "ButtonTask.h"
#include "../../include/config.h"

// Pin del pulsador (GPIO25)
#define BUTTON_PIN  25

// Tiempo de antirrebote en milisegundos
#define DEBOUNCE_DELAY_MS  50

// Prioridad y tamaño de pila de la tarea
#define BUTTON_TASK_PRIORITY  2
#define BUTTON_TASK_STACK     2048

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================
TaskHandle_t  ButtonTask::_taskHandle      = nullptr;
QueueHandle_t ButtonTask::_cmdQueueDisplay = nullptr;  // Cola para DisplayTask
QueueHandle_t ButtonTask::_cmdQueueSensor  = nullptr;  // Cola para SensorTask
QueueHandle_t ButtonTask::_cmdQueueStorage = nullptr;  // Cola para StorageTask

// ============================================================================
// start() - Inicializa el botón y crea la tarea
// ============================================================================
void ButtonTask::start(QueueHandle_t cmdQueueDisplay,
                       QueueHandle_t cmdQueueSensor,
                       QueueHandle_t cmdQueueStorage) {
    _cmdQueueDisplay = cmdQueueDisplay;  // Guardar cola de Display
    _cmdQueueSensor  = cmdQueueSensor;   // Guardar cola de Sensor
    _cmdQueueStorage = cmdQueueStorage;  // Guardar cola de Storage

    // Configurar el pin del botón como entrada con pull-up interna
    // (LOW = presionado, HIGH = soltado)
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

// ============================================================================
// taskFunction() - Detecta pulsaciones y publica el comando en las TRES colas
// ============================================================================
void ButtonTask::taskFunction(void* pvParams) {
    int lastButtonState   = HIGH;  // Último estado leído del botón
    int stableButtonState = HIGH;  // Estado estable tras el antirrebote
    unsigned long lastDebounceTime = 0;

    bool sessionActive = false;  // Estado de sesión local (empieza inactiva)

    while (true) {
        // Leer el estado actual del botón (LOW = presionado, HIGH = soltado)
        int reading = digitalRead(BUTTON_PIN);

        // Si el estado ha cambiado, reiniciar el temporizador de rebote
        if (reading != lastButtonState) {
            lastDebounceTime = millis();
        }

        // Si ha pasado el tiempo de debounce, el estado se considera estable
        if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
            if (reading != stableButtonState) {
                stableButtonState = reading;

                // Solo reaccionar en la transición HIGH → LOW (botón presionado)
                if (stableButtonState == LOW) {
                    sessionActive = !sessionActive;  // Alternar estado de sesión

                    // Preparar el comando con el nuevo estado
                    DisplayCommand cmd;
                    cmd.sessionActive = sessionActive;

                    // Publicar en las TRES colas para que cada tarea reciba su copia
                    xQueueSend(_cmdQueueDisplay, &cmd, 0);  // Para DisplayTask
                    xQueueSend(_cmdQueueSensor,  &cmd, 0);  // Para SensorTask
                    xQueueSend(_cmdQueueStorage, &cmd, 0);  // Para StorageTask

                    Serial.printf("[Button] Sesion %s\n",
                                  sessionActive ? "INICIADA" : "FINALIZADA");
                }
            }
        }

        // Guardar la última lectura para la próxima iteración
        lastButtonState = reading;

        // Pausa de 10 ms para no saturar la CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}