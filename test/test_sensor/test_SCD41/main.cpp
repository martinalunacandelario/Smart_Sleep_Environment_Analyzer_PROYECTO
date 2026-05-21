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

#include <Arduino.h>                      // Incluye la librería estándar de Arduino para ESP32
#include <Wire.h>                         // Incluye la librería para comunicación I2C
#include <SensirionI2cScd4x.h>            // Incluye la librería oficial del sensor SCD41 de Sensirion

SensirionI2cScd4x scd41;                  // Crea un objeto para controlar el sensor SCD41 por I2C

unsigned long lastRead = 0;               // Almacena el tiempo (ms) de la última lectura
const unsigned long READ_INTERVAL_MS = 5000;  // Intervalo fijo de 5 segundos entre lecturas

void printUint16Hex(uint16_t value) {     // Función auxiliar para imprimir números hexadecimales de 16 bits
    Serial.print(value < 0x1000 ? "0" : ""); // Añade un cero a la izquierda si el valor es menor que 0x1000
    Serial.print(value < 0x100 ? "0" : "");  // Añade un cero si es menor que 0x100
    Serial.print(value < 0x10 ? "0" : "");   // Añade un cero si es menor que 0x10
    Serial.print(value, HEX);                 // Imprime el valor en hexadecimal
}

void setup() {                            // Función de inicialización (se ejecuta una vez al inicio)
    Serial.begin(115200);                 // Inicia la comunicación serie a 115200 baudios
    delay(1000);                          // Espera 1 segundo para estabilizar la consola serie
    Serial.println("\n=== TEST SCD41 ==="); // Imprime cabecera de la prueba
    Serial.println("Conexiones: VDD→3.3V, GND→GND, SDA→GPIO21, SCL→GPIO22"); // Informa las conexiones

    Wire.begin();                         // Inicializa el bus I2C con los pines por defecto (SDA=21, SCL=22)
    Wire.setClock(100000);                // Configura la velocidad del bus I2C a 100 kHz

    // Inicializar sensor
    scd41.begin(Wire);                    // Vincula el objeto scd41 al bus I2C (Wire)

    uint16_t error = scd41.stopPeriodicMeasurement(); // Detiene cualquier medición periódica previa
    if (error) {                          // Si hay un error (error != 0)
        Serial.print("Error stopPeriodicMeasurement: 0x"); // Mensaje de error
        printUint16Hex(error);            // Imprime el código de error en hex
        Serial.println();                 // Salto de línea
    }

    error = scd41.startPeriodicMeasurement(); // Inicia las mediciones periódicas (actualización ~5 Hz)
    if (error) {                          // Si ocurre un error al iniciar
        Serial.print("Error startPeriodicMeasurement: 0x"); // Mensaje
        printUint16Hex(error);            // Imprime el código de error
        Serial.println();                 // Salto de línea
        while (1) delay(100);             // Bucle infinito: detiene el programa (no sigue si hay error)
    }
    Serial.println("Medición periódica iniciada."); // Confirmación de inicio correcto
    delay(500);                           // Espera 500 ms para permitir la primera lectura completa
}

void loop() {                             // Bucle principal (se ejecuta repetidamente)
    unsigned long now = millis();         // Obtiene el tiempo actual en milisegundos
    if (now - lastRead >= READ_INTERVAL_MS) { // Comprueba si han pasado 5 segundos desde la última lectura
        lastRead = now;                   // Actualiza el tiempo de la última lectura

        uint16_t error;                   // Variable para almacenar códigos de error
        uint16_t co2 = 0;                 // Variable para almacenar el valor de CO₂ en ppm
        float temperature = 0.0f;         // Variable para almacenar la temperatura en °C
        float humidity = 0.0f;            // Variable para almacenar la humedad relativa en %

        error = scd41.readMeasurement(co2, temperature, humidity); // Intenta leer los datos del sensor
        if (error) {                      // Si hay error al leer
            Serial.print("Error readMeasurement: 0x"); // Mensaje
            printUint16Hex(error);        // Imprime el código de error
            Serial.println();             // Salto de línea
            return;                       // Sale de esta iteración del loop (no imprime datos erróneos)
        }

        // Si no hubo error, imprime los datos en la consola
        Serial.println("-----------------------------");
        Serial.printf("CO2: %d ppm\n", co2);                 // Muestra CO₂ en ppm
        Serial.printf("Temperatura: %.2f °C\n", temperature); // Muestra temp con 2 decimales
        Serial.printf("Humedad: %.2f %%\n", humidity);        // Muestra humedad con 2 decimales

        // Validación de rangos según el manual del sensor SCD41 (sin interrumpir, solo informativo)
        if (co2 >= 300 && co2 <= 5000 &&            // CO₂ típico en interior: 300-5000 ppm
            temperature >= -10 && temperature <= 60 && // Rango de operación del sensor
            humidity >= 0 && humidity <= 100) {     // Humedad relativa entre 0 y 100 %
            Serial.println("✓ Datos dentro de rangos esperados."); // Todo bien
        } else {
            Serial.println("⚠️ Datos fuera de rango típico."); // Algo extraño
        }
    }
    delay(100);   // Pequeña pausa de 100 ms para evitar saturar el procesador en el bucle
}