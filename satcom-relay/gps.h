#ifndef GPS_H
#define GPS_H

#include <Adafruit_GPS.h>
#include "wiring_private.h" // SERCOM pinPeripheral() function

//#define GPS_NMEA_MESSAGES false // show gps serial messages
//#define GPS_DEBUG false

#define GPS_EN_PIN 14 //A0

#define GPS_WAKEUP_INTERVAL (60*60*1000)
#define GPS_LOCK_TIMEOUT 60000
#define GPS_BOOT_TIMEOUT (60000*5)

enum GPSState{NOT_SET, STANDBY, WAKEUP};
const char * const gpsStateStrings[] = { "NOT_SET", "STANDBY", "WAKEUP" };

class GPS {

private:
    Adafruit_GPS adafruitGPS;
    float lastFixLatitude = 0;
    float lastFixLongitude = 0;
    char lastFixDate[32] = {0};
    enum GPSState gpsCommandedState = NOT_SET;
    void getGPSTime();

public:
    GPS();
    int initGPS();
    boolean readGPSSerial();
    boolean gpsHasFix();
    void gpsStandby();
    void gpsWakeup();
    void printAdafruitGPS();
    float getLastFixLatitude();
    float getLastFixLongitude();
    const char * getLastFixDate();
    float getSecondsSinceLastFix();
    GPSState getGPSCommandedState();
    const char * getGPSCommandedStateString();
};

#endif
