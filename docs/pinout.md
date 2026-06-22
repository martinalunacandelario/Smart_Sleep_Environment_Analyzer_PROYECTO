## Esquema de conexiones (pinout)

La siguiente tabla muestra la asignación de pines GPIO del ESP32 a cada componente del sistema.

| Componente | Pines | Bus | Observaciones |
|------------|-------|-----|---------------|
| **SCD41** (CO₂, Temp, Hum) | VCC → 3.3V / GND → GND / SDA → GPIO21 / SCL → GPIO22 | I²C | Comparte bus con BH1750 y OLED |
| **BH1750** (Luz) | VCC → 3.3V / GND → GND / SDA → GPIO21 / SCL → GPIO22 | I²C | Comparte bus con SCD41 y OLED |
| **OLED SH1106** | VCC → 3.3V / GND → GND / SDA → GPIO21 / SCL → GPIO22 | I²C | Dirección 0x3C |
| **MicroSD** | VCC → 5V / GND → GND / CS → GPIO5 / SCK → GPIO18 / MOSI → GPIO23 / MISO → GPIO19 | SPI | Velocidad reducida a 4 MHz |
| **LED Rojo** | Ánodo → GPIO13 / Cátodo → GND (con resistencia 220Ω) | GPIO | Alerta crítica |
| **LED Amarillo** | Ánodo → GPIO14 / Cátodo → GND (con resistencia 220Ω) | GPIO | Alerta regular |
| **LED Verde** | Ánodo → GPIO27 / Cátodo → GND (con resistencia 220Ω) | GPIO | Estado óptimo |
| **Buzzer** | (+) → GPIO26 / (-) → GND | GPIO | Buzzer pasivo (PWM) |
| **Botón** | PIN1 → GPIO25 / PIN2 → GND | GPIO | Pull-up interna activa a LOW |