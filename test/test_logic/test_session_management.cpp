// test_session_management.cpp
// Prueba la gestión de sesiones: inicio, fin, comandos a colas y consistencia de estado.
// Simula las colas de FreeRTOS con arrays circulares simples para la prueba.

#include <Arduino.h>

// ============================================================
// Simulación de colas (sin FreeRTOS real, solo para la prueba)
// ============================================================
#define QUEUE_SIZE 5

struct Command {
    char type[16];      // "START", "END", etc.
    unsigned long value; // timestamp o duración
};

Command cmdQueue[QUEUE_SIZE];
int cmdIn = 0, cmdOut = 0;

// Enviar comando a la cola simulada
bool sendCommand(const char* type, unsigned long value) {
    int next = (cmdIn + 1) % QUEUE_SIZE;
    if (next == cmdOut) {
        Serial.println("❌ Error: cola de comandos llena");
        return false;
    }
    strncpy(cmdQueue[cmdIn].type, type, 15);
    cmdQueue[cmdIn].type[15] = '\0';
    cmdQueue[cmdIn].value = value;
    cmdIn = next;
    Serial.printf("📤 Comando enviado: %s %lu\n", type, value);
    return true;
}

// Recibir comando de la cola (sin bloquear)
bool receiveCommand(Command& cmd) {
    if (cmdIn == cmdOut) return false;
    cmd = cmdQueue[cmdOut];
    cmdOut = (cmdOut + 1) % QUEUE_SIZE;
    return true;
}

// Vaciar cola (para reiniciar pruebas)
void clearCommandQueue() {
    cmdIn = 0;
    cmdOut = 0;
}

// ============================================================
// Simulación del estado de sesión
// ============================================================
enum SessionState { IDLE, ACTIVE };
SessionState sessionState = IDLE;
unsigned long sessionStartTime = 0;
unsigned long sessionDuration = 0; // duración en ms al finalizar

// Iniciar sesión
bool startSession() {
    if (sessionState == ACTIVE) {
        Serial.println("⚠️ No se puede iniciar: sesión ya activa");
        return false;
    }
    sessionState = ACTIVE;
    sessionStartTime = millis();
    sendCommand("START", 0);
    Serial.println("🟢 Sesión iniciada");
    return true;
}

// Finalizar sesión
bool endSession() {
    if (sessionState == IDLE) {
        Serial.println("⚠️ No se puede finalizar: no hay sesión activa");
        return false;
    }
    sessionState = IDLE;
    sessionDuration = millis() - sessionStartTime;
    sendCommand("END", sessionDuration);
    Serial.printf("🔴 Sesión finalizada. Duración: %lu ms\n", sessionDuration);
    return true;
}

// Reiniciar estado (para pruebas múltiples sin resetear placa)
void resetSession() {
    sessionState = IDLE;
    sessionStartTime = 0;
    sessionDuration = 0;
    clearCommandQueue();
}

// ============================================================
// Pruebas
// ============================================================
void runTests() {
    Serial.println("\n=== TEST SESSION MANAGEMENT ===");
    Serial.println("Verificando inicio/fin de sesión y comandos en cola\n");

    // Prueba 1: Secuencia normal inicio -> fin
    Serial.println("--- Prueba 1: Inicio -> Fin ---");
    resetSession();
    startSession();
    delay(100); // simular actividad
    endSession();

    // Verificar comandos generados
    Serial.print("Comandos en cola: ");
    Command cmd;
    int cmdCount = 0;
    while (receiveCommand(cmd)) {
        Serial.printf("[%s %lu] ", cmd.type, cmd.value);
        cmdCount++;
    }
    Serial.println();
    if (cmdCount == 2) Serial.println("✅ Correcto: se generaron 2 comandos (START y END)");
    else Serial.println("❌ Error: número incorrecto de comandos");

    // Prueba 2: Iniciar dos veces seguidas (debe fallar el segundo)
    Serial.println("\n--- Prueba 2: Doble inicio ---");
    resetSession();
    startSession();
    bool secondStart = startSession(); // debe fallar
    if (!secondStart) Serial.println("✅ Correcto: segundo inicio rechazado");
    else Serial.println("❌ Error: se permitió segundo inicio");
    endSession();

    // Prueba 3: Finalizar sin inicio previo (debe fallar)
    Serial.println("\n--- Prueba 3: Fin sin inicio ---");
    resetSession();
    bool endWithoutStart = endSession();
    if (!endWithoutStart) Serial.println("✅ Correcto: fin sin inicio rechazado");
    else Serial.println("❌ Error: se permitió fin sin inicio");

    // Prueba 4: Consistencia de estado después de operaciones
    Serial.println("\n--- Prueba 4: Consistencia de estado ---");
    resetSession();
    Serial.print("Estado inicial: ");
    Serial.println(sessionState == IDLE ? "IDLE ✅" : "ACTIVE ❌");
    startSession();
    Serial.print("Después de start: ");
    Serial.println(sessionState == ACTIVE ? "ACTIVE ✅" : "IDLE ❌");
    endSession();
    Serial.print("Después de end: ");
    Serial.println(sessionState == IDLE ? "IDLE ✅" : "ACTIVE ❌");

    // Prueba 5: Verificar que END contiene la duración correcta (aproximadamente)
    Serial.println("\n--- Prueba 5: Duración en comando END ---");
    resetSession();
    startSession();
    delay(250); // duración conocida
    endSession();
    clearCommandQueue(); // vaciar para leer solo el último END? mejor leer todos
    // Recolectar comandos (debería haber START y END)
    Command startCmd, endCmd;
    bool gotStart = false, gotEnd = false;
    while (receiveCommand(cmd)) {
        if (strcmp(cmd.type, "START") == 0) {
            startCmd = cmd;
            gotStart = true;
        } else if (strcmp(cmd.type, "END") == 0) {
            endCmd = cmd;
            gotEnd = true;
        }
    }
    if (gotEnd) {
        // La duración debe ser >= 250 ms (puede tener overhead)
        if (endCmd.value >= 250) {
            Serial.printf("✅ Duración registrada: %lu ms (esperado >=250 ms)\n", endCmd.value);
        } else {
            Serial.printf("❌ Duración incorrecta: %lu ms (debería ser >=250)\n", endCmd.value);
        }
    } else {
        Serial.println("❌ No se encontró comando END");
    }

    Serial.println("\n=== FIN DE LAS PRUEBAS ===");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    runTests();
}

void loop() {
    // Vacío: las pruebas se ejecutan una sola vez
}