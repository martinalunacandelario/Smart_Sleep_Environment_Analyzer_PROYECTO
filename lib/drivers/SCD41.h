#ifndef SCD41_H                       // Si no está definido SCD41_H
#define SCD41_H                       // Definirlo para evitar inclusiones múltiples

#include <Arduino.h>                  // Librería base (define tipos básicos, Serial, etc.)
#include <Wire.h>                     // Librería para comunicación I2C

class SCD41 {                         // Declaración de la clase SCD41
public:
    // Constructor: recibe dirección I2C (por defecto 0x62) y puntero al objeto Wire
    SCD41(uint8_t addr = 0x62, TwoWire *wire = &Wire);
    
    // Inicializa el sensor: inicia mediciones periódicas
    bool begin();
    
    // Lee una medición completa (CO2, temperatura, humedad)
    bool readMeasurement(uint16_t &co2, float &temperature, float &humidity);
    
private:
    TwoWire *_wire;                   // Puntero al objeto Wire (I2C)
    uint8_t _addr;                    // Dirección I2C del sensor
    
    void sendCommand(uint16_t cmd);   // Envía un comando de 16 bits
    void sendCommandWithArg(uint16_t cmd, uint16_t arg); // Envía comando+argumento
    uint8_t calcCRC8(uint8_t data1, uint8_t data2); // Calcula CRC8
    bool isDataReady();               // Verifica si hay datos listos
};

#endif // SCD41_H