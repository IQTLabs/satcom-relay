#include <Arduino.h>
#include "wiring_private.h"

#define RX_PIN 10
#define TX_PIN 11
#define RX_PAD SERCOM_RX_PAD_2
#define TX_PAD UART_TX_PAD_0

Uart IridiumControllerSerial (&sercom3, RX_PIN, TX_PIN, RX_PAD, TX_PAD);
void SERCOM3_Handler()
{
  IridiumControllerSerial.IrqHandler();
}

uint32_t messageTimer, blinkTimer = millis();

void setup() {
  Serial.begin(115200);
  IridiumControllerSerial.begin(115200);

  // Assign pins 10 & 11 SERCOM functionality
  pinPeripheral(RX_PIN, PIO_SERCOM_ALT);
  pinPeripheral(TX_PIN, PIO_SERCOM_ALT);

  pinMode(19, OUTPUT); //A5
  pinMode(LED_BUILTIN, OUTPUT);
}

void sendMessage() {
  Serial.print("Sending message...");
  IridiumControllerSerial.println("Test message");
  Serial.println("done");
}

void loop() {
  if (millis() - messageTimer > 10000) {
    messageTimer = millis();
    digitalWrite(19, !digitalRead(19));
    delay(100);
    sendMessage();
  }

  if (IridiumControllerSerial.available() > 0) {
    if (IridiumControllerSerial.read() == '\n') {
      Serial.println("Received newline / resend request");
      sendMessage();
    }
  }

  if (millis() - blinkTimer > 500) {
    blinkTimer = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}
