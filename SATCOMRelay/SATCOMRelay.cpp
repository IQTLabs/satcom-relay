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

IridiumSBD modem(IridiumSerial);

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
  return GPS.longitude;
}

float SATCOMRelay::getLon() {
  return GPS.longitude;
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

int SATCOMRelay::initIridium() {
  int result;

  IridiumSerial.begin(19200);

  // Assign pins SERCOM functionality
  pinPeripheral(IRIDIUM_RX_PIN, PIO_SERCOM);
  pinPeripheral(IRIDIUM_TX_PIN, PIO_SERCOM);

  // If we're powering the device by USB, tell the library to
  // relax timing constraints waiting for the supercap to recharge.
  modem.setPowerProfile(IridiumSBD::USB_POWER_PROFILE);

  Serial.println("Starting modem...");
  result = modem.begin();

  if (result == ISBD_SUCCESS) {
    Serial.println("Iridium started.");
  } else {
    #if DEBUG_MODE
    printIridiumError(results);
    #endif
  }

  return result;
}

int SATCOMRelay::getIridiumIMEI() {
  int result;
  char IMEI[16];
  result = modem.getIMEI(IMEI, sizeof(IMEI));
  
  if (result == ISBD_SUCCESS) {
    Serial.print("Iridium IMEI: ");
    Serial.println(IMEI);
  } else {
    #if DEBUG_MODE
    printIridiumError(results);
    #endif
  }
  
  return result;
}

int SATCOMRelay::getIridiumTime() {
  struct tm t;
  int result = modem.getSystemTime(t);
  
  if (result == ISBD_SUCCESS) {
    char buf[32];
    sprintf(buf, "%d-%02d-%02d %02d:%02d:%02d",
            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    Serial.print("Iridium time/date is ");
    Serial.println(buf);
  } else {
    #if DEBUG_MODE
    printIridiumError(results);
    #endif
  }

  return result;
}

void SATCOMRelay::printIridiumError(int error) {
  Serial.print("ISBD ERROR CODE: ");
  switch (error) {
  case ISBD_SUCCESS:
    Serial.println("ISBD_SUCCESS");
    break;
  case ISBD_ALREADY_AWAKE:
    Serial.println("ISBD_ALREADY_AWAKE ");
    break;
  case ISBD_SERIAL_FAILURE:
    Serial.println("ISBD_SERIAL_FAILURE");
    break;
  case ISBD_PROTOCOL_ERROR:
    Serial.println("ISBD_PROTOCOL_ERROR");
    break;
  case ISBD_CANCELLED:
    Serial.println("ISBD_CANCELLED");
    break;
  case ISBD_NO_MODEM_DETECTED:
    Serial.println("ISBD_NO_MODEM_DETECTED");
    break;
  case ISBD_SBDIX_FATAL_ERROR:
    Serial.println("ISBD_SBDIX_FATAL_ERROR");
    break;
  case ISBD_SENDRECEIVE_TIMEOUT:
    Serial.println("ISBD_SENDRECEIVE_TIMEOUT");
    break;
  case ISBD_RX_OVERFLOW:
    Serial.println("ISBD_RX_OVERFLOW");
    break;
  case ISBD_REENTRANT:
    Serial.println("ISBD_REENTRANT");
    break;
  case ISBD_IS_ASLEEP:
    Serial.println("ISBD_IS_ASLEEP");
    break;
  case ISBD_NO_SLEEP_PIN:
    Serial.println("ISBD_NO_SLEEP_PIN");
    break;
  case ISBD_NO_NETWORK:
    Serial.println("ISBD_NO_NETWORK");
    break;
  case ISBD_MSG_TOO_LONG:
    Serial.println("ISBD_MSG_TOO_LONG");
    break;
  default:
    Serial.println("Error finding the error code. Lol.");
    break;
  }
}