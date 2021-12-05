#include "SATCOMRelay.h"
#include "wiring_private.h" // pinPeripheral() function

Uart GPSSerial(&sercom2, GPS_RX_PIN, GPS_TX_PIN, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM2_Handler()
{
  GPSSerial.IrqHandler();
}

Uart IridiumSerial (&sercom1, IRIDIUM_RX_PIN, IRIDIUM_TX_PIN, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler()
{
  IridiumSerial.IrqHandler();
}

SATCOMRelay::SATCOMRelay() {
  
}

int SATCOMRelay::initGPS() {
  Adafruit_GPS gps(&GPSSerial);
  GPS = gps;

  GPS.begin(9600);
  // Assign pins 10 & 11 SERCOM functionality
  pinPeripheral(GPS_RX_PIN, PIO_SERCOM);
  pinPeripheral(GPS_TX_PIN, PIO_SERCOM);
  
  // Use init parameters from GPS library example
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  GPS.sendCommand(PGCMD_ANTENNA);
  delay(1000);
  return 1;
}

boolean SATCOMRelay::readGPSSerial() {
  char c = this->GPS.read();
  //if (c) Serial.print(c); //print this to see NMEA

  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trying to print out data
    #if DEBUG_MODE
    Serial.print(GPS.lastNMEA()); // this also sets the newNMEAreceived() flag to false
    #endif
    if (GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return true; // we can fail to parse a sentence in which case we should just wait for another
  }
  return false;
}

boolean SATCOMRelay::gpsFix() {
  return GPS.fix;
}

void SATCOMRelay::gpsStandby() {
  boolean result = GPS.standby();
  this->gpsCommandedState = STANDBY;
  #if DEBUG_MODE
  if (result) { //False if already in standby, true if it entered standby
    Serial.println("GPS.standby() returned true");
  } else {
    Serial.println("GPS.standby() returned false");
  }
  #endif
}

void SATCOMRelay::gpsWakeup() {
  boolean result = GPS.wakeup();
  this->gpsCommandedState = WAKEUP;
  #if DEBUG_MODE
  if (result) { //True if woken up, false if not in standby or failed to wake
    Serial.println("GPS.wakeup() returned true");
  } else {
    Serial.println("GPS.wakeup() returned false");
  }
  #endif
}

void SATCOMRelay::printGPS() {
  Serial.print("\nTime: ");
    if (GPS.hour < 10) { Serial.print('0'); }
    Serial.print(GPS.hour, DEC); Serial.print(':');
    if (GPS.minute < 10) { Serial.print('0'); }
    Serial.print(GPS.minute, DEC); Serial.print(':');
    if (GPS.seconds < 10) { Serial.print('0'); }
    Serial.print(GPS.seconds, DEC); Serial.print('.');
    if (GPS.milliseconds < 10) {
      Serial.print("00");
    } else if (GPS.milliseconds > 9 && GPS.milliseconds < 100) {
      Serial.print("0");
    }
    Serial.println(GPS.milliseconds);
    Serial.print("Date: ");
    Serial.print(GPS.day, DEC); Serial.print('/');
    Serial.print(GPS.month, DEC); Serial.print("/20");
    Serial.println(GPS.year, DEC);
    Serial.print("Fix: "); Serial.print((int)GPS.fix);
    Serial.print(" quality: "); Serial.println((int)GPS.fixquality);
    if (GPS.fix) {
      Serial.print("Location: ");
      Serial.print(GPS.latitude, 4); Serial.print(GPS.lat);
      Serial.print(", ");
      Serial.print(GPS.longitude, 4); Serial.println(GPS.lon);
      Serial.print("Speed (knots): "); Serial.println(GPS.speed);
      Serial.print("Angle: "); Serial.println(GPS.angle);
      Serial.print("Altitude: "); Serial.println(GPS.altitude);
      Serial.print("Satellites: "); Serial.println((int)GPS.satellites);
    }
}

float SATCOMRelay::getLat() {
  return GPS.longitude;
}

float SATCOMRelay::getLon() {
  return GPS.longitude;
}

void SATCOMRelay::print() {
  Serial.println("RELAY STATE:");
  Serial.print("\tLatitude: ");
  Serial.println(GPS.latitude_fixed);
  Serial.print("\tLongitude: ");
  Serial.println(GPS.longitude_fixed);
  Serial.print("\tGPS Time: ");
  if (GPS.hour < 10)
  {
    Serial.print('0');
  }
  Serial.print(GPS.hour, DEC);
  Serial.print(':');
  if (GPS.minute < 10)
  {
    Serial.print('0');
  }
  Serial.print(GPS.minute, DEC);
  Serial.print(':');
  if (GPS.seconds < 10)
  {
    Serial.print('0');
  }
  Serial.print(GPS.seconds, DEC);
  Serial.print('.');
  if (GPS.milliseconds < 10)
  {
    Serial.print("00");
  }
  else if (GPS.milliseconds > 9 && GPS.milliseconds < 100)
  {
    Serial.print("0");
  }
  Serial.println(GPS.milliseconds);
  Serial.print("\tGPS State: ");
  if (this->gpsCommandedState == 1) {
    Serial.println("STANDBY");
  } else if (this->gpsCommandedState == 2) {
    Serial.println("WAKEUP");
  } else {
    Serial.println("NOT_SET");      
  }
  
}
