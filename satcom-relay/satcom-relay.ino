#include <ArduinoJson.h>
#include "timediff.h"
#include "sleepmanager.h"
#include "satcom-relay.h"

SATCOMRelay relay;

const char fwVersion[] = "2";
const byte readBufferSize = 184;
const int jsonBufferSize = 256;
const byte wakeupRetries = 30;

#define interruptPin 15
#define AWAKE_INTERVAL (60 * 1000)
SleepManager sleepmanager(digitalPinToInterrupt(interruptPin), AWAKE_INTERVAL);

volatile uint32_t gpsTimer, gpsBootTimer, testModePrintTimer, batteryCheckTimer, ledBlinkTimer = 2000000000L; // Make all of these times far in the past by setting them near the middle of the millis() range so they are checked promptly
byte i = 0;
char readBuffer[readBufferSize] = {0};
bool iridium_wakeup_state = false;
bool hasFixOnBoot = false;
bool gpsBooted = false;
DynamicJsonDocument doc(jsonBufferSize);

Uart IridiumInterfaceSerial(&sercom1, IRIDIUM_INTERFACE_RX_PIN, IRIDIUM_INTERFACE_TX_PIN, IRIDIUM_INTERFACE_RX_PAD, IRIDIUM_INTERFACE_TX_PAD);
Uart SensorSerial(&sercom2, SENSOR_RX_PIN, SENSOR_TX_PIN, SENSOR_RX_PAD, SENSOR_TX_PAD);

void SERCOM1_Handler()
{
  IridiumInterfaceSerial.IrqHandler();
}

void SERCOM2_Handler()
{
  SensorSerial.IrqHandler();
}

class IridiumModem
{
  public:
    void begin(Uart *uart);
    void wakeup();
    void check();
    void sendJSON(const DynamicJsonDocument &doc);
    Uart *modemUart;
  private:
    bool wakeup_state;
};

void IridiumModem::begin(Uart *modemUart) {
  this->modemUart = modemUart;
  this->wakeup_state = false;
  this->modemUart->begin(57600);

  pinMode(IRIDIUM_INTERFACE_WAKEUP_PIN, OUTPUT);
  digitalWrite(IRIDIUM_INTERFACE_WAKEUP_PIN, this->wakeup_state);
  // Assign IRIDIUM_INTERFACE pins SERCOM functionality
  pinPeripheral(IRIDIUM_INTERFACE_RX_PIN, PIO_SERCOM);
  pinPeripheral(IRIDIUM_INTERFACE_TX_PIN, PIO_SERCOM);
}

void IridiumModem::wakeup() {
   this->wakeup_state = !wakeup_state;
   digitalWrite(IRIDIUM_INTERFACE_WAKEUP_PIN, this->wakeup_state);
   // Give the modem a chance to wakeup to receive the message.
   // TODO: the modem could also verify JSON to make sure it got a complete message and ask for a retry if necessary.
   delay(1000);
}

// If the Iridium Interface MCU misses the initial message because it was sleeping it will send a \n to request a resend
void IridiumModem::check() {
  bool sawNewline = false;
  while (IridiumInterfaceSerial.available()) {
    if (IridiumInterfaceSerial.read() == '\n') {
      sawNewline = true;
    }
  }
  if (sawNewline) {
    Serial.println("Received newline/resend request from Iridium Interface");
    // TODO check this before sending
    //serializeJson(doc, IridiumInterfaceSerial);
  }
}

void IridiumModem::sendJSON(const DynamicJsonDocument &doc) {
   serializeJson(doc, *(this->modemUart));
   this->modemUart->println();
}

IridiumModem modem;

void setup() {
  Serial.begin(115200);

  // message connection
  memset(readBuffer, 0, sizeof(readBuffer));
  SensorSerial.begin(57600);

  pinMode(LED_BUILTIN, OUTPUT);

  modem.begin(&IridiumInterfaceSerial);

  // Assign SENSOR pins SERCOM functionality
  pinPeripheral(SENSOR_RX_PIN, PIO_SERCOM);
  pinPeripheral(SENSOR_TX_PIN, PIO_SERCOM);

  relay.gps.initGPS();
}

void loop() {
  msgCheck();
  // wait for a warm fix on boot
  if (hasFixOnBoot) {
    relay.gps.gpsStandby();
    gpsBooted = true;
  } else if (!hasFixOnBoot && !timeExpired(&gpsBootTimer, GPS_BOOT_TIMEOUT, true)) {
    hasFixOnBoot = gpsCheck(true);
  } else {
    gpsCheck(false);
    gpsBooted = true;
  }
  batteryCheck();
  sleepCheck();
  ledBlinkCheck();
  modem.check();

  #if TEST_MODE // print the state of the relay
  if (timeExpired(&testModePrintTimer, TEST_MODE_PRINT_INTERVAL, true)) {
    relay.print();
    Serial.println();
  }
  #endif
}

void msgCheck() {
  // Read from SensorSerial
  if (getSensorSerial()) {
    handleReadBuffer();
  }
}

// Periodically check the GPS for current position
bool gpsCheck(bool forceCheck) {
  bool hasFix = false;
  relay.gps.readGPSSerial(); // we need to keep reading in main loop to keep GPS serial buffer clear
  if (forceCheck || timeExpired(&gpsTimer, GPS_WAKEUP_INTERVAL, false)) {
    relay.gps.gpsWakeup(); // wake up the GPS until we get a fix or timeout
    hasFix = relay.gps.gpsHasFix();
    if (hasFix || timeExpired(&gpsTimer, GPS_WAKEUP_INTERVAL+GPS_LOCK_TIMEOUT, true)) {
      relay.gps.gpsStandby();
      #if DEBUG_MODE
      Serial.print("DEBUG: ");if (relay.gps.gpsHasFix()) {Serial.println("GOT GPS FIX");} else {Serial.println("GPS FIX TIMEOUT");}
      #endif
    }
  }
  return hasFix;
}

// Periodically check the battery level and update member variable
void batteryCheck() {
  if (timeExpired(&batteryCheckTimer, BATTERY_CHECK_INTERVAL, true)) {
    relay.checkBatteryVoltage();
  }
}

void sleepCheck() {
  if ((hasFixOnBoot || gpsBooted) && sleepmanager.SleepTime()) {
    // set pin mode to low
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("sleeping as timed out");
    // make sure GPS also goes to sleep
    relay.gps.gpsStandby();
    delay(500);
    #ifdef WINDOWS_DEV
    USBDevice.detach();
    #else
    USBDevice.standby();
    #endif
    sleepmanager.WFI();
    #ifdef WINDOWS_DEV
    USBDevice.attach();
    #endif
    delay(1000);
    Serial.println("wake due to interrupt");
    Serial.println();
    // request repeat of last message.
    // commenting out as it seems to have enough time to wake
    // SensorSerial.println();
    // toggle output of built-in LED pin
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

bool getSensorSerial() {
  if (SensorSerial.available()) {
    char c = SensorSerial.read();
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

void handleReadBuffer() {
  doc.clear();
  // https://arduinojson.org/v6/issues/garbage-out/
  DeserializationError error = deserializeJson(doc, (const char*)readBuffer);
  if (error) {
    Serial.print("FAILED: trying to parse JSON: ");
    Serial.println(error.c_str());
    doc.clear();
  } else {
    bool isDevice = doc.containsKey("D");
    if (!isDevice) {
      Serial.print("Ignoring message without device key");
      doc.clear();
    } else {
      bool isHeartbeat =  doc.containsKey("H");
      if (isHeartbeat) {
        byte j = 0;
        for (; j < wakeupRetries; ++j) {
          if (gpsCheck(true)) {
            break;
          }
          delay(1000);
        }
      }
      doc["u_ms"] = millis();
      doc["v"] = fwVersion;
      doc["lat"] = relay.gps.getLastFixLatitude();
      doc["lon"] = relay.gps.getLastFixLongitude();
      doc["bat"] = relay.getBatteryVoltage();
      modem.wakeup();
      modem.sendJSON(doc);
      serializeJson(doc, Serial);
      Serial.println();
    }
  }
  memset(readBuffer, 0, sizeof(readBuffer));
}

// Blink to show the Relay MCU is awake
void ledBlinkCheck() {
  if (timeExpired(&ledBlinkTimer, LED_BLINK_TIMER, true)) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}
