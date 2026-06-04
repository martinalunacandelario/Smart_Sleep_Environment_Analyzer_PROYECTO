#include "SCD41.h"                    // Incluir la cabecera propia (declaración de la clase SCD41)

// Constructor: guarda dirección y puntero al bus I2C
SCD41::SCD41(uint8_t addr, TwoWire *wire) {   // Constructor de la clase SCD41: recibe dirección I2C y puntero al bus Wire
    _addr = addr;                     // Guardar dirección I2C en el miembro privado _addr
    _wire = wire;                     // Guardar puntero al bus (por defecto Wire) en el miembro privado _wire
}   // Fin del constructor

// Inicialización del sensor: detiene mediciones previas y arranca periódicas
bool SCD41::begin() {                 // Método público begin(): inicializa el sensor
    sendCommand(0x3F86);              // Comando "stop periodic measurement" (detener mediciones periódicas previas)
    delay(50);                        // Esperar 50 ms para que el comando se procese
    sendCommand(0x21B1);              // Comando "start periodic measurement" (iniciar mediciones periódicas)
    delay(50);                        // Esperar 50 ms para que el comando se procese
    return true;                      // Siempre retorna true (si falla la comunicación se verá después en las lecturas)
}   // Fin de begin()

// Envía un comando de 16 bits sin argumentos
void SCD41::sendCommand(uint16_t cmd) {   // Método privado: envía un comando de 16 bits
    _wire->beginTransmission(_addr);  // Iniciar transmisión I2C hacia la dirección _addr
    _wire->write(cmd >> 8);           // Enviar el byte alto (8 bits superiores) del comando
    _wire->write(cmd & 0xFF);         // Enviar el byte bajo (8 bits inferiores) del comando
    _wire->endTransmission();         // Finalizar la transmisión I2C
}   // Fin de sendCommand()

// Envía un comando de 16 bits seguido de un argumento de 16 bits
void SCD41::sendCommandWithArg(uint16_t cmd, uint16_t arg) {   // Envía comando con argumento
    _wire->beginTransmission(_addr);  // Iniciar transmisión I2C
    _wire->write(cmd >> 8);           // Byte alto del comando
    _wire->write(cmd & 0xFF);         // Byte bajo del comando
    _wire->write(arg >> 8);           // Byte alto del argumento
    _wire->write(arg & 0xFF);         // Byte bajo del argumento
    _wire->endTransmission();         // Finalizar transmisión
}   // Fin de sendCommandWithArg()

// Cálculo CRC-8 (polinomio 0x31, valor inicial 0xFF)
uint8_t SCD41::calcCRC8(uint8_t data1, uint8_t data2) {   // Calcula CRC8 de dos bytes (polinomio 0x31)
    uint8_t crc = 0xFF;               // Inicializar CRC a 0xFF (valor inicial)
    crc ^= data1;                     // Hacer XOR con el primer byte
    for (uint8_t bit = 8; bit > 0; --bit) {   // Procesar 8 bits
        if (crc & 0x80) crc = (crc << 1) ^ 0x31;   // Si el bit más alto es 1, desplazar y aplicar polinomio
        else crc = (crc << 1);                    // Si no, solo desplazar
    }   // Fin del bucle para el primer byte
    crc ^= data2;                     // Hacer XOR con el segundo byte
    for (uint8_t bit = 8; bit > 0; --bit) {   // Procesar 8 bits de nuevo
        if (crc & 0x80) crc = (crc << 1) ^ 0x31;   // Misma operación
        else crc = (crc << 1);
    }   // Fin del segundo bucle
    return crc;                       // Devolver CRC calculado
}   // Fin de calcCRC8()

// Comprueba si el sensor tiene datos nuevos disponibles
bool SCD41::isDataReady() {           // Método privado: verifica si hay datos listos para leer
    sendCommand(0xE4B8);              // Comando "get data ready" (solicitar estado)
    delay(2);                         // Esperar 2 ms a que el sensor responda
    _wire->requestFrom(_addr, (uint8_t)3);   // Solicitar 3 bytes (2 de estado + CRC)
    if (_wire->available() < 3) return false;   // Si no hay 3 bytes disponibles, error
    uint8_t msb = _wire->read();      // Leer byte alto del estado
    uint8_t lsb = _wire->read();      // Leer byte bajo del estado
    uint8_t crc = _wire->read();      // Leer el CRC de esos dos bytes
    if (calcCRC8(msb, lsb) != crc) return false;   // Validar CRC; si no coincide, error
    uint16_t ready = (msb << 8) | lsb; // Combinar los dos bytes en un entero de 16 bits
    return (ready & 0x07FF) != 0;     // El bit 12 indica datos listos; máscara 0x07FF ignora bits superiores
}   // Fin de isDataReady()

// Lee la medición completa: CO2, temperatura y humedad
bool SCD41::readMeasurement(uint16_t &co2, float &temperature, float &humidity) {   // Método público: realiza una lectura completa
    if (!isDataReady()) return false; // Si no hay datos listos, salir con error
    
    sendCommand(0xEC05);              // Comando "read measurement" (solicitar medición)
    delay(2);                         // Esperar 2 ms a que el sensor prepare los datos
    _wire->requestFrom(_addr, (uint8_t)9);   // Solicitar 9 bytes (3 campos de 2 bytes + CRC cada uno)
    if (_wire->available() < 9) return false;   // Si no hay 9 bytes disponibles, error
    
    // --- CO2 ---
    uint8_t co2_msb = _wire->read();  // Leer byte alto del CO2
    uint8_t co2_lsb = _wire->read();  // Leer byte bajo del CO2
    uint8_t co2_crc = _wire->read();  // Leer CRC del CO2
    if (calcCRC8(co2_msb, co2_lsb) != co2_crc) return false;   // Validar CRC del CO2
    co2 = (co2_msb << 8) | co2_lsb;   // Combinar bytes para obtener valor de CO2 en ppm
    
    // --- Temperatura ---
    uint8_t temp_msb = _wire->read(); // Leer byte alto de temperatura
    uint8_t temp_lsb = _wire->read(); // Leer byte bajo de temperatura
    uint8_t temp_crc = _wire->read(); // Leer CRC de temperatura
    if (calcCRC8(temp_msb, temp_lsb) != temp_crc) return false;   // Validar CRC
    uint16_t temp_raw = (temp_msb << 8) | temp_lsb;   // Valor crudo (16 bits)
    temperature = -45.0f + 175.0f * temp_raw / 65535.0f;   // Convertir a grados Celsius según datasheet
    
    // --- Humedad ---
    uint8_t hum_msb = _wire->read();  // Leer byte alto de humedad
    uint8_t hum_lsb = _wire->read();  // Leer byte bajo de humedad
    uint8_t hum_crc = _wire->read();  // Leer CRC de humedad
    if (calcCRC8(hum_msb, hum_lsb) != hum_crc) return false;   // Validar CRC
    uint16_t hum_raw = (hum_msb << 8) | hum_lsb;   // Valor crudo (16 bits)
    humidity = 100.0f * hum_raw / 65535.0f;        // Convertir a porcentaje según datasheet
    
    return true;                      // Lectura exitosa
}   // Fin de readMeasurement()