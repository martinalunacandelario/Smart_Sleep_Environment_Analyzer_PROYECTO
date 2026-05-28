#include <Arduino.h>   // ← Línea esencial para PlatformIO

/*
 * Test de botón para ESP32
 * Pin: GPIO25 (G25)
 * Conexión: Botón entre G25 y GND
 * Lógica: Presionado = LOW, Soltado = HIGH
 */

const int buttonPin = 25;
int lastButtonState = HIGH;
int currentButtonState;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  Serial.println("Test de botón iniciado - Pin G25");
  Serial.println("Conecta el botón entre G25 y GND");
}

void loop() {
  int reading = digitalRead(buttonPin);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != currentButtonState) {
      currentButtonState = reading;

      if (currentButtonState == LOW) {
        Serial.println("Botón PRESIONADO");
      } else {
        Serial.println("Botón SOLTADO");
      }
    }
  }

  lastButtonState = reading;
}