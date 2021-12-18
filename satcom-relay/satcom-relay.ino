#include <ArduinoJson.h>
#include "satcom-relay.h"

SATCOMRelay relay;

const char fwVersion[] = "1.0.0";
uint32_t gpsTimer, testModePrintTimer, batteryCheckTimer, ledBlinkTimer = 2000000000L; // Make all of these times far in the past by setting them near the middle of the millis() range so they are checked promptly
volatile uint32_t awakeTimer = 0;
String msg;
byte i = 0;
const byte bufferSize = 150;
char readBuffer[bufferSize] = {};
DynamicJsonDocument doc(bufferSize);

Uart IridiumInterfaceSerial (&sercom1, IRIDIUM_INTERFACE_RX_PIN, IRIDIUM_INTERFACE_TX_PIN, IRIDIUM_INTERFACE_RX_PAD, IRIDIUM_INTERFACE_TX_PAD);
void SERCOM1_Handler()
{
  IridiumInterfaceSerial.IrqHandler();
}

#define interruptPin 15

unsigned long timeDiff(unsigned long x, unsigned long nowTime) {
  if (nowTime >= x) {
    return nowTime - x;
  }
  return (ULONG_MAX - x) + nowTime;
}

unsigned long nowTimeDiff(unsigned long x) {
  return timeDiff(x, millis());
}

void setup() {
  //while(!Serial);
  Serial.begin(115200);

  // RF connection
  memset(readBuffer, 0, sizeof(readBuffer));
  Serial1.begin(57600);

  pinMode(LED_BUILTIN, OUTPUT);

  // Assign pins 10 & 11 SERCOM functionality
  pinPeripheral(IRIDIUM_INTERFACE_RX_PIN, PIO_SERCOM);
  pinPeripheral(IRIDIUM_INTERFACE_TX_PIN, PIO_SERCOM);

  relay.gps.initGPS();

  // Setup interrupt sleep pin
  setupInterruptSleep();
}

void loop() {
  rfCheck();
  gpsCheck();
  batteryCheck();
  sleepCheck();
  ledBlinkCheck();

  #if TEST_MODE // print the state of the relay
  if (nowTimeDiff(testModePrintTimer) > TEST_MODE_PRINT_INTERVAL) {
    testModePrintTimer = millis(); // reset the timer
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

void rfCheck() {
  // Read from RF device
  if (getSerial1()) {
    handleReadBuffer();
  }
}

void gpsCheck() {
  relay.gps.readGPSSerial(); // we need to keep reading in main loop to keep GPS serial buffer clear
  if (nowTimeDiff(gpsTimer) > GPS_WAKEUP_INTERVAL) {
    relay.gps.gpsWakeup(); // wake up the GPS until we get a fix or timeout
    // TODO: double check this timeout logic
    if (relay.gps.gpsHasFix() || (nowTimeDiff(gpsTimer) > GPS_WAKEUP_INTERVAL+GPS_LOCK_TIMEOUT)) { 
      relay.gps.gpsStandby();
      gpsTimer = millis(); // reset the timer
      #if DEBUG_MODE
      Serial.print("DEBUG: ");if (relay.gps.gpsHasFix()) {Serial.println("GOT GPS FIX");} else {Serial.println("GPS FIX TIMEOUT");}
      #endif
    }
  }
}

void batteryCheck() {
  if (nowTimeDiff(batteryCheckTimer) > BATTERY_CHECK_INTERVAL) {
    batteryCheckTimer = millis(); // reset the timer
    relay.checkBatteryVoltage();
  }
}

void sleepCheck() {
  if (nowTimeDiff(awakeTimer) > AWAKE_INTERVAL) {
    // set pin mode to low
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("sleeping as timed out");
    USBDevice.detach();
    __WFI();  // wake from interrupt
    USBDevice.attach();
    delay(500);
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
    doc["uptime_ms"] = millis();
    doc["version"] = fwVersion;
    doc["lat"] = relay.gps.getLastFixLatitude();
    doc["lon"] = relay.gps.getLastFixLongitude();
    doc["bat"] = relay.getBatteryVoltage();
    digitalWrite(IRIDIUM_INTERFACE_WAKEUP_PIN, !digitalRead(IRIDIUM_INTERFACE_WAKEUP_PIN));
    delay(500);
    serializeJson(doc, IridiumInterfaceSerial);
    serializeJson(doc, Serial);
    Serial.println();
  }
  memset(readBuffer, 0, sizeof(readBuffer));
}

void ledBlinkCheck() {
  if (nowTimeDiff(ledBlinkTimer) > LED_BLINK_TIMER) {
    ledBlinkTimer = millis(); // reset the timer
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}
