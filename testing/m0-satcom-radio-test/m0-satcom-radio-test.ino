#include <IridiumSBD.h>
#include <time.h>
#include <Adafruit_GPS.h>

#include <Arduino.h>   // required before wiring_private.h
#include "wiring_private.h" // pinPeripheral() function

// GPS
#define RX_PIN 5
#define TX_PIN 22

Uart GPSSerial (&sercom2, RX_PIN, TX_PIN, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM2_Handler()
{
  GPSSerial.IrqHandler();
}
Adafruit_GPS GPS(&GPSSerial);
#define GPSECHO false // Set to 'true' if you want to debug and listen to the raw GPS sentences
uint32_t gpsTimer = millis();

// Iridium
Uart IridiumSerial (&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler()
{
  IridiumSerial.IrqHandler();
}
IridiumSBD modem(IridiumSerial);
#define DIAGNOSTICS true // Change this to see diagnostics
uint32_t iridiumTimer = millis();

void gpsSetup();
void gpsParseSerial();
void gpsPrint();

void iridiumSetup();
void iridiumGetInfo();

void setup() {
  while (!Serial);
  Serial.begin(115200);
  Serial.println("GPS and Iridium Test");

  Serial.println("GPS Setup Started");
  gpsSetup();
  Serial.println("GPS Setup Finished");

  Serial.println("Iridium Setup Started");
  iridiumSetup();
  Serial.println("Iridium Setup Finished");
}

void loop() {
  // GPS
  gpsParseSerial();
  if (millis() - gpsTimer > 2000) { // Print GPS info every 2s
    gpsTimer = millis(); // reset the timer
    Serial.println("GPS TEST");
    gpsPrint();
    Serial.println();
  }

  // Iridium
  if (millis() - iridiumTimer > 2000) { // Print iridium every 2s
    iridiumTimer = millis(); // reset the timer
    Serial.println("Iridium TEST");
    iridiumGetInfo();
    Serial.println();
  }
}

void gpsSetup() {
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);

  // Assign pins 10 & 11 SERCOM functionality
  pinPeripheral(RX_PIN, PIO_SERCOM);
  pinPeripheral(TX_PIN, PIO_SERCOM);
  
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // the parser doesn't care about other sentences at this time
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // Request updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_ANTENNA);

  delay(1000);

  // Ask for firmware version
  GPSSerial.println(PMTK_Q_RELEASE);
}

void gpsParseSerial() {
  // read data from the GPS in the 'main loop'
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  if (GPSECHO)
    if (c) Serial.print(c);
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trying to print out data
    //Serial.print(GPS.lastNMEA()); // this also sets the newNMEAreceived() flag to false
    if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return; // we can fail to parse a sentence in which case we should just wait for another
  }
}

void gpsPrint() {
  Serial.print("Time: ");
  if (GPS.hour < 10) {
    Serial.print('0');
  }
  Serial.print(GPS.hour, DEC); Serial.print(':');
  if (GPS.minute < 10) {
    Serial.print('0');
  }
  Serial.print(GPS.minute, DEC); Serial.print(':');
  if (GPS.seconds < 10) {
    Serial.print('0');
  }
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

void iridiumSetup() {
  int err;

  // Start the serial port connected to the satellite modem
  IridiumSerial.begin(19200);

  // Assign pins 10 & 11 SERCOM functionality
  pinPeripheral(10, PIO_SERCOM);
  pinPeripheral(11, PIO_SERCOM);

  // If we're powering the device by USB, tell the library to
  // relax timing constraints waiting for the supercap to recharge.
  modem.setPowerProfile(IridiumSBD::USB_POWER_PROFILE);

  // Begin satellite modem operation
  Serial.println(F("Starting modem..."));
  err = modem.begin();
  if (err != ISBD_SUCCESS)
  {
    Serial.print(F("Begin failed: error "));
    Serial.println(err);
    if (err == ISBD_NO_MODEM_DETECTED)
      Serial.println(F("No modem detected: check wiring."));
    return;
  }
}

void iridiumGetInfo() {
  int err;
  
  // Get the IMEI
  char IMEI[16];
  err = modem.getIMEI(IMEI, sizeof(IMEI));
  if (err != ISBD_SUCCESS)
  {
    Serial.print(F("getIMEI failed: error "));
    Serial.println(err);
    return;
  }
  Serial.print(F("IMEI is "));
  Serial.print(IMEI);
  Serial.println(F("."));
  
  // Get time
  struct tm t;
  err = modem.getSystemTime(t);
  if (err == ISBD_SUCCESS)
  {
    char buf[32];
    sprintf(buf, "%d-%02d-%02d %02d:%02d:%02d",
            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    Serial.print(F("Iridium time/date is "));
    Serial.println(buf);
  }

  else if (err == ISBD_NO_NETWORK) // Did it fail because the transceiver has not yet seen the network?
  {
    Serial.println(F("No network detected.  Waiting ..."));
  }

  else
  {
    Serial.print(F("Unexpected error "));
    Serial.println(err);
    return;
  }
}

#if DIAGNOSTICS
void ISBDConsoleCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}

void ISBDDiagsCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}
#endif
