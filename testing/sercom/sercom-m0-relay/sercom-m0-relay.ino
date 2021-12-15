#include <Arduino.h>   // required before wiring_private.h
#include "wiring_private.h" // pinPeripheral() function

#define RX_PIN 10
#define TX_PIN 11
#define RX_PAD SERCOM_RX_PAD_2
#define TX_PAD UART_TX_PAD_0

Uart Serial3 (&sercom3, RX_PIN, TX_PIN, RX_PAD, TX_PAD);
void SERCOM3_Handler() {
  Serial3.IrqHandler();
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial3.begin(115200);
  pinPeripheral(10, PIO_SERCOM_ALT);
  pinPeripheral(11, PIO_SERCOM_ALT);
}

void loop() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  Serial3.println("FROM:RELAY TO:MODEM");
  delay(1000);
}
