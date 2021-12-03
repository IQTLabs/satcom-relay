#include <time.h>
#include <IridiumSBD.h>
#include <Adafruit_GPS.h>
#include "wiring_private.h" // pinPeripheral() function
#include "serial_config.h"
#include "state.h"
#include "iridium.h"
#include "gps.h"

boolean testMode = true;
uint32_t testModeTimer = millis();

State state = { 0, //battery
                0, //lastGPSUpdate
                0, //lat
                0, //long
                10000, //gpsSleepTimer
                10000};//heartbeatSleepTime
                
void setup() {
  while (!Serial);
  Serial.begin(115200);

  gpsSetup();
  iridiumSetup();
}

/*
 * TODO
 * 
 * Add low power sleep if possible
 * 
 */
void loop() {
  if (testMode) {
    if (millis() - testModeTimer > 2000) {
      testModeTimer = millis();
      //print the test stuff here
      printState(&state);
    }
  }

  /*
   * TODO
   * 
   * restructure the following to:
   * turn gps on (using digital io pin)
   * wait for lock
   * update "state" variable
   * turn gps off
   */
  gpsParseSerial();
  if (millis() - gpsTimer > state.gpsSleepTimer) {
    gpsTimer = millis(); // reset the timer
    Serial.println("GPS TEST");
    gpsPrint();
    Serial.println();
  }

  /*
   * TODO
   * 
   * The following should: 
   * if heartbeat timer is up, send "state"
   * if rf event/data is received, send it and "state"
   */
  if (millis() - iridiumTimer > state.heartbeatSleepTime) {
    iridiumTimer = millis(); // reset the timer
    Serial.println("Iridium TEST");
    iridiumGetInfo();
    Serial.println();
  }
}
