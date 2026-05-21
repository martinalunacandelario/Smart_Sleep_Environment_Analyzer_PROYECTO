/**
 * TEST INDIVIDUAL: Active Buzzer (Zumbador activo)
 * 
 * Conexiones:
 *   VCC (+) → GPIO 15
 *   GND (-) → GND
 * 
 * Validaciones:
 *   - Escritura digital básica (HIGH/LOW)
 *   - Temporización con millis() no bloqueante
 *   - Secuencia de pitidos (corto, largo, pausas)
 *   - Medición de duraciones y ciclos
 *   - Comprobación de funcionamiento audible
 */

#include <Arduino.h>

// Definición del pin del buzzer
#define BUZZER_PIN 15

// Variables para control de timings no bloqueante
unsigned long lastChangeTime = 0;
int step = 0;               // 0=buzzer ON, 1=buzzer OFF, 2=espera larga, etc.
unsigned long stepStartTime = 0;

// Duración de cada fase (ms)
const unsigned long BUZZER_ON_SHORT_MS = 200;    // pitido corto: 200 ms
const unsigned long BUZZER_OFF_SHORT_MS = 300;   // pausa corta: 300 ms
const unsigned long BUZZER_ON_LONG_MS = 800;     // pitido largo: 800 ms
const unsigned long BUZZER_OFF_LONG_MS = 500;    // pausa larga: 500 ms
const unsigned long PAUSE_BETWEEN_CYCLES_MS = 2000; // pausa entre ciclos completos

// Contador de ciclos completos
int cycleCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== TEST ACTIVE BUZZER ===");
  Serial.println("Conexiones: VCC(+) → GPIO 15, GND(-) → GND");
  Serial.println("El buzzer generará pitidos cortos y largos con pausas.\n");

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);  // Asegurar apagado

  // Iniciar primera secuencia
  stepStartTime = millis();
  step = 0;   // pitido corto
  digitalWrite(BUZZER_PIN, HIGH);
  Serial.println("Pitido CORTO (200 ms)");
}

void loop() {
  unsigned long now = millis();

  switch (step) {
    case 0: // Pitido corto encendido
      if (now - stepStartTime >= BUZZER_ON_SHORT_MS) {
        digitalWrite(BUZZER_PIN, LOW);
        stepStartTime = now;
        step = 1;
        Serial.println("Pausa corta (300 ms)");
      }
      break;

    case 1: // Pausa corta
      if (now - stepStartTime >= BUZZER_OFF_SHORT_MS) {
        digitalWrite(BUZZER_PIN, HIGH);
        stepStartTime = now;
        step = 2;
        Serial.println("Pitido LARGO (800 ms)");
      }
      break;

    case 2: // Pitido largo encendido
      if (now - stepStartTime >= BUZZER_ON_LONG_MS) {
        digitalWrite(BUZZER_PIN, LOW);
        stepStartTime = now;
        step = 3;
        Serial.println("Pausa larga (500 ms)");
      }
      break;

    case 3: // Pausa larga
      if (now - stepStartTime >= BUZZER_OFF_LONG_MS) {
        cycleCount++;
        Serial.print("Ciclo #");
        Serial.print(cycleCount);
        Serial.println(" completado. Esperando 2 segundos...");
        // Apagar buzzer por si acaso
        digitalWrite(BUZZER_PIN, LOW);
        stepStartTime = now;
        step = 4;
      }
      break;

    case 4: // Pausa entre ciclos (2 segundos)
      if (now - stepStartTime >= PAUSE_BETWEEN_CYCLES_MS) {
        // Reiniciar secuencia
        digitalWrite(BUZZER_PIN, HIGH);
        stepStartTime = now;
        step = 0;
        Serial.println("Iniciando nuevo ciclo: Pitido CORTO");
      }
      break;
  }

  delay(1); // pequeña pausa para no saturar
}