#include <ArduinoJson.h>
#include "satcom-relay.h"
#include "timediff.h"
#include "sleepmanager.h"
#include "iridium-modem.h"
#include "sensor-manager.h"

#define SDCARD_ENABLE_LED true

// Ensure MISO/MOSI/SCK pins are not connected to the port replicator board
#include "messagelog.h"
MessageLog *sentMessageLog;
const size_t messageBufferSize = 1024;
char messageBuffer[messageBufferSize];

SATCOMRelay relay;

const char fwVersion[] = "2";
const int jsonBufferSize = 256;
const byte wakeupRetries = 30;
DynamicJsonDocument doc(jsonBufferSize);

#define interruptPin 15
#define AWAKE_INTERVAL (60 * 1000)
SleepManager sleepmanager(digitalPinToInterrupt(interruptPin), AWAKE_INTERVAL);

volatile uint32_t gpsTimer, gpsBootTimer, testModePrintTimer, batteryCheckTimer, ledBlinkTimer = 2000000000L; // Make all of these times far in the past by setting them near the middle of the millis() range so they are checked promptly
bool iridium_wakeup_state = false;
bool hasFixOnBoot = false;
bool gpsBooted = false;

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

IridiumModem modem;
SensorSerialManager ssm(&SensorSerial, &doc);

void setup() {
  sentMessageLog = new MessageLog("sent.txt", SDCardCSPin, SDCardDetectPin, SDCardActivityLEDPin);

  pinMode(IRIDIUM_INTERFACE_WAKEUP_PIN, OUTPUT);
  digitalWrite(IRIDIUM_INTERFACE_WAKEUP_PIN, iridium_wakeup_state);

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  SensorSerial.begin(57600);
  modem.begin(&IridiumInterfaceSerial, IRIDIUM_INTERFACE_WAKEUP_PIN, IRIDIUM_INTERFACE_RX_PIN, IRIDIUM_INTERFACE_TX_PIN);

  // Assign SENSOR pins SERCOM functionality
  pinPeripheral(SENSOR_RX_PIN, PIO_SERCOM);
  pinPeripheral(SENSOR_TX_PIN, PIO_SERCOM);

  relay.gps.initGPS(Serial1);
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
  if (!ssm.poll()) {
    return;
  }
  bool isHeartBeat = false;
  if (ssm.parse(&isHeartBeat)) {
    if (isHeartBeat) {
      for (byte j = 0; j < wakeupRetries; ++j) {
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
    ssm.resetBuffer();
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

<<<<<<< HEAD
void EIC_ISR(void) {
  awakeTimer = millis(); // refresh awake timer.
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
      iridium_wakeup_state = !iridium_wakeup_state;
      digitalWrite(IRIDIUM_INTERFACE_WAKEUP_PIN, iridium_wakeup_state);
      size_t docSize = measureJson(doc);
      serializeJson(doc, &messageBuffer, docSize);
      if (sentMessageLog->append(messageBuffer) < docSize) {
        Serial.println(F("Error from sentMessageLog->append()"));
      }
      // Give the modem a chance to wakeup to receive the message.
      // TODO: the modem could also verify JSON to make sure it got a complete message and ask for a retry if necessary.
      delay(1000);
      IridiumInterfaceSerial.println(messageBuffer);
      Serial.println(messageBuffer);
      memset(messageBuffer, 0, messageBufferSize);
    }
  }
  memset(readBuffer, 0, sizeof(readBuffer));
}

=======
>>>>>>> main
// Blink to show the Relay MCU is awake
void ledBlinkCheck() {
  if (timeExpired(&ledBlinkTimer, LED_BLINK_TIMER, true)) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}
