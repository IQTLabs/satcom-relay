APP_NAME := test_satcom_relay
ARDUINO_LIBS := AUnit ArduinoJson
include ../EpoxyDuino/EpoxyDuino.mk

test_satcom_relay.o: test_satcom_relay.ino ../satcom-relay/timediff.h ../satcom-relay/sensor-manager.h

test_satcom_relay.out: test_satcom_relay.o

runtests: test_satcom_relay.out
	./test_satcom_relay.out
