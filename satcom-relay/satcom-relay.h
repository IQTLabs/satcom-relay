#ifndef SATCOMRelay_H
#define SATCOMRelay_H

#include <Arduino.h>
#include "gps.h"

#define TEST_MODE true
#define TEST_MODE_PRINT_INTERVAL 2000 // use for testing. prints relay status to Serial

#define DEBUG_MODE false // print lots of debugging messages

#define VBATPIN 9 //A7
#define BATTERY_CHECK_INTERVAL 10000

class SATCOMRelay {

private:
  float battery = -1;

public:
  GPS gps;

  SATCOMRelay();
  void print();
  void checkBatteryVoltage();
};

#endif
