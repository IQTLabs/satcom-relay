#include <ArduinoJson.h>
#include "satcom-relay.h"

SATCOMRelay relay;

#define interruptPin 15
const char fwVersion[] = "1.0.0";
const byte readBufferSize = 255;
const int jsonBufferSize = 384;
const byte wakeupRetries = 30;

volatile uint32_t awakeTimer, gpsTimer, testModePrintTimer, batteryCheckTimer, ledBlinkTimer = 2000000000L; // Make all of these times far in the past by setting them near the middle of the millis() range so they are checked promptly
byte i = 0;
char readBuffer[readBufferSize] = {0};
bool iridium_wakeup_state = false;
DynamicJsonDocument doc(jsonBufferSize);

Uart IridiumInterfaceSerial (&sercom1, IRIDIUM_INTERFACE_RX_PIN, IRIDIUM_INTERFACE_TX_PIN, IRIDIUM_INTERFACE_RX_PAD, IRIDIUM_INTERFACE_TX_PAD);

void SERCOM1_Handler()
{
  IridiumInterfaceSerial.IrqHandler();
}

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

void setup() {
  pinMode(IRIDIUM_INTERFACE_WAKEUP_PIN, OUTPUT);
  digitalWrite(IRIDIUM_INTERFACE_WAKEUP_PIN, iridium_wakeup_state);

  Serial.begin(115200);

  // message connection
  memset(readBuffer, 0, sizeof(readBuffer));
  Serial1.begin(57600);

  IridiumInterfaceSerial.begin(57600);

  pinMode(LED_BUILTIN, OUTPUT);

  // Assign pins 10 & 11 SERCOM functionality
  pinPeripheral(IRIDIUM_INTERFACE_RX_PIN, PIO_SERCOM);
  pinPeripheral(IRIDIUM_INTERFACE_TX_PIN, PIO_SERCOM);

  relay.gps.initGPS();

  // Setup interrupt sleep pin
  setupInterruptSleep();
}

void loop() {
  msgCheck();
  gpsCheck(false);
  batteryCheck();
  sleepCheck();
  ledBlinkCheck();
  iridiumInterfaceCheck();

  #if TEST_MODE // print the state of the relay
  if (timeExpired(&testModePrintTimer, TEST_MODE_PRINT_INTERVAL, true)) {
    relay.print();
    Serial.println();
  }
  #endif
}

void setupInterruptSleep() {
  // whenever we get an interrupt, reset the awake clock.
  attachInterrupt(digitalPinToInterrupt(interruptPin), EIC_ISR, CHANGE);
  // Set external 32k oscillator to run when idle or sleep mode is chosen
  SYSCTRL->XOSC32K.reg |=  (SYSCTRL_XOSC32K_RUNSTDBY | SYSCTRL_XOSC32K_ONDEMAND);
  REG_GCLK_CLKCTRL  |= GCLK_CLKCTRL_ID(GCM_EIC) | // generic clock multiplexer id for the external interrupt controller
                       GCLK_CLKCTRL_GEN_GCLK1 |   // generic clock 1 which is xosc32k
                       GCLK_CLKCTRL_CLKEN;        // enable it
  // Write protected, wait for sync
  while (GCLK->STATUS.bit.SYNCBUSY);

  // Set External Interrupt Controller to use channel 4
  EIC->WAKEUP.reg |= EIC_WAKEUP_WAKEUPEN4;

  PM->SLEEP.reg |= PM_SLEEP_IDLE_CPU;  // Enable Idle0 mode - sleep CPU clock only
  //PM->SLEEP.reg |= PM_SLEEP_IDLE_AHB; // Idle1 - sleep CPU and AHB clocks
  //PM->SLEEP.reg |= PM_SLEEP_IDLE_APB; // Idle2 - sleep CPU, AHB, and APB clocks

  // It is either Idle mode or Standby mode, not both.
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;   // Enable Standby or "deep sleep" mode
}

void msgCheck() {
  // Read from Serial1
  if (getSerial1()) {
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
  if (timeExpired(&awakeTimer, AWAKE_INTERVAL, true)) {
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
    __WFI();  // wake from interrupt
    #ifdef WINDOWS_DEV
    USBDevice.attach();
    #endif
    delay(1000);
    Serial.println("wake due to interrupt");
    Serial.println();
    // request repeat of last message.
    Serial1.println();
    // toggle output of built-in LED pin
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void EIC_ISR(void) {
  awakeTimer = millis(); // refresh awake timer.
}

bool getSerial1() {
  if (Serial1.available()) {
    char c = Serial1.read();
    if (i == (sizeof(readBuffer) - 1)) {
      c = 0;
    }
    if (c == '\n' || c == '\r') {
      c = 0;
    }
    readBuffer[i] = c;
    if (c == 0) {
      i = 0;
      return true;
    } else {
      ++i;
    }
  }
  return false;
}

void handleReadBuffer() {
  doc.clear();
  DeserializationError error = deserializeJson(doc, readBuffer);
  if (error) {
    Serial.print("FAILED: trying to parse JSON: ");
    Serial.println(error.c_str());
    doc.clear();
  } else {
    bool isHeartbeat = doc.containsKey("heartbeat");
    if (isHeartbeat) {
      byte j = 0;
      for (; j < wakeupRetries; ++j) {
        if (gpsCheck(true)) {
          break;
        }
        delay(1000);
      }
    }
    doc["uptime_ms"] = millis();
    doc["version"] = fwVersion;
    doc["lat"] = relay.gps.getLastFixLatitude();
    doc["lon"] = relay.gps.getLastFixLongitude();
    doc["bat"] = relay.getBatteryVoltage();
    iridium_wakeup_state = !iridium_wakeup_state;
    digitalWrite(IRIDIUM_INTERFACE_WAKEUP_PIN, iridium_wakeup_state);
    // Give the modem a chance to wakeup to receive the message.
    // TODO: the modem could also verify JSON to make sure it got a complete message and ask for a retry if necessary.
    delay(1000);
    serializeJson(doc, readBuffer, sizeof(readBuffer));
    IridiumInterfaceSerial.println(readBuffer);
    Serial.println(readBuffer);
  }
  memset(readBuffer, 0, sizeof(readBuffer));
}

// Blink to show the Relay MCU is awake
void ledBlinkCheck() {
  if (timeExpired(&ledBlinkTimer, LED_BLINK_TIMER, true)) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}

// If the Iridium Interface MCU misses the initial message because it was sleeping it will send a \n to request a resend
void iridiumInterfaceCheck() {
  bool sawNewline = false;
  while (IridiumInterfaceSerial.available()) {
    if (IridiumInterfaceSerial.read() == '\n') {
      sawNewline = true;
    }
  }
  if (sawNewline) {
    Serial.println("Received newline/resend request from Iridium Interface");
    // TODO check this before sending
    serializeJson(doc, IridiumInterfaceSerial);
  }
}
