#include "SATCOMRelay.h"
#include "wiring_private.h" // pinPeripheral() function

Uart GPSSerial(&sercom2, GPS_RX_PIN, GPS_TX_PIN, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM2_Handler()
{
  GPSSerial.IrqHandler();
}

SATCOMRelay::SATCOMRelay() {
  pinMode(GPS_EN_PIN, OUTPUT);
  pinMode(VBATPIN, INPUT);
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
    #if GPS_NMEA_MESSAGES
    Serial.print("GPS_NMEA_MESSAGES: ");
    Serial.print(GPS.lastNMEA()); // this also sets the newNMEAreceived() flag to false
    #endif
    if (GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return true; // we can fail to parse a sentence in which case we should just wait for another
  }
  return false;
}

boolean SATCOMRelay::gpsFix() {
  // return GPS.fix; this variable seems to be unreliable
  
  if (this->GPS.secondsSinceFix()<5) { // consider anything < 5sec a Fix
    return true;
  }
  return false;

  
}

void SATCOMRelay::gpsStandby() {
  if (this->gpsCommandedState != STANDBY) {
    digitalWrite(GPS_EN_PIN, HIGH);  
    this->gpsCommandedState = STANDBY;

    #if DEBUG_MODE
    Serial.println("DEBUG: gpsStandby()");
    #endif
  }
}

void SATCOMRelay::gpsWakeup() {
  if (this->gpsCommandedState != WAKEUP) {
    digitalWrite(GPS_EN_PIN, LOW);
    this->gpsCommandedState = WAKEUP;

    #if DEBUG_MODE
    Serial.println("DEBUG: gpsWakeup()");
    #endif
  }
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
  return GPS.latitudeDegrees;
}

float SATCOMRelay::getLon() {
  return GPS.longitudeDegrees;
}

void SATCOMRelay::print() {
  Serial.println("SATCOM RELAY STATUS:");
  Serial.print("GPS Latitude: ");
  Serial.println(GPS.latitude_fixed);
  Serial.print("GPS Longitude: ");
  Serial.println(GPS.longitude_fixed);
  Serial.print("GPS Time: ");
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
  Serial.print("GPS Commanded State: ");
  if (this->gpsCommandedState == 1) {
    Serial.println("STANDBY");
  } else if (this->gpsCommandedState == 2) {
    Serial.println("WAKEUP");
  } else {
    Serial.println("NOT_SET");      
  }
  Serial.print("GPS Second Since Fix: ");
  Serial.println(GPS.secondsSinceFix());
  Serial.print("Battery Voltage: ");
  Serial.println(this->battery);
}

void SATCOMRelay::checkBatteryVoltage() {
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  this->battery = measuredvbat;
  //Serial.print("VBat: " ); Serial.println(measuredvbat);
}

void SATCOMRelay::getGPSTime(char * buf) {
  sprintf(buf, "%d-%02d-%02d %02d:%02d:%02d",
          GPS.year, GPS.month, GPS.day, GPS.hour, GPS.minute, GPS.seconds);
  #if DEBUG_MODE
  Serial.print("GPS time and date ");
  Serial.println(buf);
  #endif
}