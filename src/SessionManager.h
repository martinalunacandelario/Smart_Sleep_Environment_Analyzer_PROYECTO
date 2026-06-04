#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <Arduino.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// Número máximo de tareas suscritas a cambios de sesión
#define SESSION_MAX_SUBSCRIBERS  6

// Estructura de comando de sesión
struct SessionCommand {
    bool sessionActive;  // true = sesión iniciada, false = finalizada
};

class SessionManager {
public:
    // Inicializar el manager — llamar UNA VEZ en setup() antes de crear tareas
    static void init();

    // Suscribir una cola para recibir notificaciones de cambio de sesión
    // Cada tarea consumidora llama a esto con su propia cola
    static void subscribe(QueueHandle_t queue);

    // Iniciar sesión — lo puede llamar ButtonTask, WebServerTask, o cualquier otra
    // Devuelve false si ya había una sesión activa
    static bool startSession();

    // Finalizar sesión
    // Devuelve false si no había sesión activa
    static bool stopSession();

    // Consultar estado actual de forma segura desde cualquier tarea o handler web
    static bool isSessionActive();

private:
    static SemaphoreHandle_t _mutex;                                    // Protege el estado compartido
    static bool              _sessionActive;                            // Estado actual de la sesión
    static QueueHandle_t     _subscribers[SESSION_MAX_SUBSCRIBERS];    // Colas suscritas
    static int               _subscriberCount;                         // Número de suscriptores

    // Notifica a todos los suscriptores con el nuevo estado
    static void notifyAll(bool active);
};

#endif