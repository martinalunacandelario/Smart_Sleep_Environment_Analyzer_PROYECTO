#include <Wire.h>
#include <BH1750.h>


// Crear el objeto del sensor
BH1750 lightMeter;


// Definir los pines I2C para el ESP32 (valores por defecto)
#define I2C_SDA 21
#define I2C_SCL 22


void setup() {
 // Iniciar la comunicación serial a 115200 baudios
 Serial.begin(115200);
 Serial.println();
 Serial.println(F("--- Iniciando Prueba del Sensor BH1750 ---"));
 Serial.println(F("Este programa probara la conexion y lectura del sensor."));
  // Inicializar el bus I2C con los pines del ESP32
 Wire.begin(I2C_SDA, I2C_SCL);
 Serial.print(F("I2C iniciado en pines -> SDA: "));
 Serial.print(I2C_SDA);
 Serial.print(F(", SCL: "));
 Serial.println(I2C_SCL);


 // --- PRUEBA DE DETECCION (Escaneo I2C) ---
 Serial.println(F("\n[PASO 1]: Escaneando dispositivos I2C..."));
 byte error, address;
 int nDevices = 0;
  for(address = 1; address < 127; address++ ) {
   Wire.beginTransmission(address);
   error = Wire.endTransmission();
  
   if (error == 0) {
     Serial.print(F("  -> Dispositivo I2C encontrado en direccion 0x"));
     if (address<16) Serial.print("0");
     Serial.print(address, HEX);
    
     // Verificar si la dirección corresponde a la del BH1750 (0x23 o 0x5C)
     if (address == 0x23 || address == 0x5C) {
       Serial.println(F(" (POSIBLE BH1750)"));
     } else {
       Serial.println();
     }
     nDevices++;
   }
 }
  if (nDevices == 0) {
   Serial.println(F("  -> Error: No se encontraron dispositivos I2C."));
   Serial.println(F("     Por favor revisa las conexiones del sensor."));
 } else {
   Serial.print(F("  -> Total de dispositivos encontrados: "));
   Serial.println(nDevices);
 }


 // --- INICIALIZACION DEL SENSOR ---
 Serial.println(F("\n[PASO 2]: Inicializando sensor BH1750..."));
  // Intentar inicializar el sensor. begin() devuelve false si falla.
 if (lightMeter.begin()) {
   Serial.println(F("  -> BH1750 inicializado CORRECTAMENTE!"));
   Serial.println(F("     Modo de medicion: Alta resolucion (1 lx / 0.5 lx precision aprox)"));
 } else {
   Serial.println(F("  -> Error: No se pudo inicializar el BH1750."));
   Serial.println(F("     Verifica que el sensor este conectado correctamente."));
 }
  Serial.println(F("\n--- Iniciando lecturas continuas ---"));
 Serial.println(F("Leyendo cada 1 segundo. Apunta una linterna al sensor para probarlo.\n"));
 delay(2000);
}


void loop() {
 // Leer el valor de luz actual en lux
 uint16_t lux = lightMeter.readLightLevel();
  // Mostrar el resultado por el puerto serie
 Serial.print(F("Luz medida: "));
 Serial.print(lux);
 Serial.println(F(" lx"));
  // Pequeña ayuda visual en el monitor serie
 if (lux < 10) {
   Serial.println(F("  -> Muy oscuro. Tapa el sensor con la mano."));
 } else if (lux > 400) {
   Serial.println(F("  -> Muy brillante. Apunta una linterna o lleva el sensor cerca de la luz."));
 }
  Serial.println(F("-----------------------------------"));
  // Esperar 1 segundo antes de la siguiente lectura
 delay(1000);
}
#include <Wire.h>
#include <BH1750.h>


// Crear el objeto del sensor
BH1750 lightMeter;


// Definir los pines I2C para el ESP32 (valores por defecto)
#define I2C_SDA 21
#define I2C_SCL 22


void setup() {
 // Iniciar la comunicación serial a 115200 baudios
 Serial.begin(115200);
 Serial.println();
 Serial.println(F("--- Iniciando Prueba del Sensor BH1750 ---"));
 Serial.println(F("Este programa probara la conexion y lectura del sensor."));
  // Inicializar el bus I2C con los pines del ESP32
 Wire.begin(I2C_SDA, I2C_SCL);
 Serial.print(F("I2C iniciado en pines -> SDA: "));
 Serial.print(I2C_SDA);
 Serial.print(F(", SCL: "));
 Serial.println(I2C_SCL);


 // --- PRUEBA DE DETECCION (Escaneo I2C) ---
 Serial.println(F("\n[PASO 1]: Escaneando dispositivos I2C..."));
 byte error, address;
 int nDevices = 0;
  for(address = 1; address < 127; address++ ) {
   Wire.beginTransmission(address);
   error = Wire.endTransmission();
  
   if (error == 0) {
     Serial.print(F("  -> Dispositivo I2C encontrado en direccion 0x"));
     if (address<16) Serial.print("0");
     Serial.print(address, HEX);
    
     // Verificar si la dirección corresponde a la del BH1750 (0x23 o 0x5C)
     if (address == 0x23 || address == 0x5C) {
       Serial.println(F(" (POSIBLE BH1750)"));
     } else {
       Serial.println();
     }
     nDevices++;
   }
 }
  if (nDevices == 0) {
   Serial.println(F("  -> Error: No se encontraron dispositivos I2C."));
   Serial.println(F("     Por favor revisa las conexiones del sensor."));
 } else {
   Serial.print(F("  -> Total de dispositivos encontrados: "));
   Serial.println(nDevices);
 }


 // --- INICIALIZACION DEL SENSOR ---
 Serial.println(F("\n[PASO 2]: Inicializando sensor BH1750..."));
  // Intentar inicializar el sensor. begin() devuelve false si falla.
 if (lightMeter.begin()) {
   Serial.println(F("  -> BH1750 inicializado CORRECTAMENTE!"));
   Serial.println(F("     Modo de medicion: Alta resolucion (1 lx / 0.5 lx precision aprox)"));
 } else {
   Serial.println(F("  -> Error: No se pudo inicializar el BH1750."));
   Serial.println(F("     Verifica que el sensor este conectado correctamente."));
 }
  Serial.println(F("\n--- Iniciando lecturas continuas ---"));
 Serial.println(F("Leyendo cada 1 segundo. Apunta una linterna al sensor para probarlo.\n"));
 delay(2000);
}


void loop() {
 // Leer el valor de luz actual en lux
 uint16_t lux = lightMeter.readLightLevel();
  // Mostrar el resultado por el puerto serie
 Serial.print(F("Luz medida: "));
 Serial.print(lux);
 Serial.println(F(" lx"));
  // Pequeña ayuda visual en el monitor serie
 if (lux < 10) {
   Serial.println(F("  -> Muy oscuro. Tapa el sensor con la mano."));
 } else if (lux > 400) {
   Serial.println(F("  -> Muy brillante. Apunta una linterna o lleva el sensor cerca de la luz."));
 }
  Serial.println(F("-----------------------------------"));
  // Esperar 1 segundo antes de la siguiente lectura
 delay(1000);
}
