#include "BH1750.h"                    // Incluir la cabecera propia de la clase BH1750

BH1750::BH1750(uint8_t addr, TwoWire *wire) {   // Constructor: recibe dirección I2C y puntero al bus Wire
    _addr = addr;                     // Guardar la dirección I2C en el miembro privado _addr
    _wire = wire;                     // Guardar el puntero al bus I2C en el miembro privado _wire
}   // Fin del constructor

bool BH1750::begin() {                // Método público: inicializa el sensor BH1750
    _wire->beginTransmission(_addr);  // Iniciar transmisión I2C hacia la dirección _addr
    _wire->write(0x01);               // Enviar comando 0x01: Power on (encender el sensor)
    _wire->endTransmission();         // Finalizar transmisión
    delay(10);                        // Esperar 10 ms para que el sensor se estabilice
    
    _wire->beginTransmission(_addr);  // Iniciar nueva transmisión
    _wire->write(0x10);               // Enviar comando 0x10: Continuously H-resolution mode (medición continua, alta resolución)
    _wire->endTransmission();         // Finalizar transmisión
    delay(10);                        // Esperar 10 ms para que el sensor configure el modo
    
    return true;                      // Retornar true (inicialización exitosa)
}   // Fin de begin()

float BH1750::readLight() {           // Método público: realiza una lectura de luz
    _wire->requestFrom(_addr, (uint8_t)2);   // Solicitar 2 bytes del sensor (medición en alta resolución)
    if (_wire->available() < 2) return -1;  // Si no hay 2 bytes disponibles, retornar -1 (error)
    
    uint8_t high = _wire->read();     // Leer el byte alto (MSB)
    uint8_t low = _wire->read();      // Leer el byte bajo (LSB)
    uint16_t raw = (high << 8) | low; // Combinar bytes en un entero de 16 bits (valor crudo)
    return raw / 1.2f;                // Convertir a lux según la fórmula del datasheet (para modo H-resolution)
}   // Fin de readLight()