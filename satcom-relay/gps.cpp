#include "gps.h"

Uart GPSSerial(&sercom2, GPS_RX_PIN, GPS_TX_PIN, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM2_Handler()
{
  GPSSerial.IrqHandler();
}

int GPS::initGPS() {
  Adafruit_GPS temp_adafruitGPS(&GPSSerial);
  adafruitGPS = temp_adafruitGPS;

  adafruitGPS.begin(9600);
  // Assign pins 10 & 11 SERCOM functionality
  pinPeripheral(GPS_RX_PIN, PIO_SERCOM);
  pinPeripheral(GPS_TX_PIN, PIO_SERCOM);
  
  // Use init parameters from GPS library example
  adafruitGPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  adafruitGPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  adafruitGPS.sendCommand(PGCMD_ANTENNA);
  delay(1000);
  return 1;
}

void GPS::gpsStandby() {
  if (this->gpsCommandedState != STANDBY) {
    digitalWrite(GPS_EN_PIN, HIGH);  
    this->gpsCommandedState = STANDBY;

    #if GPS_DEBUG
    Serial.println("DEBUG: gpsStandby()");
    #endif
  }
}

void GPS::gpsWakeup() {
  if (this->gpsCommandedState != WAKEUP) {
    digitalWrite(GPS_EN_PIN, LOW);
    this->gpsCommandedState = WAKEUP;

    #if GPS_DEBUG
    Serial.println("DEBUG: gpsWakeup()");
    #endif
  }
}

void GPS::printAdafruitGPS() {
  Serial.print("\nTime: ");
    if (adafruitGPS.hour < 10) { Serial.print('0'); }
    Serial.print(adafruitGPS.hour, DEC); Serial.print(':');
    if (adafruitGPS.minute < 10) { Serial.print('0'); }
    Serial.print(adafruitGPS.minute, DEC); Serial.print(':');
    if (adafruitGPS.seconds < 10) { Serial.print('0'); }
    Serial.print(adafruitGPS.seconds, DEC); Serial.print('.');
    if (adafruitGPS.milliseconds < 10) {
      Serial.print("00");
    } else if (adafruitGPS.milliseconds > 9 && adafruitGPS.milliseconds < 100) {
      Serial.print("0");
    }
    Serial.println(adafruitGPS.milliseconds);
    Serial.print("Date: ");
    Serial.print(adafruitGPS.day, DEC); Serial.print('/');
    Serial.print(adafruitGPS.month, DEC); Serial.print("/20");
    Serial.println(adafruitGPS.year, DEC);
    Serial.print("Fix: "); Serial.print((int)adafruitGPS.fix);
    Serial.print(" quality: "); Serial.println((int)adafruitGPS.fixquality);
    if (adafruitGPS.fix) {
      Serial.print("Location: ");
      Serial.print(adafruitGPS.latitude, 4); Serial.print(adafruitGPS.lat);
      Serial.print(", ");
      Serial.print(adafruitGPS.longitude, 4); Serial.println(adafruitGPS.lon);
      Serial.print("Speed (knots): "); Serial.println(adafruitGPS.speed);
      Serial.print("Angle: "); Serial.println(adafruitGPS.angle);
      Serial.print("Altitude: "); Serial.println(adafruitGPS.altitude);
      Serial.print("Satellites: "); Serial.println((int)adafruitGPS.satellites);
    }
}

float GPS::getLastFixLatitude() {
  return this->lastFixLatitude;
}

float GPS::getLastFixLongitude() {
  return this->lastFixLongitude;
}

void GPS::getGPSTime() {
  sprintf(this->lastFixDate, "%d-%02d-%02d %02d:%02d:%02d",
          adafruitGPS.year, adafruitGPS.month, adafruitGPS.day, adafruitGPS.hour, adafruitGPS.minute, adafruitGPS.seconds);
  #if GPS_DEBUG
  Serial.print("GPS time and date ");
  Serial.println(buf);
  #endif
}

boolean GPS::readGPSSerial() {
  #ifdef GPS_NMEA_MESSAGES
  char c = this->adafruitGPS.read();
  if (c) Serial.print(c); //print this to see NMEA
  #else
  this->adafruitGPS.read();
  #endif

  if (adafruitGPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trying to print out data
    #if GPS_NMEA_MESSAGES
    Serial.print("GPS_NMEA_MESSAGES: ");
    Serial.print(adafruitGPS.lastNMEA()); // this also sets the newNMEAreceived() flag to false
    #endif
    if (adafruitGPS.parse(adafruitGPS.lastNMEA())) { // this also sets the newNMEAreceived() flag to false
      this->lastFixLatitude = adafruitGPS.latitudeDegrees;
      this->lastFixLongitude = adafruitGPS.longitudeDegrees;
      getGPSTime();
      return true; 
    }
  }
  return false; // we can fail to parse a sentence in which case we should just wait for another
}

boolean GPS::gpsHasFix() {
    // adafruitGPS.fix seems to be unreliable
    // use Robs hack
    // consider anything < 5sec to mean GPS has a current Fix
    if (this->adafruitGPS.secondsSinceFix() < 5) {
        return true;
    }
    return false;
}

const char * GPS::getLastFixDate() {
    return this->lastFixDate;
}

float GPS::getSecondsSinceLastFix() {
    return this->adafruitGPS.secondsSinceFix();
}

GPSState GPS::getGPSCommandedState() {
    return this->gpsCommandedState;
}

const char * GPS::getGPSCommandedStateString() {
    return gpsStateStrings[gpsCommandedState];
}
