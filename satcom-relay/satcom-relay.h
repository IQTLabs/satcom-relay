#ifndef SATCOMRelay_H
#define SATCOMRelay_H

#include <Arduino.h>
#include "gps.h"

#define TEST_MODE true
#define TEST_MODE_PRINT_INTERVAL 2000 // use for testing. prints relay status to Serial

#define DEBUG_MODE false // print lots of debugging messages

#define VBATPIN 9 //A7
#define BATTERY_CHECK_INTERVAL 10000
#define AWAKE_INTERVAL (60 * 1000)

#define LED_BLINK_TIMER 500

#define IRIDIUM_INTERFACE_WAKEUP_PIN 19
#define IRIDIUM_INTERFACE_RX_PIN 10
#define IRIDIUM_INTERFACE_TX_PIN 11
#define IRIDIUM_INTERFACE_RX_PAD SERCOM_RX_PAD_2
#define IRIDIUM_INTERFACE_TX_PAD UART_TX_PAD_0

class SATCOMRelay {

private:
  float battery = -1;

public:
  GPS gps;

  SATCOMRelay();
  void print();
  void checkBatteryVoltage();
  String getBatteryVoltage();
};

#endif
