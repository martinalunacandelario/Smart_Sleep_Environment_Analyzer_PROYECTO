/**
 * TEST INDIVIDUAL: Pantalla OLED SH1106 (I²C, 128x64 píxeles)
 * 
 * Conexiones (pines compartidos con SCD41):
 *   VCC → 3.3V
 *   GND → GND
 *   SDA → GPIO 21
 *   SCL → GPIO 22
 * 
 * Validaciones:
 *   - Inicialización y comunicación I²C con pines personalizados
 *   - Limpieza de pantalla
 *   - DIBUJO DE MÚLTIPLES FORMAS: cada 3 segundos cambia el patrón
 *   - Visualización de texto en diferentes tamaños y posiciones
 *   - Medición de tiempos de refresco (timing)
 *   - Actualización periódica (cada 3 segundos)
 */

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

// Definición de pines para el bus I2C (compartido con SCD41)
#define OLED_SDA 21
#define OLED_SCL 22

// Constructor del objeto U8g2: se pasa el bus Wire y los pines manualmente
U8G2_SH1106_128X64_NONAME_1_HW_I2C oled(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ OLED_SCL, /* data=*/ OLED_SDA);

unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL_MS = 3000;  // Actualizar cada 3 segundos
int frameCounter = 0;

// Función que dibuja una pantalla diferente según el número de frame (0..3)
void drawScreen(int mode) {
  oled.firstPage();
  do {
    // Borramos el contenido anterior (sobreescribimos todo)
    oled.clearDisplay();   // Asegura que no queden restos de frames anteriores
    
    // Siempre dibujamos un marco exterior común
    oled.drawFrame(0, 0, 128, 64);
    oled.setFont(u8g2_font_6x10_tf);
    oled.drawStr(2, 10, "TEST OLED SH1106");
    
    // Mostramos el modo actual en la esquina superior derecha
    char modeStr[16];
    sprintf(modeStr, "Modo %d", mode);
    oled.drawStr(90, 10, modeStr);
    
    // Dibujamos diferentes patrones según el modo
    switch (mode % 4) {
      case 0:
        // MODO 0: cuadrícula de puntos
        for (int x = 0; x < 128; x += 8) {
          for (int y = 20; y < 64; y += 8) {
            oled.drawPixel(x, y);
          }
        }
        oled.setFont(u8g2_font_helvB08_tf);
        oled.drawStr(10, 30, "Patron: Pixeles");
        break;
        
      case 1:
        // MODO 1: líneas diagonales
        for (int i = 0; i < 4; i++) {
          oled.drawLine(0, 20 + i*10, 127, 60 - i*8);
          oled.drawLine(127, 20 + i*10, 0, 60 - i*8);
        }
        oled.setFont(u8g2_font_helvB08_tf);
        oled.drawStr(10, 30, "Patron: Lineas");
        break;
        
      case 2:
        // MODO 2: rectángulos concéntricos
        for (int r = 0; r < 5; r++) {
          oled.drawFrame(20 + r*8, 20 + r*4, 88 - r*16, 30 - r*8);
        }
        oled.setFont(u8g2_font_helvB08_tf);
        oled.drawStr(10, 30, "Patron: Rectangulos");
        break;
        
      case 3:
        // MODO 3: círculos y texto
        oled.drawCircle(64, 38, 20, U8G2_DRAW_ALL);
        oled.drawCircle(64, 38, 12, U8G2_DRAW_ALL);
        oled.setFont(u8g2_font_helvB08_tf);
        oled.drawStr(10, 30, "Patron: Circulos");
        oled.drawStr(50, 55, "OK");
        break;
    }
    
    // Mostramos el tiempo de refresco (aproximado, se mide fuera)
    char buf[20];
    sprintf(buf, "Frame: %d", frameCounter);
    oled.drawStr(2, 62, buf);
    
  } while (oled.nextPage());
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== TEST OLED SH1106 ===");
  Serial.println("Conexiones: VCC→3.3V, GND→GND, SDA→GPIO21, SCL→GPIO22");
  
  // Inicializar el bus I2C con los pines personalizados
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);    // 400 kHz es seguro para OLED
  
  // Inicializar la pantalla OLED
  oled.begin();
  Serial.println("OLED inicializado correctamente.");
  
  // Limpiar pantalla
  oled.clearBuffer();
  oled.sendBuffer();
  delay(500);
  
  // Mostrar primera pantalla
  frameCounter = 0;
  drawScreen(frameCounter);
  Serial.println("Primera pantalla dibujada.");
  
  // Medir tiempo de dibujo completo (timing)
  unsigned long start = micros();
  drawScreen(0);
  unsigned long elapsed = micros() - start;
  Serial.printf("Tiempo de dibujo (incluyendo I2C): %lu us\n", elapsed);
  Serial.println("El test se ejecutará actualizando la pantalla cada 3 segundos.\n");
}

void loop() {
  unsigned long now = millis();
  if (now - lastUpdate >= UPDATE_INTERVAL_MS) {
    lastUpdate = now;
    frameCounter++;
    
    // Medir tiempo de esta actualización
    unsigned long start = micros();
    drawScreen(frameCounter % 4);   // Cambiamos el modo cíclicamente (0,1,2,3,0...)
    unsigned long elapsed = micros() - start;
    
    // Información por serie
    Serial.printf("Actualización %d (modo %d): tiempo = %lu us\n", 
                  frameCounter, frameCounter % 4, elapsed);
    
    if (elapsed < 100000) {
      Serial.println("✓ Timing correcto (refresco rápido).");
    } else {
      Serial.println("⚠️ Refresco lento (>100 ms). Revisar bus I2C.");
    }
  }
  delay(10);
}