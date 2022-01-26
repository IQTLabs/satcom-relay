#include <SPI.h>
#include <SD.h>

// Comment this line out to disable Serial console logging (or redefine as desired)
#define MESSAGELOG_PRINTLN(x) {Serial.print(" ");Serial.print(__LINE__);Serial.print(":\t");Serial.println(x);}

#ifndef MESSAGELOG_PRINTLN
#define MESSAGELOG_PRINTLN(x) (0)
#endif

#ifndef _MESSAGELOG_H
#define _MESSAGELOG_H

// MessageLog implements a LIFO stack with a file on an SD card
class MessageLog {
  public:
    MessageLog(const char *filename, int sdChipSelectPin, int sdCardDetectPin, int activityLEDPin);
    int push(String *newMessage);
    void pop(String *message);
    int numMessages();
    void dumpToSerial();
  private:
    String filename;
    int sdChipSelectPin, sdCardDetectPin, activityLEDPin;
    void ledOn();
    void ledOff();
    unsigned int copyBytes(const char *sourceFilename, const char *destFilename, unsigned int start, unsigned int count);
    size_t write(uint8_t);
    bool read(uint32_t position, char *x);
    size_t size();
    int normalize();
};

#endif
