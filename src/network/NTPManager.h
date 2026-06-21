#ifndef NTP_MANAGER_H
#define NTP_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

// ============================================================================
// NTPManager - Gestión de WiFi y NTP (Hora real desde Internet)
// ============================================================================
class NTPManager {
public:
    static bool begin();
    static bool syncNTP();
    
    static String getCurrentDateTime();
    static String getCurrentDate();
    static String getCurrentTime();
    static String getCurrentTimeHM();
    static unsigned long getCurrentEpoch();
    static bool isTimeSynced();
    static String getLocalIP();
    static String getAPIP();

private:
    static bool _timeSynced;
    static unsigned long _lastSync;
    static unsigned long _epoch;
};

#endif