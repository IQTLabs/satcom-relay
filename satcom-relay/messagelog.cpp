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

// Wrapper for SDLib::File::read() which operates atomically on a File as well
// as implements an activity LED
bool MessageLog::read(uint32_t position, char *x) {
  ledOn();
  bool readStatus = false;
  File file = SD.open(this->filename, FILE_READ);
  if (file) {
    file.seek(position);
    int y = file.read();
    if (y != -1) {
      readStatus = true;
      *x = y;
    }
  }
  file.close();
  ledOff();
  return readStatus;
}

// Wrapper for SDLib::File::write() which operates atomically on a File as well
// as implements an activity LED. Returns number of bytes written.
size_t MessageLog::write(uint8_t c) {
  ledOn();
  File file = SD.open(this->filename, FILE_WRITE);
  size_t s = 0;
  if (!file) {
    MESSAGELOG_PRINTLN("Unable to open " + this->filename + " with mode FILE_WRITE");
    ledOff();
    return s;
  }
  s = file.write(c);
  file.close();
  ledOff();
  return s;
}

// Wrapper for SDLib::File::size()
size_t MessageLog::size() {
  ledOn();
  size_t s = 0;
  File file = SD.open(this->filename, FILE_READ);
  if (file) {
    s = file.size();
  }
  file.close();
  ledOff();
  return s;
}

// normalize ensures the underlying file is properly formatted and is idempotent
int MessageLog::normalize() {
  // Ensure newline is at end of file
  int s = size();
  if (s == 0) {
    if (write('\n') != 1) {
      MESSAGELOG_PRINTLN("Error adding newline to " + this->filename);
      return -1;
    }
    return 0;
  }
  char c;
  if (!read(size() - 1, &c)) {
    MESSAGELOG_PRINTLN("Error initializing " + this->filename);
    return -1;
  }
  if (c != '\n') {
    if (write('\n') != 1) {
      MESSAGELOG_PRINTLN("Error adding newline to " + this->filename);
      return -1;
    }
  }
  return 0;
}

// Dump contents of this->filename to Serial
void MessageLog::dumpToSerial() {
  normalize();
  Serial.println(F("----------"));
  size_t s = size();
  char c;
  for (size_t i = 0; i < s; i++) {
    read(i, &c);
    Serial.print(c);
  }
  Serial.println(F("----------"));
}

// push places a String on the stack
int MessageLog::push(String *message) {
  if (normalize() == -1) {
    return -1;
  }
  message->trim();
  // Make sure message is terminated with a newline
  message->concat('\n');
  for (size_t i = 0; i < message->length(); i++) {
    if (write(message->charAt(i)) != sizeof(message->charAt(i))) {
      MESSAGELOG_PRINTLN(F("push write() failed"));
      return -1;
    }
  }
  return 0;
}

// pop removes and returns the most recent String on the stack
void MessageLog::pop(String *message) {
  *message = "";
  normalize();
  // The majority of this method is a workaround for the fact that some versions
  // of the SD library don't support having multiple files open at once.
  size_t s = size();
  // Find penultimate newline and note position
  if (s == 0) {
    return;
  }
  int penultimateNewline = -1, curNewline = -1;
  char c;
  for (size_t i = 0; i < s; i++) {
    if (!read(i, &c)) {
      MESSAGELOG_PRINTLN("cannot read " + this->filename);
      return;
    }
    if (c == '\n') {
      penultimateNewline = curNewline;
      curNewline = i;
    }
  }

  if (curNewline == -1) {
    MESSAGELOG_PRINTLN(F("pop() no newlines found"));
    return;
  }

  // Get last line
  for (size_t i = penultimateNewline; i < s; i++) {
    if (!read(i, &c)) {
      MESSAGELOG_PRINTLN(F("pop() cannot find last line"));
      *message = "";
      return;
    }
    message->concat(c);
  }

  // CopyBytes from 0 to the second to last newline position to temp file
  char tempFilename[16] = {0};
  snprintf(tempFilename, sizeof(tempFilename), "%d.txt", (uint16_t)millis());

  // Delete and recreate the temp file first in case it already exists
  SD.remove(tempFilename);
  File temp = SD.open(tempFilename, (O_READ | O_WRITE | O_CREAT));
  temp.close();

  // Copy everything to the temp file
  for (size_t i = 0; i < s; i++) {
    if (!read(i, &c)) {
      *message = "";
      MESSAGELOG_PRINTLN(F("pop() cannot read #2"));
      return;
    }
    File temp = SD.open(tempFilename, FILE_WRITE);
    if (!temp) {
      MESSAGELOG_PRINTLN(F("Unable to create temp file"));
      MESSAGELOG_PRINTLN(tempFilename);
      *message = "";
      return;
    }
    if (temp.write(c) != 1) {
      MESSAGELOG_PRINTLN(F("Error writing to temp file"));
      MESSAGELOG_PRINTLN(tempFilename);
      temp.close();
      *message = "";
      return;
    }
    temp.close();
  }
  // Write everything except the last line back to this->filename
  SD.remove(this->filename);
  for (int i = 0; i < penultimateNewline + 1; i++) {
    File temp = SD.open(tempFilename, FILE_READ);
    if (!temp) {
      MESSAGELOG_PRINTLN(F("Unable to open temp file"));
      MESSAGELOG_PRINTLN(tempFilename);
      *message = "";
      return;
    }
    temp.seek(i);
    int c = temp.read();
    if (c == -1) {
      MESSAGELOG_PRINTLN(F("Error reading from temp file"));
      MESSAGELOG_PRINTLN(tempFilename);
      temp.close();
      *message = "";
      return;
    }
    write((char)c);
    temp.close();
  }

  // Delete temp file
  SD.remove(tempFilename);
  // Return line
  message->trim();
}

// numMessages returns the number of messages in the stack
int MessageLog::numMessages() {
  size_t s = size();
  int num = 0;
  // start with i = 1 since an "empty" normalized file will still have a
  // newline as the 0th character
  if (s < 2) {
    return 0;
  }
  char c;
  for (size_t i = 1; i < s; i++) {
    if (!read(i, &c)) {
      MESSAGELOG_PRINTLN("numMessages(): error reading from " + this->filename + " at " + i + " length " + s);
      return -1;
    }
    if (c == '\n') {
      num++;
    }
  }
  return num;
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
