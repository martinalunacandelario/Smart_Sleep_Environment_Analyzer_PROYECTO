/**
 * TEST BUZZER PASIVO + TRANSISTOR (5V)
 * Conexiones:
 *   ESP32 GPIO26 → 1kΩ → Base (2N3904/PN2222)
 *   Emisor → GND
 *   Colector → (-) Buzzer Pasivo
 *   (+) Buzzer Pasivo → 5V (VIN del ESP32)
 
 */


#include <Arduino.h>


#define BUZZER_PIN 26


// Notas más agudas (una octava arriba para más volumen percibido)
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_D6  1175
#define NOTE_E6  1319


// Melodía "Para Elisa" en tonos más agudos
int melody[] = { NOTE_E6, NOTE_D6, NOTE_E6, NOTE_D6, NOTE_E6, NOTE_B5, NOTE_D6, NOTE_C6, NOTE_A5, NOTE_C5, NOTE_E5, NOTE_A5, NOTE_B5 };
int noteDurations[] = { 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 4 };


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== TEST BUZZER PASIVO (volumen mejorado) ===");


  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);


  // ------------------------------------------------------------
  // Prueba 1: tono más agudo (2500 Hz en lugar de 1000 Hz)
  // ------------------------------------------------------------
  Serial.println("\n--- 1. Encendido/apagado (2.5 kHz) ---");
  tone(BUZZER_PIN, 2500);
  delay(1000);
  noTone(BUZZER_PIN);
  delay(500);


  // ------------------------------------------------------------
  // Prueba 2: frecuencias agudas (2 kHz, 3 kHz, 4 kHz)
  // ------------------------------------------------------------
  Serial.println("\n--- 2. Frecuencias agudas ---");
  tone(BUZZER_PIN, 2000);
  delay(800);
  tone(BUZZER_PIN, 3000);
  delay(800);
  tone(BUZZER_PIN, 4000);
  delay(800);
  noTone(BUZZER_PIN);
  delay(500);


  // ------------------------------------------------------------
  // Prueba 3: Barrido de 1500 Hz a 4500 Hz (rango más audible)
  // ------------------------------------------------------------
  Serial.println("\n--- 3. Barrido de frecuencias (1500→4500 Hz) ---");
  for (int f = 1500; f <= 4500; f += 50) {
    tone(BUZZER_PIN, f);
    delay(8);
  }
  noTone(BUZZER_PIN);
  delay(500);


  // ------------------------------------------------------------
  // Prueba 4: Melodía (más aguda)
  // ------------------------------------------------------------
  Serial.println("\n--- 4. Melodía (Para Elisa aguda) ---");
  int numNotes = sizeof(melody) / sizeof(melody[0]);
  for (int i = 0; i < numNotes; i++) {
    int duration = 1000 / noteDurations[i];
    tone(BUZZER_PIN, melody[i]);
    delay(duration);
    noTone(BUZZER_PIN);
    delay(50);
  }
  delay(500);


  // ------------------------------------------------------------
  // Prueba 5: Sweep rápido (sube y baja en rango agudo)
  // ------------------------------------------------------------
  Serial.println("\n--- 5. Sweep sube/baja (2000→4000 Hz) ---");
  for (int f = 2000; f <= 4000; f += 20) {
    tone(BUZZER_PIN, f);
    delay(4);
  }
  for (int f = 4000; f >= 2000; f -= 20) {
    tone(BUZZER_PIN, f);
    delay(4);
  }
  noTone(BUZZER_PIN);
  delay(500);


  Serial.println("\n=== TEST COMPLETADO ===");
}


void loop() {
  delay(10000);
}
