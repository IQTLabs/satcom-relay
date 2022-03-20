#include "messagelog.h"

// MessageLog constructs a new MessageLog object. Set activityLEDPin to < 1 to
// disable activity LED functionality.
MessageLog::MessageLog(const char* filename, int sdChipSelectPin, int sdCardDetectPin, int activityLEDPin) {
  this->filename = filename;
  this->sdChipSelectPin = sdChipSelectPin;
  this->sdCardDetectPin = sdCardDetectPin;
  this->activityLEDPin = activityLEDPin;

  // Ensure SD card pin modes are configured
  pinMode(this->activityLEDPin, OUTPUT);
  pinMode(this->sdCardDetectPin, INPUT_PULLUP);
  if (this->activityLEDPin >= 0) {
    pinMode(this->activityLEDPin, OUTPUT);
  }
}

// Wrapper for SDLib::File::println() which implements an activity LED.
// Returns number of bytes written.
int MessageLog::append(String *message) {
  message->trim();
  return this->append(message->c_str());
}

// Wrapper for SDLib::File::println() which implements an activity LED.
// Returns number of bytes written.
int MessageLog::append(const char *message) {
  size_t s = 0;
  ledOn();

  // Check SD card status before proceding
  if (digitalRead(this->sdCardDetectPin) == LOW) {
    MESSAGELOG_PRINTLN(F("SD card not inserted."));
    ledOff();
    return s;
  }
  if (!SD.begin(this->sdChipSelectPin)) {
    MESSAGELOG_PRINTLN(F("Error initializing SD card interface."));
    ledOff();
    return s;
  }

  File file = SD.open(this->filename, FILE_WRITE);
  if (!file) {
    MESSAGELOG_PRINTLN("Unable to open " + this->filename + " with mode FILE_WRITE");
    ledOff();
    return s;
  }
  s = file.println(message);
  file.close();
  ledOff();
  return s;
}

// ledOn sets the led pin high
void MessageLog::ledOn() {
  if (this->activityLEDPin >= 0) {
    digitalWrite(this->activityLEDPin, HIGH);
  }
}

// ledOff sets the led pin low
void MessageLog::ledOff() {
  if (this->activityLEDPin >= 0) {
    digitalWrite(this->activityLEDPin, LOW);
  }
}
