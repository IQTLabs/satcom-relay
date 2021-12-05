#ifndef SATCOMRelay_H
#define SATCOMRelay_H

#include <Arduino.h>
#include <IridiumSBD.h>
#include <Adafruit_GPS.h>

#define TEST_MODE true
#define TEST_MODE_PRINT_INTERVAL 2000 // use for testing. prints relay status to Serial

#define DEBUG_MODE true // print lots of debugging messages

#define GPS_NMEA_MESSAGES true // show gps serial messages

#define GPS_EN_PIN 14 //A0
#define GPS_RX_PIN 5
#define GPS_TX_PIN 22
#define IRIDIUM_RX_PIN 11
#define IRIDIUM_TX_PIN 10

#define GPS_WAKEUP_INTERVAL 10000
#define GPS_LOCK_TIMEOUT 60000

enum gpsState{NOT_SET, STANDBY, WAKEUP};

class SATCOMRelay {
  
  private:
    Adafruit_GPS GPS;
    float battery = -1;
    enum gpsState gpsCommandedState = NOT_SET;

  public:
    SATCOMRelay();
    int initGPS();
    boolean readGPSSerial();
    boolean gpsFix();
    void gpsStandby();
    void gpsWakeup();
    void printGPS();
    float getLat();
    float getLon();
    void print();
};
#endif
