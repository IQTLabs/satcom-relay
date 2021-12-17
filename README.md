# Satellite Communication Relay

A SATCOM platform for relaying messages in remote locations

## About

## Environment Setup

The M0 was chosen as the Relay MCU because of its [6 SERCOM interfaces](https://learn.adafruit.com/using-atsamd21-sercom-to-add-more-spi-i2c-serial-ports/creating-a-new-serial).

### Board

- Adafruit Feather M0 Basic [(diagram)](https://cdn-learn.adafruit.com/assets/assets/000/046/244/original/adafruit_products_Feather_M0_Basic_Proto_v2.2-1.png?1504885373)

### Arduino Libraries

- Adafruit GPS Library by Adafruit
- IridiumSBDi2c by SparkFun Electronics [(datasheet)](https://docs.rockblock.rock7.com/docs/connectors)
- ArduinoJson by Benoit Blanchon

### Wiring Diagram

![fritzing](fritzing/satcom-relay_bb.png)

## Operations

The M0 uses Serial1 (RX0 and TX1) to listen for JSON messages from other devices. These messages then get additional keys added (`uptime_ms` and `version`) and then logged out the console.

The M0 will automatically go to sleep after `AWAKE_INTERVAL` (1 minute) if the interrupt pin (A1) hasn't been toggled. Every time the interrupt pin is toggled the `awakeTimer` is reset and the count towards `AWAKE_INTERVAL` starts over. When in sleep mode, toggling the interrupt pin will wake up the M0 again.
