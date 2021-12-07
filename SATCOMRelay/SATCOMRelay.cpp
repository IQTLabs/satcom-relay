#include "SATCOMRelay.h"

SATCOMRelay::SATCOMRelay() {
  pinMode(GPS_EN_PIN, OUTPUT);
  pinMode(VBATPIN, INPUT);
}

void SATCOMRelay::print() {
  Serial.println("SATCOM RELAY STATUS:");
  Serial.print("GPS Last Fix Latitude: ");
  Serial.println(gps.getLastFixLatitude());
  Serial.print("GPS Last Fix Longitude: ");
  Serial.println(gps.getLastFixLongitude());
  Serial.print("GPS Last Fix Time: ");
  Serial.println(gps.getLastFixDate());
  Serial.print("GPS Seconds Since Last Fix: ");
  Serial.println(gps.getSecondsSinceLastFix());
  Serial.print("GPS Commanded State: ");
  Serial.println(gps.getGPSCommandedStateString());
  Serial.print("Battery Voltage: ");
  Serial.println(this->battery);
}

void SATCOMRelay::checkBatteryVoltage() {
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  this->battery = measuredvbat;
}

