#ifndef GPS_H
#define GPS_H

#include <Adafruit_GPS.h>
#include "wiring_private.h" // SERCOM pinPeripheral() function

#define GPS_NMEA_MESSAGES false // show gps serial messages
#define GPS_DEBUG false

#define GPS_EN_PIN 14 //A0
#define GPS_RX_PIN 5
#define GPS_TX_PIN 22

#define GPS_WAKEUP_INTERVAL 10000
#define GPS_LOCK_TIMEOUT 60000

enum GPSState{NOT_SET, STANDBY, WAKEUP};
static char gpsStateStrings[3][10] = { "NOT_SET", "STANDBY", "WAKEUP" };

class GPS {

private:
    Adafruit_GPS adafruitGPS;
    float lastFixLatitude;
    float lastFixLongitude;
    char lastFixDate[32];
    enum GPSState gpsCommandedState = NOT_SET;
    void getGPSTime(char * buf);

public:

    int initGPS();
    boolean readGPSSerial();
    boolean gpsHasFix();
    void gpsStandby();
    void gpsWakeup();
    void printAdafruitGPS();
    float getLastFixLatitude();
    float getLastFixLongitude();
    char * getLastFixDate();
    float getSecondsSinceLastFix();
    GPSState getGPSCommandedState();
    char * getGPSCommandedStateString();
};

#endif