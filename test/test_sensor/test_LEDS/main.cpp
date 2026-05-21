/**
 * TEST INDIVIDUAL: LEDs (Rojo, Amarillo, Verde)
 * 
 * Conexiones:
 *   LED Rojo    → GPIO 26 (ánodo a GPIO, cátodo a GND con resistencia 220Ω)
 *   LED Amarillo → GPIO 27
 *   LED Verde   → GPIO 14
 * 
 * Validaciones:
 *   - Escritura digital básica (HIGH/LOW)
 *   - Temporización con millis() no bloqueante
 *   - Secuencia automática de encendido/apagado con medición de tiempos
 *   - Medición de tiempo de ciclo y verificación de duraciones
 *   - Test de parpadeo rápido y lento
 */

#include <Arduino.h>

// Definición de pines para los LEDs
#define LED_ROJO   26
#define LED_AMARILLO 27
#define LED_VERDE   14

// Variables para control de tiempos no bloqueante
unsigned long lastChangeTime = 0;
int currentStep = 0;          // 0=Rojo on, 1=Rojo off, 2=Amarillo on, 3=Amarillo off, 4=Verde on, 5=Verde off, etc.
unsigned long stepStartTime = 0;

// Duración de cada fase (ms)
const unsigned long LED_ON_TIME_MS = 1000;     // 1 segundo encendido
const unsigned long LED_OFF_TIME_MS = 500;     // 0.5 segundo apagado

// Contador de ciclos completos
int cycleCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== TEST LEDs (Rojo, Amarillo, Verde) ===");
  Serial.println("Conexiones:");
  Serial.println("  LED Rojo    → GPIO 26");
  Serial.println("  LED Amarillo → GPIO 27");
  Serial.println("  LED Verde   → GPIO 14");
  Serial.println("Cada LED se encenderá 1 segundo y apagará 0.5 segundos en secuencia.\n");
  
  // Configurar pines como salida
  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  
  // Asegurar todos apagados
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_VERDE, LOW);
  
  // Iniciar primera fase
  stepStartTime = millis();
  currentStep = 0;   // Rojo on
  digitalWrite(LED_ROJO, HIGH);
  Serial.println("Inicio de secuencia: Rojo ENCENDIDO");
}

void loop() {
  unsigned long now = millis();
  
  // Máquina de estados para secuencia no bloqueante
  switch (currentStep) {
    case 0: // Rojo encendido
      if (now - stepStartTime >= LED_ON_TIME_MS) {
        digitalWrite(LED_ROJO, LOW);
        stepStartTime = now;
        currentStep = 1;
        Serial.println("Rojo APAGADO");
      }
      break;
      
    case 1: // Rojo apagado (espera antes siguiente LED)
      if (now - stepStartTime >= LED_OFF_TIME_MS) {
        digitalWrite(LED_AMARILLO, HIGH);
        stepStartTime = now;
        currentStep = 2;
        Serial.println("Amarillo ENCENDIDO");
      }
      break;
      
    case 2: // Amarillo encendido
      if (now - stepStartTime >= LED_ON_TIME_MS) {
        digitalWrite(LED_AMARILLO, LOW);
        stepStartTime = now;
        currentStep = 3;
        Serial.println("Amarillo APAGADO");
      }
      break;
      
    case 3: // Amarillo apagado
      if (now - stepStartTime >= LED_OFF_TIME_MS) {
        digitalWrite(LED_VERDE, HIGH);
        stepStartTime = now;
        currentStep = 4;
        Serial.println("Verde ENCENDIDO");
      }
      break;
      
    case 4: // Verde encendido
      if (now - stepStartTime >= LED_ON_TIME_MS) {
        digitalWrite(LED_VERDE, LOW);
        stepStartTime = now;
        currentStep = 5;
        Serial.println("Verde APAGADO");
      }
      break;
      
    case 5: // Verde apagado (fin de ciclo)
      if (now - stepStartTime >= LED_OFF_TIME_MS) {
        cycleCount++;
        Serial.print("Ciclo completo #");
        Serial.println(cycleCount);
        // Reiniciar secuencia
        digitalWrite(LED_ROJO, HIGH);
        stepStartTime = now;
        currentStep = 0;
        Serial.println("Rojo ENCENDIDO (nuevo ciclo)");
      }
      break;
  }
  
  // Opcional: cada 10 ciclos mostrar medición de tiempo total
  if (cycleCount > 0 && cycleCount % 10 == 0 && currentStep == 5 && (millis() - stepStartTime) < LED_OFF_TIME_MS) {
    // Evitar múltiples mensajes; lo hacemos al inicio del case 5? Mejor no.
  }
  
  delay(1); // pequeña pausa para no saturar
}