#include <Arduino.h>   // Incluye la librería estándar de Arduino (para Serial, pinMode, etc.)
#include <SPI.h>       // Incluye la librería SPI para comunicación con la tarjeta SD
#include <SD.h>        // Incluye la librería SD para manejar la tarjeta de memoria

/*
 * TEST lector MicroSD para ESP32
 * 
 * Conexiones:
 * CS   -> GPIO5
 * SCK  -> GPIO18
 * MOSI -> GPIO23
 * MISO -> GPIO19
 * VCC  -> 5V
 * GND  -> GND
 */

#define SD_CS    5     // Define el pin CS (Chip Select) como GPIO5
#define SD_SCK   18    // Define el pin SCK (Clock) como GPIO18
#define SD_MOSI  23    // Define el pin MOSI (Master Out Slave In) como GPIO23
#define SD_MISO  19    // Define el pin MISO (Master In Slave Out) como GPIO19

const char* testFileName = "/test.txt";   // Nombre del archivo de prueba (en la raíz)

// Prototipo
void printDirectory(File dir, int numTabs);   // Declaración anticipada de la función recursiva para listar archivos

void setup() {    // Función de configuración (se ejecuta una sola vez al inicio)

    Serial.begin(115200);    // Inicia la comunicación serie a 115200 baudios
    delay(2000);             // Espera 2 segundos para estabilizar el monitor serie

    Serial.println();        // Imprime una línea en blanco
    Serial.println("=================================");   // Imprime una línea decorativa
    Serial.println("      TEST MICROSD ESP32");             // Imprime título del test
    Serial.println("=================================");   // Imprime línea decorativa

    // Inicializar SPI
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI);   // Inicia el bus SPI con los pines SCK, MISO y MOSI definidos

    // Inicializar SD
    Serial.print("Inicializando tarjeta SD... ");   // Mensaje informativo

    if (!SD.begin(SD_CS)) {        // Intenta montar la tarjeta SD usando el pin CS. Si falla...
        Serial.println("ERROR");   // Imprime ERROR
        Serial.println("No se pudo inicializar la tarjeta.");   // Mensaje de error
        Serial.println("Revisa:");                               // Sugerencia
        Serial.println("- Cableado");                            // Posible causa
        Serial.println("- Alimentacion 5V");                     // Posible causa
        Serial.println("- Tarjeta formateada FAT32");            // Posible causa
        return;                    // Sale de setup() (no continúa)
    }

    Serial.println("OK");          // Si inicia correctamente, imprime OK

    // Tipo de tarjeta
    uint8_t cardType = SD.cardType();   // Obtiene el tipo de tarjeta (SDSC, SDHC, MMC, etc.)

    Serial.print("Tipo de tarjeta: ");   // Imprime etiqueta

    switch (cardType) {    // Según el tipo obtenido...

        case CARD_NONE:    // Si es NINGUNA
            Serial.println("NINGUNA");   // Imprime NINGUNA
            return;                      // Sale (tarjeta no detectada)

        case CARD_MMC:     // Si es MMC
            Serial.println("MMC");       // Imprime MMC
            break;                       // Sale del switch

        case CARD_SD:      // Si es SDSC (estándar)
            Serial.println("SDSC");      // Imprime SDSC
            break;                       // Sale del switch

        case CARD_SDHC:    // Si es SDHC (alta capacidad)
            Serial.println("SDHC");      // Imprime SDHC
            break;                       // Sale del switch

        default:           // Cualquier otro valor
            Serial.println("DESCONOCIDO");   // Imprime DESCONOCIDO
            break;                       // Sale del switch
    }

    // Tamaño
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);   // Calcula el tamaño total en MB (divide bytes entre 1024*1024)

    Serial.print("Tamano total: ");   // Imprime etiqueta
    Serial.print(cardSize);           // Imprime el valor numérico
    Serial.println(" MB");            // Imprime la unidad (MB)

    // Escritura
    Serial.println();                       // Línea en blanco
    Serial.println("Creando archivo test.txt ...");   // Mensaje informativo

    File file = SD.open(testFileName, FILE_WRITE);   // Abre (o crea) el archivo en modo escritura

    if (!file) {                         // Si no se pudo abrir (file es falso)
        Serial.println("ERROR al crear archivo.");   // Mensaje de error
        return;                          // Sale de setup()
    }

    String contenido = "";               // Crea una variable String vacía
    contenido += "=== TEST MICROSD ESP32 ===\n";       // Añade primera línea con salto de línea
    contenido += "Prueba completada correctamente.\n"; // Añade segunda línea
    contenido += "Conexion SPI OK.\n";                 // Añade tercera línea
    contenido += "Fecha compilacion: ";                // Añade etiqueta de fecha
    contenido += __DATE__;               // Añade la fecha de compilación (macro del compilador)
    contenido += " ";                    // Añade un espacio
    contenido += __TIME__;               // Añade la hora de compilación (macro)
    contenido += "\n";                   // Añade salto de línea final

    Serial.print("Escribiendo datos... ");   // Mensaje informativo

    if (file.print(contenido)) {         // Intenta escribir el contenido en el archivo
        Serial.println("OK");            // Si éxito, imprime OK
    } else {                             // Si falla
        Serial.println("ERROR");         // Imprime ERROR
    }

    file.close();        // Cierra el archivo (importante para guardar los datos)

    // Lectura
    Serial.println();                       // Línea en blanco
    Serial.println("Leyendo archivo...");   // Mensaje informativo

    file = SD.open(testFileName);        // Abre el archivo en modo lectura (por defecto)

    if (!file) {                         // Si no se pudo abrir
        Serial.println("ERROR al abrir archivo.");   // Mensaje de error
        return;                          // Sale de setup()
    }

    Serial.println("----- CONTENIDO -----");   // Encabezado

    while (file.available()) {           // Mientras haya bytes por leer en el archivo
        Serial.write(file.read());       // Lee un byte y lo envía por el puerto serie (carácter a carácter)
    }

    Serial.println("---------------------");   // Pie del contenido

    file.close();        // Cierra el archivo

    // Listado de archivos
    Serial.println();                        // Línea en blanco
    Serial.println("Archivos en la raiz:");  // Mensaje informativo

    File root = SD.open("/");        // Abre el directorio raíz ("/")

    printDirectory(root, 0);         // Llama a la función recursiva para mostrar todos los archivos y carpetas

    root.close();                    // Cierra el directorio raíz

    Serial.println();                // Línea en blanco
    Serial.println("=== PRUEBA COMPLETADA ===");   // Mensaje final
}

void loop() {    // Función principal (se ejecuta repetidamente)

    delay(10000);   // Espera 10 segundos entre iteraciones (no hay nada más que hacer, solo no terminar)
}

// Mostrar directorios
void printDirectory(File dir, int numTabs) {   // Función recursiva para listar contenido

    while (true) {    // Bucle infinito controlado internamente

        File entry = dir.openNextFile();   // Abre el siguiente archivo/carpeta dentro del directorio

        if (!entry) {      // Si no hay más entradas (entry es nulo)
            break;         // Sal del bucle
        }

        for (int i = 0; i < numTabs; i++) {   // Bucle para imprimir espacios de indentación
            Serial.print("  ");               // Imprime dos espacios por cada nivel de profundidad
        }

        Serial.print(entry.name());   // Imprime el nombre del archivo o carpeta

        if (entry.isDirectory()) {    // Si es una carpeta...

            Serial.println("/");      // Imprime una barra (indicador de directorio) y salta de línea

            printDirectory(entry, numTabs + 1);   // Llama recursivamente para mostrar el contenido de esa subcarpeta (aumenta indentación)

        } else {                      // Si es un archivo normal...

            Serial.print("  (");      // Imprime espacio y paréntesis abierto
            Serial.print(entry.size());   // Imprime el tamaño del archivo en bytes
            Serial.println(" bytes)");    // Imprime " bytes)" y salta de línea
        }

        entry.close();   // Cierra la entrada actual (libera recursos)
    }
}