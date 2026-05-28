#include <Arduino.h>   // Incluye la librería estándar de Arduino para PlatformIO (define HIGH, LOW, Serial, etc.)

/*
 * Test de botón para ESP32
 * Pin: GPIO25 (G25)
 * Conexión: Botón entre G25 y GND
 * Lógica: Presionado = LOW, Soltado = HIGH
 */

const int buttonPin = 25;          // Define el número del pin GPIO donde está conectado el botón (G25)
int lastButtonState = HIGH;        // Almacena el último estado conocido del botón (comienza en HIGH, reposo)
int currentButtonState;            // Almacena el estado actual del botón después del antirrebote
unsigned long lastDebounceTime = 0; // Guarda el momento (en ms) del último cambio de estado detectado
const unsigned long debounceDelay = 50; // Tiempo de antirrebote en milisegundos (ignora cambios menores a 50ms)

void setup() {                     // Función de configuración: se ejecuta una sola vez al inicio
  Serial.begin(115200);            // Inicia la comunicación serie a 115200 baudios (para ver mensajes en monitor)
  pinMode(buttonPin, INPUT_PULLUP); // Configura el pin como entrada con resistencia pull-up interna (estado HIGH por defecto)
  Serial.println("Test de botón iniciado - Pin G25"); // Imprime mensaje de inicio en el monitor serie
  Serial.println("Conecta el botón entre G25 y GND"); // Recuerda la conexión necesaria
}

void loop() {                      // Función principal: se ejecuta repetidamente mientras el ESP32 está encendido
  int reading = digitalRead(buttonPin); // Lee el valor actual del pin del botón (HIGH o LOW)

  if (reading != lastButtonState) {     // Si el valor leído es diferente al último estado registrado...
    lastDebounceTime = millis();        // ... reinicia el contador de tiempo de antirrebote (registra el momento actual)
  }

  if ((millis() - lastDebounceTime) > debounceDelay) { // Si ha pasado más tiempo que el retardo de rebote...
    if (reading != currentButtonState) {               // ... y el valor leído es diferente del estado estable actual...
      currentButtonState = reading;                    // ... actualiza el estado estable con la nueva lectura

      if (currentButtonState == LOW) {                 // Si el estado estable es LOW (botón presionado a tierra)...
        Serial.println("Botón PRESIONADO");            // ... imprime mensaje de presionado
      } else {                                         // De lo contrario (estado HIGH, botón soltado)...
        Serial.println("Botón SOLTADO");               // ... imprime mensaje de soltado
      }
    }
  }

  lastButtonState = reading;        // Guarda la lectura actual como último estado para la próxima iteración
}