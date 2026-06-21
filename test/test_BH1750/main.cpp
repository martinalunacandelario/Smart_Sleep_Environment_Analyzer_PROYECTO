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

    for(address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.print(F("  -> Dispositivo I2C encontrado en direccion 0x"));
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);

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
    // ✅ CORREGIDO: readLight() en lugar de readLightLevel()
    float lux = lightMeter.readLight();

    Serial.print(F("Luz medida: "));
    Serial.print(lux);
    Serial.println(F(" lx"));

    if (lux < 10) {
        Serial.println(F("  -> Muy oscuro. Tapa el sensor con la mano."));
    } else if (lux > 400) {
        Serial.println(F("  -> Muy brillante. Apunta una linterna o lleva el sensor cerca de la luz."));
    }

    Serial.println(F("-----------------------------------"));
    delay(1000);
}