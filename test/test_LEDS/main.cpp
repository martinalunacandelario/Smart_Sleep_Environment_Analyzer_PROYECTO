/**
 * TEST LEDs (Rojo, Amarillo, Verde)
 * Conexiones fijas:
 *   LED Rojo    → GPIO 13
 *   LED Amarillo → GPIO 14
 *   LED Verde   → GPIO 27
 * 
 * Se detiene automáticamente después de 5 ciclos.
 */

#include <Arduino.h>               // Incluye la librería estándar de Arduino (necesaria para PlatformIO)

#define LED_ROJO     13            // Define el pin GPIO13 para el LED rojo
#define LED_AMARILLO 14            // Define el pin GPIO14 para el LED amarillo
#define LED_VERDE    27            // Define el pin GPIO27 para el LED verde

unsigned long stepStartTime = 0;   // Almacena el tiempo (millis) de inicio del paso actual
int currentStep = 0;               // Indica en qué paso de la secuencia nos encontramos (0 a 5)
const unsigned long LED_ON_TIME_MS = 1000;   // Tiempo que cada LED permanece encendido (1 segundo)
const unsigned long LED_OFF_TIME_MS = 500;   // Tiempo de espera entre LEDs (0.5 segundos)
int cycleCount = 0;                // Contador de ciclos completados (rojo → amarillo → verde)
bool testRunning = true;           // Controla si la prueba sigue activa (false al terminar)

void setup() {                     // Función de configuración: se ejecuta una vez al inicio
  Serial.begin(115200);            // Inicia la comunicación serie a 115200 baudios
  delay(1000);                     // Espera 1 segundo para estabilizar el monitor serie
  Serial.println("\n=== TEST LEDs (Rojo=13, Amarillo=14, Verde=27) ==="); // Mensaje de inicio
  Serial.println("Encendiendo todos los LEDs por 2 segundos..."); // Informa prueba inicial

  pinMode(LED_ROJO, OUTPUT);       // Configura el pin del LED rojo como salida
  pinMode(LED_AMARILLO, OUTPUT);   // Configura el pin del LED amarillo como salida
  pinMode(LED_VERDE, OUTPUT);      // Configura el pin del LED verde como salida

  // Prueba inicial: todos encendidos (verifica que ningún LED esté roto)
  digitalWrite(LED_ROJO, HIGH);    // Enciende LED rojo (HIGH = encendido)
  digitalWrite(LED_AMARILLO, HIGH);// Enciende LED amarillo
  digitalWrite(LED_VERDE, HIGH);   // Enciende LED verde
  delay(2000);                     // Mantiene todos encendidos durante 2 segundos

  // Apagar todo y empezar secuencia
  digitalWrite(LED_ROJO, LOW);     // Apaga LED rojo
  digitalWrite(LED_AMARILLO, LOW); // Apaga LED amarillo
  digitalWrite(LED_VERDE, LOW);    // Apaga LED verde
  
  Serial.println("Iniciando secuencia (5 ciclos, luego se detiene)...\n"); // Mensaje
  stepStartTime = millis();        // Guarda el tiempo actual como inicio del paso
  currentStep = 0;                 // Comienza con el paso 0 (rojo encendido)
  digitalWrite(LED_ROJO, HIGH);    // Enciende el primer LED (rojo)
  Serial.println("Rojo ENCENDIDO"); // Notifica por serie
}

void loop() {                      // Función principal: se ejecuta repetidamente
  if (!testRunning) return;        // Si la prueba ya terminó, no hacer nada

  unsigned long now = millis();    // Obtiene el tiempo actual en milisegundos

  switch (currentStep) {           // Evalúa en qué paso de la secuencia estamos
    case 0: // Rojo encendido
      if (now - stepStartTime >= LED_ON_TIME_MS) { // Si ha pasado 1 segundo
        digitalWrite(LED_ROJO, LOW);   // Apaga el LED rojo
        stepStartTime = now;           // Reinicia el temporizador para el siguiente paso
        currentStep = 1;               // Avanza al paso 1 (pausa antes del amarillo)
        Serial.println("Rojo APAGADO"); // Notifica
      }
      break;

    case 1: // Pausa antes del amarillo
      if (now - stepStartTime >= LED_OFF_TIME_MS) { // Si ha pasado 0.5 segundos
        digitalWrite(LED_AMARILLO, HIGH); // Enciende LED amarillo
        stepStartTime = now;              // Reinicia temporizador
        currentStep = 2;                  // Avanza al paso 2 (amarillo encendido)
        Serial.println("Amarillo ENCENDIDO");
      }
      break;

    case 2: // Amarillo encendido
      if (now - stepStartTime >= LED_ON_TIME_MS) { // 1 segundo encendido
        digitalWrite(LED_AMARILLO, LOW);  // Apaga amarillo
        stepStartTime = now;
        currentStep = 3;                  // Paso 3 (pausa antes del verde)
        Serial.println("Amarillo APAGADO");
      }
      break;

    case 3: // Pausa antes del verde
      if (now - stepStartTime >= LED_OFF_TIME_MS) { // 0.5 segundos
        digitalWrite(LED_VERDE, HIGH);    // Enciende verde
        stepStartTime = now;
        currentStep = 4;                  // Paso 4 (verde encendido)
        Serial.println("Verde ENCENDIDO");
      }
      break;

    case 4: // Verde encendido
      if (now - stepStartTime >= LED_ON_TIME_MS) { // 1 segundo encendido
        digitalWrite(LED_VERDE, LOW);     // Apaga verde
        stepStartTime = now;
        currentStep = 5;                  // Paso 5 (fin de ciclo)
        Serial.println("Verde APAGADO");
      }
      break;

    case 5: // Fin de ciclo
      if (now - stepStartTime >= LED_OFF_TIME_MS) { // Espera 0.5 segundos antes de repetir
        cycleCount++;                      // Incrementa contador de ciclos
        Serial.print("✅ Ciclo #");        // Muestra número de ciclo
        Serial.println(cycleCount);

        if (cycleCount >= 5) {             // Si ya se hicieron 5 ciclos
          testRunning = false;             // Detiene la prueba
          digitalWrite(LED_ROJO, LOW);     // Apaga todos los LEDs
          digitalWrite(LED_AMARILLO, LOW);
          digitalWrite(LED_VERDE, LOW);
          Serial.println("\n=== PRUEBA COMPLETADA (5 ciclos) ===");
          Serial.println("Todos los LEDs apagados. Fin.");
          return;                          // Sale de loop() (no se ejecuta más)
        }

        // Si no se alcanzaron 5 ciclos, reinicia el ciclo con el LED rojo
        digitalWrite(LED_ROJO, HIGH);      // Enciende rojo
        stepStartTime = now;               // Reinicia temporizador
        currentStep = 0;                   // Vuelve al paso 0
        Serial.println("Rojo ENCENDIDO (nuevo ciclo)");
      }
      break;
  }
}