#ifndef BH1750_H                       // Si no está definido BH1750_H
#define BH1750_H                       // Definir BH1750_H para evitar inclusiones múltiples

#include <Arduino.h>                   // Incluye la librería base de Arduino (para Serial, tipos, etc.)
#include <Wire.h>                      // Incluye la librería Wire para comunicación I2C

class BH1750 {                         // Declaración de la clase BH1750 (sensor de luz)
public:                                // Sección pública: accesible desde fuera de la clase
    BH1750(uint8_t addr = 0x23, TwoWire *wire = &Wire);   // Constructor: dirección I2C (por defecto 0x23) y puntero al bus (por defecto Wire)
    bool begin();                      // Método público: inicializa el sensor (configura modo de medición)
    float readLight();                 // Método público: realiza una lectura y devuelve el valor en lux
private:                               // Sección privada: solo accesible desde dentro de la clase
    TwoWire *_wire;                    // Puntero al objeto Wire (bus I2C)
    uint8_t _addr;                     // Dirección I2C del sensor (por defecto 0x23)
};                                     // Fin de la clase BH1750

#endif // BH1750_H                      // Fin de la protección de inclusión