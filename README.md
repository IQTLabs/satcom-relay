# Satellite Communication Relay

A SATCOM platform for relaying messages in remote locations

## About

## Environment Setup

The M0 was chosen as the Relay MCU because of its [6 SERCOM interfaces](https://learn.adafruit.com/using-atsamd21-sercom-to-add-more-spi-i2c-serial-ports/creating-a-new-serial).

### Board

- Adafruit Feather M0 Basic [(diagram)](https://cdn-learn.adafruit.com/assets/assets/000/046/244/original/adafruit_products_Feather_M0_Basic_Proto_v2.2-1.png?1504885373)

### Arduino Libraries

- Adafruit GPS Library by Adafruit
- IridiumSBDi2c by SparkFun Electronics [(diagram)](https://docs.rockblock.rock7.com/docs/connectors)

### Wiring Diagram

![fritzing](fritzing/satcom-relay_bb.png)
