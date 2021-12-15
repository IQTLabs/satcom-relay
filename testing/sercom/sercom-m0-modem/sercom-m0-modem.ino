#include <Arduino.h>   // required before wiring_private.h
#include "wiring_private.h" // pinPeripheral() function

#define RX_PIN 11
#define TX_PIN 10
#define RX_PAD SERCOM_RX_PAD_0
#define TX_PAD UART_TX_PAD_2

Uart Serial3 (&sercom3, RX_PIN, TX_PIN, RX_PAD, TX_PAD);
void SERCOM3_Handler() {
  Serial3.IrqHandler();
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial3.begin(115200);
  pinPeripheral(10, PIO_SERCOM_ALT);
  pinPeripheral(11, PIO_SERCOM_ALT);
}

void loop() {
  if (Serial3.available()) {
    Serial.print((char)Serial3.read());
  }
}
