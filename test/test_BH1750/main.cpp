/*
 * ============================================================
 * TEST SENSOR BH1750
 * ============================================================
 * Propósito: Verificar el correcto funcionamiento del sensor de luz BH1750
 *            conectado a un ESP32 mediante bus I2C.
 * 
 * Lo que busca este test:
 *   - Detectar si el sensor está presente en el bus I2C (direcciones 0x23 o 0x5C)
 *   - Inicializar el sensor correctamente
 *   - Leer y mostrar valores de luminosidad en lux cada segundo
 *   - Diagnosticar posibles fallos de conexión
 * 
 * Pines utilizados (ESP32):
 *   - SDA -> GPIO 21
 *   - SCL -> GPIO 22
 * 
 * Comunicación: Serie a 115200 baudios para ver resultados en monitor
 * ============================================================
 */

#include <Wire.h>      // Librería para comunicación I2C
#include <BH1750.h>    // Librería específica del sensor BH1750

// Crear el objeto del sensor
BH1750 lightMeter;     // Instancia del sensor BH1750

// Definir los pines I2C para el ESP32 (valores por defecto)
#define I2C_SDA 21      // Pin de datos I2C (ESP32)
#define I2C_SCL 22      // Pin de reloj I2C (ESP32)

void setup() {
 // Iniciar la comunicación serial a 115200 baudios
 Serial.begin(115200);         // Inicia la comunicación serie a 115200 bps
 Serial.println();              // Línea en blanco para separar del ruido inicial
 Serial.println(F("--- Iniciando Prueba del Sensor BH1750 ---"));  // Mensaje inicio prueba
 Serial.println(F("Este programa probara la conexion y lectura del sensor.")); // Explicación

 // Inicializar el bus I2C con los pines del ESP32
 Wire.begin(I2C_SDA, I2C_SCL); // Inicializa el bus I2C con los pines definidos
 Serial.print(F("I2C iniciado en pines -> SDA: "));  // Texto informativo
 Serial.print(I2C_SDA);        // Muestra el pin SDA usado
 Serial.print(F(", SCL: "));    // Texto
 Serial.println(I2C_SCL);       // Muestra el pin SCL usado

 // --- PRUEBA DE DETECCION (Escaneo I2C) ---
 Serial.println(F("\n[PASO 1]: Escaneando dispositivos I2C...")); // Indicador de paso
 byte error, address;           // Variables: error para resultado de transmisión, address para dirección I2C
 int nDevices = 0;              // Contador de dispositivos encontrados

 for(address = 1; address < 127; address++ ) {   // Recorre direcciones I2C válidas (1 a 126)
   Wire.beginTransmission(address);   // Inicia comunicación con la dirección actual
   error = Wire.endTransmission();    // Finaliza transmisión y devuelve 0 si hubo respuesta

   if (error == 0) {                  // Si hay un dispositivo en esa dirección
     Serial.print(F("  -> Dispositivo I2C encontrado en direccion 0x")); // Mensaje
     if (address<16) Serial.print("0");  // Añade cero para formato de dos dígitos
     Serial.print(address, HEX);       // Imprime dirección en hexadecimal

     // Verificar si la dirección corresponde a la del BH1750 (0x23 o 0x5C)
     if (address == 0x23 || address == 0x5C) {  // Direcciones típicas del BH1750
       Serial.println(F(" (POSIBLE BH1750)"));  // Indica que podría ser el sensor buscado
     } else {
       Serial.println();               // Solo salto de línea si no es BH1750
     }
     nDevices++;                       // Incrementa contador de dispositivos
   }
 }

 if (nDevices == 0) {                  // Si no se encontró ningún dispositivo
   Serial.println(F("  -> Error: No se encontraron dispositivos I2C.")); // Error
   Serial.println(F("     Por favor revisa las conexiones del sensor.")); // Sugerencia
 } else {                              // Si hay al menos un dispositivo
   Serial.print(F("  -> Total de dispositivos encontrados: ")); // Mensaje
   Serial.println(nDevices);           // Muestra cantidad total
 }

 // --- INICIALIZACION DEL SENSOR ---
 Serial.println(F("\n[PASO 2]: Inicializando sensor BH1750...")); // Paso 2
 // Intentar inicializar el sensor. begin() devuelve false si falla.
 if (lightMeter.begin()) {             // Intenta inicializar el sensor (usa direcciones por defecto)
   Serial.println(F("  -> BH1750 inicializado CORRECTAMENTE!")); // Éxito
   Serial.println(F("     Modo de medicion: Alta resolucion (1 lx / 0.5 lx precision aprox)")); // Modo configurado
 } else {                              // Si falla la inicialización
   Serial.println(F("  -> Error: No se pudo inicializar el BH1750.")); // Error
   Serial.println(F("     Verifica que el sensor este conectado correctamente.")); // Recomendación
 }

 Serial.println(F("\n--- Iniciando lecturas continuas ---")); // Separador
 Serial.println(F("Leyendo cada 1 segundo. Apunta una linterna al sensor para probarlo.\n")); // Instrucciones
 delay(2000);                          // Espera 2 segundos antes de empezar lecturas
}

void loop() {
 // Leer el valor de luz actual en lux
 uint16_t lux = lightMeter.readLightLevel();  // Realiza la medición y guarda en lux (rango 0-65535)

 // Mostrar el resultado por el puerto serie
 Serial.print(F("Luz medida: "));      // Texto
 Serial.print(lux);                    // Muestra el valor numérico
 Serial.println(F(" lx"));             // Unidad (lux)

 // Pequeña ayuda visual en el monitor serie
 if (lux < 10) {                       // Si muy poca luz
   Serial.println(F("  -> Muy oscuro. Tapa el sensor con la mano.")); // Mensaje sugerencia
 } else if (lux > 400) {               // Si mucha luz
   Serial.println(F("  -> Muy brillante. Apunta una linterna o lleva el sensor cerca de la luz.")); // Sugerencia
 }

 Serial.println(F("-----------------------------------")); // Línea separadora
 delay(1000);                          // Espera 1 segundo antes de la siguiente lectura
}