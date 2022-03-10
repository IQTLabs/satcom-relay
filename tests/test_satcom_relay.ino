#line 2 "test_satcom_relay.ino"

#include <Arduino.h>
#include <AUnit.h>

class Uart {
  public:
    Uart() {};
    bool available() { return p < l; }
    char read() { return buffer[p++]; }
    void setbuf(const char *test_str) { buffer = test_str; l = strlen(test_str); p = 0; };
    byte p = 0;
    byte l = 0;
    const char *buffer = NULL;
};

Uart uart;

#include "../satcom-relay/timediff.h"
#include "../satcom-relay/sensor-manager.h"

using aunit::TestRunner;

const char okJson[] = "{\"D\": 1}\n";
const char okHbJson[] = "{\"D\": 1, \"H\": 1}\n";
const char badJson[] = "{/notJson\n";

test(parse, check) {
  bool isHeartbeat = false;
  DynamicJsonDocument doc(256);
  SensorSerialManager ssm(&uart, &doc);
  assertEqual(false, ssm.poll());
  uart.setbuf(okJson);
  while (!ssm.poll()) {};
  assertEqual(true, ssm.parse(&isHeartbeat));
  assertEqual(false, isHeartbeat); 
  uart.setbuf(okHbJson);
  while (!ssm.poll()) {};
  assertEqual(true, ssm.parse(&isHeartbeat));
  assertEqual(true, isHeartbeat);
  uart.setbuf(badJson);
  while (!ssm.poll()) {};
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
