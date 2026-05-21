/**
 * TEST INDIVIDUAL: Pulsador físico (botón) para inicio/fin de sesión
 * 
 * Conexión recomendada:
 *   - Un terminal del botón a GPIO 25
 *   - El otro terminal a GND
 *   - Se activará resistencia pull-up interna (INPUT_PULLUP)
 * 
 * Funcionalidad:
 *   - Detecta pulsaciones con debounce por software
 *   - Mide duración de pulsación (tiempo presionado)
 *   - Mide tiempo entre pulsaciones
 *   - Indica flanco de bajada (presionado) y subida (liberado)
 * 
 * Validaciones:
 *   - Lectura digital (escritura/lectura básica)
 *   - Timings: estabilidad de debounce, medición de intervalos
 *   - Sin librerías externas
 */

#include <Arduino.h>

// Configuración del pin del botón (cambiar si es necesario)
#define BUTTON_PIN 25

// Variables para debounce y timings
volatile bool lastStableState = HIGH;   // Último estado estable (HIGH = no presionado)
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY_MS = 50;   // 50 ms típico para debounce

// Variables para medir duración de pulsación
unsigned long pressStartTime = 0;
unsigned long pressDuration = 0;
unsigned long lastPressTime = 0;         // Momento en que terminó la última pulsación
unsigned long timeBetweenPresses = 0;

// Contador de pulsaciones
int pressCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== TEST PULSADOR (BOTON) ===");
  Serial.print("Pin utilizado: GPIO ");
  Serial.println(BUTTON_PIN);
  Serial.println("Conecta el botón entre GPIO y GND. (Pull-up interno activado)");
  Serial.println("El sistema detectará pulsaciones y medirá timings.\n");

  // Configurar pin como entrada con pull-up interno
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Leer estado inicial estable
  lastStableState = digitalRead(BUTTON_PIN);
  Serial.println("Sistema listo. Presiona el botón...\n");
}

void loop() {
  // Leer estado actual del pin (puede tener rebotes)
  bool currentReading = digitalRead(BUTTON_PIN);
  
  // Si el estado ha cambiado respecto al último estable, reiniciamos temporizador de debounce
  if (currentReading != lastStableState) {
    lastDebounceTime = millis();
  }
  
  // Si ha pasado el tiempo de debounce, consideramos el cambio como válido
  if ((millis() - lastDebounceTime) >= DEBOUNCE_DELAY_MS) {
    // Si el estado estable actual es diferente al que teníamos registrado, hay cambio real
    if (currentReading != lastStableState) {
      lastStableState = currentReading;
      
      if (lastStableState == LOW) {   // Botón presionado (conectado a GND)
        // Inicio de pulsación
        pressStartTime = millis();
        pressCount++;
        Serial.print("+++ BOTON PRESIONADO (inicio pulsacion) - Pulsacion #");
        Serial.println(pressCount);
        
        // Mostrar tiempo desde la última pulsación (si no es la primera)
        if (lastPressTime != 0) {
          timeBetweenPresses = pressStartTime - lastPressTime;
          Serial.print("    Tiempo desde anterior pulsacion: ");
          Serial.print(timeBetweenPresses);
          Serial.println(" ms");
        }
        
      } else {  // lastStableState == HIGH -> botón liberado
        // Fin de pulsación: calcular duración
        pressDuration = millis() - pressStartTime;
        lastPressTime = millis();
        Serial.print("--- BOTON LIBERADO - Duracion pulsacion: ");
        Serial.print(pressDuration);
        Serial.println(" ms");
      }
    }
  }
  
  // Pequeña pausa para no saturar (no afecta timings)
  delay(10);
}