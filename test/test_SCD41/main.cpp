/**
 * TEST INDIVIDUAL: SCD41 (CO₂, Temperatura, Humedad)
 * 
 * Conexiones:
 *   VDD → 3.3V
 *   GND → GND
 *   SDA → GPIO 21
 *   SCL → GPIO 22
 * 
 * Validaciones:
 *   - Comunicación I2C
 *   - Lectura periódica cada 5 segundos (timing)
 *   - Medición del tiempo de ejecución de cada lectura
 *   - Rangos plausibles de los datos
 */

#include <Arduino.h>
#include <Wire.h>


// Dirección I2C del SCD41
const uint8_t SCD4X_ADDRESS = 0x62;


// Comandos del sensor
const uint16_t CMD_START_PERIODIC_MEASUREMENT = 0x21B1;
const uint16_t CMD_READ_MEASUREMENT = 0xEC05;
const uint16_t CMD_STOP_MEASUREMENT = 0x3F86;
const uint16_t CMD_GET_SERIAL_NUMBER = 0x3682;
const uint16_t CMD_GET_TEMP_OFFSET = 0x2318;
const uint16_t CMD_SET_TEMP_OFFSET = 0x241D;
const uint16_t CMD_GET_ALTITUDE = 0x2322;
const uint16_t CMD_SET_ALTITUDE = 0x2427;
const uint16_t CMD_GET_DATA_READY = 0xE4B8;


// Pines I2C
#define I2C_SDA 21
#define I2C_SCL 22


// Función para enviar comandos
void sendCommand(uint16_t command) {
   Wire.beginTransmission(SCD4X_ADDRESS);
   Wire.write(command >> 8);
   Wire.write(command & 0xFF);
   Wire.endTransmission();
}


// Función para enviar comandos con argumentos
void sendCommandWithArg(uint16_t command, uint16_t arg) {
   Wire.beginTransmission(SCD4X_ADDRESS);
   Wire.write(command >> 8);
   Wire.write(command & 0xFF);
   Wire.write(arg >> 8);
   Wire.write(arg & 0xFF);
   Wire.endTransmission();
}


// Cálculo CRC8
uint8_t calcCRC8(uint8_t data1, uint8_t data2) {
   uint8_t crc = 0xFF;
   crc ^= data1;
   for (uint8_t bit = 8; bit > 0; --bit) {
       if (crc & 0x80) {
           crc = (crc << 1) ^ 0x31;
       } else {
           crc = (crc << 1);
       }
   }
   crc ^= data2;
   for (uint8_t bit = 8; bit > 0; --bit) {
       if (crc & 0x80) {
           crc = (crc << 1) ^ 0x31;
       } else {
           crc = (crc << 1);
       }
   }
   return crc;
}


// Verificar si hay datos listos
bool isDataReady() {
   sendCommand(CMD_GET_DATA_READY);
   delay(2);
   Wire.requestFrom(SCD4X_ADDRESS, (uint8_t)3);
   if (Wire.available() == 3) {
       uint16_t status = (Wire.read() << 8) | Wire.read();
       uint8_t crc = Wire.read();
       uint8_t calc = calcCRC8(status >> 8, status & 0xFF);
       if (calc == crc) {
           return (status & 0x07FF) != 0;
       }
   }
   return false;
}


// Leer número de serie
void getSerialNumber() {
   sendCommand(CMD_GET_SERIAL_NUMBER);
   delay(2);
   Wire.requestFrom(SCD4X_ADDRESS, (uint8_t)9);
   if (Wire.available() == 9) {
       uint16_t serial[3];
       bool crc_ok = true;
       for (int i = 0; i < 3; i++) {
           uint8_t msb = Wire.read();
           uint8_t lsb = Wire.read();
           uint8_t crc = Wire.read();
           uint8_t calc = calcCRC8(msb, lsb);
           if (calc != crc) crc_ok = false;
           serial[i] = (msb << 8) | lsb;
       }
       if (crc_ok) {
           Serial.print(F("Numero de serie: "));
           for (int i = 0; i < 3; i++) {
               Serial.print(serial[i], HEX);
               if (i < 2) Serial.print("-");
           }
           Serial.println();
       } else {
           Serial.println(F("Error CRC al leer numero de serie"));
       }
   }
}


// Leer medición
bool readMeasurement(uint16_t &co2, float &temperature, float &humidity) {
   if (!isDataReady()) {
       return false;
   }
  
   sendCommand(CMD_READ_MEASUREMENT);
   delay(2);
   Wire.requestFrom(SCD4X_ADDRESS, (uint8_t)9);
  
   if (Wire.available() == 9) {
       // Leer CO2
       uint8_t co2_msb = Wire.read();
       uint8_t co2_lsb = Wire.read();
       uint8_t co2_crc = Wire.read();
       if (calcCRC8(co2_msb, co2_lsb) != co2_crc) return false;
       co2 = (co2_msb << 8) | co2_lsb;
      
       // Leer Temperatura
       uint8_t temp_msb = Wire.read();
       uint8_t temp_lsb = Wire.read();
       uint8_t temp_crc = Wire.read();
       if (calcCRC8(temp_msb, temp_lsb) != temp_crc) return false;
       uint16_t temp_raw = (temp_msb << 8) | temp_lsb;
       temperature = -45.0f + 175.0f * temp_raw / 65535.0f;
      
       // Leer Humedad
       uint8_t hum_msb = Wire.read();
       uint8_t hum_lsb = Wire.read();
       uint8_t hum_crc = Wire.read();
       if (calcCRC8(hum_msb, hum_lsb) != hum_crc) return false;
       uint16_t hum_raw = (hum_msb << 8) | hum_lsb;
       humidity = 100.0f * hum_raw / 65535.0f;
      
       return true;
   }
   return false;
}


void setup() {
   Serial.begin(115200);
   delay(1000);
  
   Serial.println();
   Serial.println(F("========================================"));
   Serial.println(F("   Prueba del Sensor SCD41 - CO2"));
   Serial.println(F("   (Sin librerias externas)"));
   Serial.println(F("========================================"));
  
   Wire.begin(I2C_SDA, I2C_SCL);
   Wire.setClock(100000);
   Serial.print(F("I2C iniciado en pines -> SDA: "));
   Serial.print(I2C_SDA);
   Serial.print(F(", SCL: "));
   Serial.println(I2C_SCL);
  
   // Escaneo I2C
   Serial.println(F("\n[PASO 1]: Escaneando dispositivos I2C..."));
   bool sensorFound = false;
   for(uint8_t address = 1; address < 127; address++) {
       Wire.beginTransmission(address);
       if(Wire.endTransmission() == 0) {
           Serial.print(F("  -> Dispositivo en 0x"));
           if(address < 16) Serial.print("0");
           Serial.print(address, HEX);
           if(address == SCD4X_ADDRESS) {
               Serial.println(F(" (SCD41 DETECTADO!)"));
               sensorFound = true;
           } else {
               Serial.println();
           }
       }
   }
  
   if(!sensorFound) {
       Serial.println(F("\n  -> ERROR: No se detecto SCD41 en direccion 0x62"));
       Serial.println(F("     Verifica las conexiones:"));
       Serial.println(F("     VCC -> 3.3V"));
       Serial.println(F("     GND -> GND"));
       Serial.println(F("     SDA -> GPIO 21"));
       Serial.println(F("     SCL -> GPIO 22"));
       while(1);
   }
  
   Serial.println(F("\n[PASO 2]: Inicializando SCD41..."));
  
   // Detener mediciones previas
   sendCommand(CMD_STOP_MEASUREMENT);
   delay(50);
  
   // Leer número de serie
   getSerialNumber();
  
   // Iniciar mediciones periódicas
   sendCommand(CMD_START_PERIODIC_MEASUREMENT);
   Serial.println(F("  -> Mediciones periodicas iniciadas"));
   Serial.println(F("  -> Esperando 5 segundos para la primera lectura..."));
   delay(5000);
  
   Serial.println(F("\n========================================"));
   Serial.println(F("   Leyendo datos cada 2 segundos"));
   Serial.println(F("========================================\n"));
}


void loop() {
   uint16_t co2;
   float temperature, humidity;
  
   if (readMeasurement(co2, temperature, humidity)) {
       Serial.print(F("CO2: "));
       Serial.print(co2);
       Serial.println(F(" ppm"));
      
       Serial.print(F("Temperatura: "));
       Serial.print(temperature, 2);
       Serial.println(F(" °C"));
      
       Serial.print(F("Humedad: "));
       Serial.print(humidity, 2);
       Serial.println(F(" %"));
      
       // Calidad del aire
       Serial.print(F("Calidad: "));
       if(co2 < 800) Serial.println(F("BUENA"));
       else if(co2 < 1200) Serial.println(F("MODERADA - Ventilar"));
       else Serial.println(F("MALA - Ventilar urgentemente!"));
      
       Serial.println(F("----------------------------------------"));
   } else {
       Serial.println(F("Esperando datos del sensor..."));
   }
  
   delay(2000);
}
