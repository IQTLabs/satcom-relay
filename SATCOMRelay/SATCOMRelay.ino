#include "SATCOMRelay.h"

SATCOMRelay relay;

uint32_t gpsTimer, testModePrintTimer, batteryCheckTimer = 2000000000L; // Make all of these times far in the past by setting them near the middle of the millis() range so they are checked promptly

void setup() {
  while(!Serial);
  Serial.begin(115200);
  relay.initGPS();
  relay.initIridium();
}

void loop() {

  relay.readGPSSerial(); // we need to keep reading in main loop to keep GPS serial buffer clear
  if (millis() - gpsTimer > GPS_WAKEUP_INTERVAL) { // wake up the GPS until we get a fix or timeout
    relay.gpsWakeup();
    // TODO: double check this timeout logic
    if (relay.gpsFix() || (millis() - gpsTimer > GPS_WAKEUP_INTERVAL+GPS_LOCK_TIMEOUT)) { 
      relay.gpsStandby();
      gpsTimer = millis(); // reset the timer
      #if DEBUG_MODE
      Serial.print("DEBUG: ");if (relay.gpsFix()) {Serial.println("GOT GPS FIX");} else {Serial.println("GPS FIX TIMEOUT");}
      #endif
    } 
    //relay.printGPS();
  }

  if (millis() - batteryCheckTimer > BATTERY_CHECK_INTERVAL) { 
    relay.checkBatteryVoltage();
  }

  #if TEST_MODE // print the state of the relay
  if (millis() - testModePrintTimer > TEST_MODE_PRINT_INTERVAL) {
    testModePrintTimer = millis(); // reset the timer
    relay.print();
    relay.getIridiumIMEI();
    relay.getIridiumTime();
    Serial.println();
  }
  #endif

}

#if IRIDIUM_DIAGNOSTICS
void ISBDConsoleCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}

void ISBDDiagsCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}
#endif