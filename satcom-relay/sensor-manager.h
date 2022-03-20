#include <ArduinoJson.h>

const byte readBufferSize = 184;

class SensorSerialManager {
  public:
    SensorSerialManager(Uart *sensor, DynamicJsonDocument *doc);
    void resetBuffer();
    bool poll();
    bool parse(bool *isHeartbeat);
    char readBuffer[readBufferSize] = {0};
  private:
    byte i = 0;
    Uart *_sensor;
    DynamicJsonDocument *_doc;
};

SensorSerialManager::SensorSerialManager(Uart *sensor, DynamicJsonDocument *doc) {
  _sensor = sensor; _doc = doc; resetBuffer();
}

void SensorSerialManager::resetBuffer() {
  memset(readBuffer, 0, sizeof(readBuffer));
}

bool SensorSerialManager::poll() {
  if (_sensor->available()) {
    char c = _sensor->read();
    Serial.print(c);
    if (i == (sizeof(readBuffer) - 1)) {
      c = 0;
    }
    if (c == '\n' || c == '\r') {
      c = 0;
    }
    readBuffer[i] = c;
    if (c == 0) {
      i = 0;
      Serial.println();
      return true;
    } else {
      ++i;
    }
  }
  return false;
}

bool SensorSerialManager::parse(bool *isHeartbeat) {
  _doc->clear();
  // https://arduinojson.org/v6/issues/garbage-out/
  DeserializationError error = deserializeJson(*_doc, (const char*)readBuffer);
  if (error) {
    Serial.print("FAILED: trying to parse JSON: ");
    Serial.println(error.c_str());
  } else {
    bool isDevice = _doc->containsKey("D");
    if (!isDevice) {
      Serial.print("Ignoring message without device key");
    } else {
      *isHeartbeat = _doc->containsKey("H");
      return true;
    }
  }
  *isHeartbeat = false;
  resetBuffer();
  _doc->clear();
  return false;
}
