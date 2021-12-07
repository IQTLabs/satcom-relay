#include "satcom-relay.h"

SATCOMRelay relay;

uint32_t gpsTimer, testModePrintTimer, batteryCheckTimer = 2000000000L; // Make all of these times far in the past by setting them near the middle of the millis() range so they are checked promptly

void setup() {
  while(!Serial);
  Serial.begin(115200);

  relay.gps.initGPS();
}

void loop() {
  gpsCheck();

  batteryCheck();

  #if TEST_MODE // print the state of the relay
  if (millis() - testModePrintTimer > TEST_MODE_PRINT_INTERVAL) {
    testModePrintTimer = millis(); // reset the timer
    relay.print();
    Serial.println();
  }
  #endif
}

void gpsCheck() {
  relay.gps.readGPSSerial(); // we need to keep reading in main loop to keep GPS serial buffer clear
  if (millis() - gpsTimer > GPS_WAKEUP_INTERVAL) { 
    relay.gps.gpsWakeup(); // wake up the GPS until we get a fix or timeout
    // TODO: double check this timeout logic
    if (relay.gps.gpsHasFix() || (millis() - gpsTimer > GPS_WAKEUP_INTERVAL+GPS_LOCK_TIMEOUT)) { 
      relay.gps.gpsStandby();
      gpsTimer = millis(); // reset the timer
      #if DEBUG_MODE
      Serial.print("DEBUG: ");if (relay.gps.gpsHasFix()) {Serial.println("GOT GPS FIX");} else {Serial.println("GPS FIX TIMEOUT");}
      #endif
    } 
  }
}

void batteryCheck() {
  if (millis() - batteryCheckTimer > BATTERY_CHECK_INTERVAL) {
    batteryCheckTimer = millis(); // reset the timer
    relay.checkBatteryVoltage();
  }
}