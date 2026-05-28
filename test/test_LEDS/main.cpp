/**
 * TEST LEDs (Rojo, Amarillo, Verde)
 * Conexiones fijas:
 *   LED Rojo    → GPIO 13
 *   LED Amarillo → GPIO 14
 *   LED Verde   → GPIO 27
 * 
 * Se detiene automáticamente después de 5 ciclos.
 */

#include <Arduino.h>

#define LED_ROJO     13
#define LED_AMARILLO 14
#define LED_VERDE    27

unsigned long stepStartTime = 0;
int currentStep = 0;
const unsigned long LED_ON_TIME_MS = 1000;
const unsigned long LED_OFF_TIME_MS = 500;
int cycleCount = 0;
bool testRunning = true;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== TEST LEDs (Rojo=13, Amarillo=14, Verde=27) ===");
  Serial.println("Encendiendo todos los LEDs por 2 segundos...");

  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);

  // Prueba inicial: todos encendidos
  digitalWrite(LED_ROJO, HIGH);
  digitalWrite(LED_AMARILLO, HIGH);
  digitalWrite(LED_VERDE, HIGH);
  delay(2000);

  // Apagar todo y empezar secuencia
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_VERDE, LOW);
  
  Serial.println("Iniciando secuencia (5 ciclos, luego se detiene)...\n");
  stepStartTime = millis();
  currentStep = 0;
  digitalWrite(LED_ROJO, HIGH);
  Serial.println("Rojo ENCENDIDO");
}

void loop() {
  if (!testRunning) return;

  unsigned long now = millis();

  switch (currentStep) {
    case 0: // Rojo on
      if (now - stepStartTime >= LED_ON_TIME_MS) {
        digitalWrite(LED_ROJO, LOW);
        stepStartTime = now;
        currentStep = 1;
        Serial.println("Rojo APAGADO");
      }
      break;
    case 1: // espera
      if (now - stepStartTime >= LED_OFF_TIME_MS) {
        digitalWrite(LED_AMARILLO, HIGH);
        stepStartTime = now;
        currentStep = 2;
        Serial.println("Amarillo ENCENDIDO");
      }
      break;
    case 2: // Amarillo on
      if (now - stepStartTime >= LED_ON_TIME_MS) {
        digitalWrite(LED_AMARILLO, LOW);
        stepStartTime = now;
        currentStep = 3;
        Serial.println("Amarillo APAGADO");
      }
      break;
    case 3: // espera
      if (now - stepStartTime >= LED_OFF_TIME_MS) {
        digitalWrite(LED_VERDE, HIGH);
        stepStartTime = now;
        currentStep = 4;
        Serial.println("Verde ENCENDIDO");
      }
      break;
    case 4: // Verde on
      if (now - stepStartTime >= LED_ON_TIME_MS) {
        digitalWrite(LED_VERDE, LOW);
        stepStartTime = now;
        currentStep = 5;
        Serial.println("Verde APAGADO");
      }
      break;
    case 5: // fin de ciclo
      if (now - stepStartTime >= LED_OFF_TIME_MS) {
        cycleCount++;
        Serial.print("✅ Ciclo #");
        Serial.println(cycleCount);

        if (cycleCount >= 5) {
          testRunning = false;
          digitalWrite(LED_ROJO, LOW);
          digitalWrite(LED_AMARILLO, LOW);
          digitalWrite(LED_VERDE, LOW);
          Serial.println("\n=== PRUEBA COMPLETADA (5 ciclos) ===");
          Serial.println("Todos los LEDs apagados. Fin.");
          return;
        }

        digitalWrite(LED_ROJO, HIGH);
        stepStartTime = now;
        currentStep = 0;
        Serial.println("Rojo ENCENDIDO (nuevo ciclo)");
      }
      break;
  }
  delay(1);
}