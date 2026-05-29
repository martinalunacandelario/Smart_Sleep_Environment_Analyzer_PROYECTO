#include <Arduino.h>               // Incluye la librería estándar de Arduino (para Serial, delay, etc.)
#include <U8g2lib.h>              // Incluye la librería U8g2 para manejar la pantalla OLED
#include <Wire.h>                 // Incluye la librería Wire para comunicación I2C

/*
 * TEST OLED con controlador SH1106 vía I2C
 * Conexiones:
 *   OLED VCC → ESP32 3.3V
 *   OLED GND → ESP32 GND
 *   OLED SDA → GPIO21
 *   OLED SCL → GPIO22
 * 
 * Este test incluye múltiples pruebas: texto, fuentes, formas,
 * barra de progreso, inversión, contador, scroll y power save.
 */

// Constructor para SH1106 128x64 con I2C por hardware
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
// Crea un objeto u8g2 para SH1106 de 128x64 píxeles, sin pin de reset (uso I2C por hardware)

// Definir los pines I2C que vamos a usar
const int I2C_SDA = 21;           // Pin SDA (datos I2C) conectado a GPIO21
const int I2C_SCL = 22;           // Pin SCL (reloj I2C) conectado a GPIO22

// Variables para controlar las pruebas
int testStage = 0;                // Prueba actual (0-9) - comienza en 0
unsigned long lastTestChange = 0; // Momento (millis) del último cambio de prueba
bool testRunning = true;          // Controla si el test sigue activo (true = ejecutando)

// ========== PROTOTIPOS DE FUNCIONES (declaraciones anticipadas) ==========
void testClearAndText();          // Declara la función testClearAndText (se define más abajo)
void testDifferentFonts();        // Declara la función testDifferentFonts
void testShapes();                // Declara la función testShapes
void testProgressBar();           // Declara la función testProgressBar
void testInversion();             // Declara la función testInversion
void testCounter();               // Declara la función testCounter
void testScroll();                // Declara la función testScroll
void testPowerSave();             // Declara la función testPowerSave
void testBitmap();                // Declara la función testBitmap
void testAllFeatures();           // Declara la función testAllFeatures
void finalScreen();               // Declara la función finalScreen (pantalla de fin)
// =========================================================================

void setup() {                    // Función de configuración (se ejecuta una vez al inicio)
  Serial.begin(115200);           // Inicia la comunicación serie a 115200 baudios
  delay(1000);                    // Espera 1 segundo para estabilizar el monitor serie
  Serial.println("\n=== TEST OLED SH1106 ==="); // Imprime título en el monitor

  // Inicializar el bus I2C en los pines que hemos definido
  Wire.begin(I2C_SDA, I2C_SCL);   // Inicia el bus I2C con los pines SDA=21 y SCL=22
  Serial.println("I2C iniciado en pines SDA=21, SCL=22"); // Mensaje de confirmación

  // Inicializar la pantalla OLED
  u8g2.begin();                   // Inicia la comunicación con la pantalla OLED
  u8g2.enableUTF8Print();         // Habilita caracteres UTF-8 (tildes, ñ, etc.)
  u8g2.setContrast(255);          // Configura el contraste al máximo (valor 0-255)
  u8g2.setFlipMode(0);            // Sin rotación (modo normal, no invertido)

  Serial.println("Pantalla SH1106 iniciada correctamente"); // Mensaje por serie

  // Mensaje de bienvenida en la pantalla
  u8g2.clearBuffer();             // Limpia el buffer interno de la pantalla (todo negro)
  u8g2.setFont(u8g2_font_ncenB12_tr);  // Selecciona una fuente de tamaño 12 (negrita)
  u8g2.drawStr(20, 20, "Test OLED");   // Dibuja el texto "Test OLED" en coordenadas x=20, y=20
  u8g2.drawStr(20, 40, "SH1106");      // Dibuja "SH1106" en x=20, y=40
  u8g2.setFont(u8g2_font_ncenB08_tr);  // Cambia a una fuente más pequeña (tamaño 8)
  u8g2.drawStr(20, 58, "Iniciando...");// Dibuja "Iniciando..." en x=20, y=58
  u8g2.sendBuffer();              // Envía el buffer a la pantalla (muestra lo dibujado)

  delay(2000);                    // Pausa 2 segundos para que se pueda leer el mensaje inicial
}

void loop() {                     // Función principal (se ejecuta repetidamente)
  // Si el test ha finalizado, no hacemos nada
  if (!testRunning) return;       // Si testRunning es false, sale de loop() inmediatamente

  // Cambiar de prueba cada 8 segundos
  if (millis() - lastTestChange > 8000) { // Si han pasado más de 8000 ms desde el último cambio
    lastTestChange = millis();    // Actualiza el tiempo del último cambio al momento actual
    testStage++;                  // Incrementa el número de prueba (pasa a la siguiente)
    // Si se han completado todas las pruebas (0 a 9), finalizar
    if (testStage >= 10) {        // Si ya se hicieron 10 pruebas (0..9)
      testRunning = false;        // Detiene el test (cambia la bandera a false)
      finalScreen();              // Llama a la función que muestra la pantalla de finalización
      return;                    // Sale de loop() (no continúa)
    }
    Serial.print("Prueba ");     // Imprime "Prueba " en el monitor serie
    Serial.println(testStage);   // Imprime el número de prueba actual (1..10)
  }

  // Ejecutar la prueba correspondiente según el valor de testStage
  switch (testStage) {            // Selecciona el caso según testStage
    case 0: testClearAndText(); break;   // Prueba 0: texto básico
    case 1: testDifferentFonts(); break; // Prueba 1: diferentes fuentes
    case 2: testShapes(); break;         // Prueba 2: formas geométricas
    case 3: testProgressBar(); break;    // Prueba 3: barra de progreso
    case 4: testInversion(); break;      // Prueba 4: inversión de colores
    case 5: testCounter(); break;        // Prueba 5: contador numérico
    case 6: testScroll(); break;         // Prueba 6: scroll (desplazamiento)
    case 7: testPowerSave(); break;      // Prueba 7: ahorro de energía (apagado/encendido)
    case 8: testBitmap(); break;         // Prueba 8: mapa de bits (dibujo de píxeles)
    case 9: testAllFeatures(); break;    // Prueba 9: combinación final
  }
}

// --- Definición de cada prueba ---

// Prueba 0: Texto básico y limpieza
void testClearAndText() {                // Definición de la función testClearAndText
  u8g2.clearBuffer();                   // Limpia el buffer de la pantalla
  u8g2.setFont(u8g2_font_ncenB10_tr);   // Selecciona fuente de tamaño 10 (negrita)
  u8g2.drawStr(10, 20, "Test 1: Texto");// Dibuja "Test 1: Texto" en (10,20)
  u8g2.setFont(u8g2_font_ncenB08_tr);   // Cambia a fuente de tamaño 8
  u8g2.drawStr(10, 40, "SH1106 OK");    // Dibuja "SH1106 OK" en (10,40)
  u8g2.drawStr(10, 58, "SDA=21, SCL=22"); // Dibuja "SDA=21, SCL=22" en (10,58)
  u8g2.sendBuffer();                    // Envía el buffer a la pantalla (muestra)
}

// Prueba 1: Diferentes estilos de fuente
void testDifferentFonts() {              // Definición de la función testDifferentFonts
  u8g2.clearBuffer();                   // Limpia el buffer
  u8g2.setFont(u8g2_font_ncenB08_tr);   // Fuente tamaño 8
  u8g2.drawStr(5, 12, "Fuente pequena");// Dibuja texto en (5,12)
  u8g2.setFont(u8g2_font_ncenB10_tr);   // Fuente tamaño 10
  u8g2.drawStr(5, 28, "Fuente media");  // Dibuja texto en (5,28)
  u8g2.setFont(u8g2_font_ncenB14_tr);   // Fuente tamaño 14
  u8g2.drawStr(5, 46, "Fuente grande"); // Dibuja texto en (5,46)
  u8g2.setFont(u8g2_font_courB18_tn);   // Fuente monoespaciada tamaño 18
  u8g2.drawStr(5, 62, "Monoespacio");   // Dibuja texto en (5,62)
  u8g2.sendBuffer();                    // Envía a la pantalla
}

// Prueba 2: Dibujo de formas geométricas básicas
void testShapes() {                     // Definición de la función testShapes
  u8g2.clearBuffer();                   // Limpia el buffer
  u8g2.drawLine(0, 10, 128, 10);        // Dibuja una línea desde (0,10) hasta (128,10) (horizontal)
  u8g2.drawFrame(10, 20, 30, 20);       // Dibuja un rectángulo vacío (solo borde) en (10,20) de ancho 30, alto 20
  u8g2.drawBox(50, 20, 30, 20);         // Dibuja un rectángulo relleno en (50,20) de ancho 30, alto 20
  u8g2.drawCircle(100, 30, 10);         // Dibuja un círculo vacío de radio 10 centrado en (100,30)
  u8g2.drawDisc(100, 55, 8);            // Dibuja un círculo relleno de radio 8 centrado en (100,55)
  u8g2.drawTriangle(20, 50, 40, 60, 30, 55); // Dibuja un triángulo con vértices (20,50), (40,60), (30,55)
  u8g2.sendBuffer();                    // Envía a la pantalla
}

// Prueba 3: Barra de progreso animada
void testProgressBar() {                // Definición de la función testProgressBar
  static int progress = 0;              // Variable estática: progreso actual (0-100)
  static unsigned long lastUpdate = 0;  // Variable estática: tiempo de la última actualización
  // Actualizar la barra cada 50 ms
  if (millis() - lastUpdate > 50) {     // Si han pasado más de 50 ms
    lastUpdate = millis();              // Actualiza el tiempo de última actualización
    progress = (progress + 2) % 101;    // Incrementa progreso en 2, y lo mantiene entre 0 y 100
    u8g2.clearBuffer();                // Limpia el buffer
    u8g2.setFont(u8g2_font_ncenB08_tr); // Fuente pequeña
    u8g2.drawStr(25, 20, "Barra de progreso"); // Texto "Barra de progreso" en (25,20)
    u8g2.drawFrame(10, 30, 108, 10);    // Dibuja marco de barra en (10,30) ancho 108, alto 10
    u8g2.drawBox(10, 30, (progress * 108) / 100, 10); // Rellena según porcentaje
    u8g2.setCursor(50, 58);            // Posiciona el cursor en (50,58) para texto
    u8g2.print(progress);              // Imprime el número de progreso
    u8g2.print("%");                   // Imprime el símbolo de porcentaje
    u8g2.sendBuffer();                 // Envía a la pantalla
  }
}

// Prueba 4: Simular inversión de pantalla (fondo negro, texto blanco)
void testInversion() {                 // Definición de la función testInversion
  static bool inverted = false;        // Estado actual (invertido o no)
  static unsigned long lastToggle = 0; // Tiempo del último cambio
  if (millis() - lastToggle > 1000) {  // Si han pasado más de 1000 ms (1 segundo)
    lastToggle = millis();             // Actualiza el tiempo
    inverted = !inverted;              // Invierte el estado
    u8g2.clearBuffer();               // Limpia el buffer
    if (inverted) {                    // Si está invertido
      u8g2.setDrawColor(1);            // Color blanco (para dibujar)
      u8g2.drawBox(0, 0, 128, 64);     // Rellena toda la pantalla de blanco
      u8g2.setDrawColor(0);            // Cambia a color negro (para el texto)
      u8g2.setFont(u8g2_font_ncenB12_tr); // Fuente tamaño 12
      u8g2.drawStr(20, 32, "INVERTIDO"); // Dibuja "INVERTIDO" en (20,32)
    } else {                          // Si no está invertido
      u8g2.setDrawColor(1);            // Color blanco
      u8g2.setFont(u8g2_font_ncenB12_tr); // Fuente tamaño 12
      u8g2.drawStr(20, 32, "NORMAL");  // Dibuja "NORMAL" en (20,32)
    }
    u8g2.sendBuffer();                // Envía a la pantalla
    u8g2.setDrawColor(1);             // Restaura color blanco por defecto
  }
}

// Prueba 5: Contador numérico
void testCounter() {                   // Definición de la función testCounter
  static int cnt = 0;                 // Variable estática para el contador
  static unsigned long lastInc = 0;    // Tiempo del último incremento
  if (millis() - lastInc > 200) {      // Si han pasado más de 200 ms
    lastInc = millis();               // Actualiza el tiempo
    cnt = (cnt + 1) % 1000;           // Incrementa el contador en 1 (hasta 999, luego vuelve a 0)
    u8g2.clearBuffer();              // Limpia el buffer
    u8g2.setFont(u8g2_font_ncenB18_tr); // Fuente grande (tamaño 18)
    u8g2.setCursor(30, 40);          // Posiciona cursor en (30,40)
    u8g2.print("Cnt:");              // Imprime "Cnt:"
    u8g2.setCursor(80, 40);          // Posiciona cursor en (80,40)
    u8g2.print(cnt);                 // Imprime el valor del contador
    u8g2.setFont(u8g2_font_ncenB08_tr); // Fuente pequeña
    u8g2.drawStr(10, 60, "Incrementa cada 200ms"); // Texto explicativo
    u8g2.sendBuffer();               // Envía a la pantalla
  }
}

// Prueba 6: Scroll (desplazamiento) horizontal y vertical
void testScroll() {                  // Definición de la función testScroll
  static int scrollX = 0;            // Posición del scroll horizontal (desplazamiento)
  static unsigned long lastScroll = 0; // Tiempo del último scroll
  if (millis() - lastScroll > 30) {   // Cada 30 ms
    lastScroll = millis();           // Actualiza el tiempo
    scrollX = (scrollX + 1) % 256;   // Incrementa scrollX (0-255)
    u8g2.clearBuffer();             // Limpia el buffer
    u8g2.setFont(u8g2_font_ncenB12_tr); // Fuente tamaño 12
    u8g2.drawStr(10 - scrollX, 30, "Scroll horizontal"); // Mueve el texto hacia la izquierda
    u8g2.drawStr(10, 50 + (scrollX % 20), "Scroll vertical"); // Mueve el texto verticalmente
    u8g2.sendBuffer();              // Envía a la pantalla
  }
}

// Prueba 7: Ahorro de energía (apagado y encendido de la pantalla)
void testPowerSave() {               // Definición de la función testPowerSave
  static bool on = true;             // Estado de la pantalla (encendida/apagada)
  static unsigned long lastToggle = 0; // Tiempo del último cambio
  if (millis() - lastToggle > 1500) { // Cada 1.5 segundos
    lastToggle = millis();           // Actualiza el tiempo
    on = !on;                       // Invierte el estado
    if (on) {                       // Si debe encender
      u8g2.setPowerSave(0);         // Apaga el modo de ahorro de energía (pantalla ON)
      u8g2.clearBuffer();           // Limpia el buffer
      u8g2.setFont(u8g2_font_ncenB12_tr); // Fuente tamaño 12
      u8g2.drawStr(20, 32, "Pantalla ON"); // Dibuja "Pantalla ON"
      u8g2.sendBuffer();            // Envía a la pantalla
    } else {                        // Si debe apagar
      u8g2.setPowerSave(1);         // Activa el modo de ahorro (pantalla OFF)
    }
  }
}

// Prueba 8: Mostrar un pequeño gráfico de mapa de bits (un rectángulo y un corazón simple)
void testBitmap() {                  // Definición de la función testBitmap
  static unsigned long lastUpdate = 0; // Tiempo de la última actualización
  if (millis() - lastUpdate > 1000) { // Cada 1 segundo
    lastUpdate = millis();           // Actualiza el tiempo
    u8g2.clearBuffer();             // Limpia el buffer
    u8g2.setFont(u8g2_font_ncenB08_tr); // Fuente pequeña
    u8g2.drawStr(40, 15, "Prueba 8"); // Texto "Prueba 8" en (40,15)
    u8g2.drawStr(30, 28, "Bitmap"); // Texto "Bitmap" en (30,28)
    // Dibujar un corazón simple con píxeles
    u8g2.drawPixel(64, 35);         // Dibuja un píxel en (64,35)
    u8g2.drawPixel(66, 35);         // Dibuja un píxel en (66,35)
    u8g2.drawPixel(65, 36);         // Dibuja un píxel en (65,36)
    u8g2.drawPixel(64, 37);         // Dibuja un píxel en (64,37)
    u8g2.drawPixel(65, 37);         // Dibuja un píxel en (65,37)
    u8g2.drawPixel(66, 37);         // Dibuja un píxel en (66,37)
    u8g2.drawPixel(63, 36);         // Dibuja un píxel en (63,36)
    u8g2.drawPixel(67, 36);         // Dibuja un píxel en (67,36)
    u8g2.drawPixel(65, 38);         // Dibuja un píxel en (65,38)
    u8g2.drawStr(55, 55, "❤️");      // Dibuja un carácter de corazón (Unicode) en (55,55)
    u8g2.sendBuffer();              // Envía a la pantalla
  }
}

// Prueba 9: Combinación final de todas las funcionalidades
void testAllFeatures() {             // Definición de la función testAllFeatures
  static int step = 0;              // Paso actual (0-3)
  static unsigned long lastStep = 0; // Tiempo del último cambio de paso
  if (millis() - lastStep > 1500) {  // Cada 1.5 segundos
    lastStep = millis();            // Actualiza el tiempo
    step = (step + 1) % 4;          // Cambia al siguiente paso (0,1,2,3, luego vuelve a 0)
    u8g2.clearBuffer();            // Limpia el buffer
    switch (step) {                 // Según el paso
      case 0:                       // Paso 0
        u8g2.setFont(u8g2_font_ncenB14_tr); // Fuente tamaño 14
        u8g2.drawStr(25, 35, "Test OK");    // Dibuja "Test OK" en (25,35)
        break;
      case 1:                       // Paso 1
        u8g2.drawCircle(64, 32, 20); // Dibuja círculo vacío radio 20 en (64,32)
        u8g2.drawCircle(64, 32, 15); // Dibuja círculo vacío radio 15
        u8g2.drawCircle(64, 32, 10); // Dibuja círculo vacío radio 10
        break;
      case 2:                       // Paso 2
        u8g2.setFont(u8g2_font_ncenB08_tr); // Fuente pequeña
        u8g2.drawStr(5, 20, "Test completado"); // Texto en (5,20)
        u8g2.drawStr(5, 40, "con exito");      // Texto en (5,40)
        break;
      case 3:                       // Paso 3
        u8g2.drawBox(0, 0, 128, 64); // Rellena toda la pantalla de blanco
        u8g2.setDrawColor(0);        // Cambia a negro
        u8g2.setFont(u8g2_font_ncenB12_tr); // Fuente tamaño 12
        u8g2.drawStr(40, 35, "FIN"); // Dibuja "FIN" en (40,35)
        u8g2.setDrawColor(1);        // Restaura color blanco
        break;
    }
    u8g2.sendBuffer();              // Envía a la pantalla
  }
}

// Pantalla final cuando se completan todas las pruebas
void finalScreen() {                 // Definición de la función finalScreen
  u8g2.clearBuffer();               // Limpia el buffer
  u8g2.setFont(u8g2_font_ncenB14_tr); // Fuente tamaño 14
  u8g2.drawStr(10, 30, "PRUEBA");   // Dibuja "PRUEBA" en (10,30)
  u8g2.drawStr(10, 50, "COMPLETADA"); // Dibuja "COMPLETADA" en (10,50)
  u8g2.setFont(u8g2_font_ncenB08_tr); // Fuente pequeña
  u8g2.drawStr(10, 62, "Reinicia para repetir"); // Texto explicativo en (10,62)
  u8g2.sendBuffer();                // Envía a la pantalla
  Serial.println("Test finalizado. Pantalla muestra mensaje de fin."); // Mensaje por serie
}