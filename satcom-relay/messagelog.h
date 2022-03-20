#include <SPI.h>
#include <SD.h>

// Comment this line out to disable Serial console logging (or redefine as desired)
#define MESSAGELOG_PRINTLN(x) {Serial.print(" ");Serial.print(__LINE__);Serial.print(":\t");Serial.println(x);}

#ifndef MESSAGELOG_PRINTLN
#define MESSAGELOG_PRINTLN(x) (0)
#endif

#ifndef _MESSAGELOG_H
#define _MESSAGELOG_H

#define SDCardCSPin 4
#define SDCardDetectPin 7
#define SDCardActivityLEDPin (SDCARD_ENABLE_LED ? 8 : -1)

// MessageLog implements a LIFO stack with a file on an SD card
class MessageLog {
  public:
    MessageLog(const char *filename, int sdChipSelectPin, int sdCardDetectPin, int activityLEDPin);
    int append(String *message);
    int append(const char *message);
    void dumpToSerial();
  private:
    String filename;
    int sdChipSelectPin, sdCardDetectPin, activityLEDPin;
    void ledOn();
    void ledOff();
};

#endif
