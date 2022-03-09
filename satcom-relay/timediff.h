#include "Arduino.h"
#include "limits.h"

unsigned long timeDiff(unsigned long x, unsigned long nowTime) {
  if (nowTime >= x) {
    return nowTime - x;
  }
  return (ULONG_MAX - x) + nowTime;
}

unsigned long nowTimeDiff(unsigned long x) {
  return timeDiff(x, millis());
}

bool timeExpired(volatile unsigned long *x, unsigned long interval, bool reset) {
  if (nowTimeDiff(*x) > interval) {
    if (reset) {
      *x = millis();
    }
    return true;
  }
  return false;
}
