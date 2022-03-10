#line 2 "test_satcom_relay.ino"

#include <Arduino.h>
#include <AUnit.h>
#include <StdioSerial.h>

typedef StdioSerial Uart;
#include "../satcom-relay/timediff.h"
#include "../satcom-relay/sensor-manager.h"

using aunit::TestRunner;

const char okJson[] = "{\"D\": 1}";
const char okHbJson[] = "{\"D\": 1, \"H\": 1}";
const char badJson[] = "{/notJson";

test(parse, check) {
  bool isHeartbeat = false;
  DynamicJsonDocument doc(256);
  SensorSerialManager ssm(NULL, &doc);
  ssm.resetBuffer();
  memcpy(ssm.readBuffer, okJson, sizeof(okJson));
  assertEqual(true, ssm.parse(&isHeartbeat));
  assertEqual(false, isHeartbeat); 
  memcpy(ssm.readBuffer, okHbJson, sizeof(okHbJson));
  assertEqual(true, ssm.parse(&isHeartbeat));
  assertEqual(true, isHeartbeat);
  memcpy(ssm.readBuffer, badJson, sizeof(badJson));
  assertEqual(false, ssm.parse(&isHeartbeat));
  assertEqual(false, isHeartbeat);
}

test(timeExpired, check) {
  unsigned long x = millis();
  assertEqual(false, timeExpired(&x, 500, false));
  delay(501);
  assertEqual(true, timeExpired(&x, 500, true));
}

test(timeDiff, check) {
  unsigned long rollover = timeDiff(ULONG_MAX, 1);
  assertEqual((unsigned long)1, rollover);
  unsigned long nonrollover = timeDiff(10, 20);
  assertEqual((unsigned long)10, nonrollover);
}

void setup() {
}

void loop() {
  TestRunner::run();
}
