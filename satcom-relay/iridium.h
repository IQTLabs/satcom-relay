IridiumSBD modem(IridiumSerial);
#define DIAGNOSTICS false // Change this to see diagnostics
uint32_t iridiumTimer = millis();

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
