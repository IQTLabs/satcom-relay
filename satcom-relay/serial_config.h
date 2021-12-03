#define GPS_RX_PIN 5
#define GPS_TX_PIN 22
#define IRIDIUM_RX_PIN 11
#define IRIDIUM_TX_PIN 10

#define GPSSerial Serial2
#define IridiumSerial Serial3

Uart Serial2 (&sercom2, GPS_RX_PIN, GPS_TX_PIN, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM2_Handler()
{
  Serial2.IrqHandler();
}

Uart Serial3 (&sercom1, IRIDIUM_RX_PIN, IRIDIUM_TX_PIN, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler()
{
  Serial3.IrqHandler();
}
