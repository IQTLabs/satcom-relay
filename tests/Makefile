APP_NAME := test_satcom_relay
ARDUINO_LIBS := AUnit ArduinoJson Adafruit_GPS_Library
include ../EpoxyDuino/EpoxyDuino.mk
EXTRA_CXXFLAGS=-include workarounds.h -include wiring_private.h -I.

test_satcom_relay.o: test_satcom_relay.ino ../satcom-relay/timediff.h ../satcom-relay/sensor-manager.h ../satcom-relay/gps.cpp ../satcom-relay/gps.h

test_satcom_relay.out: test_satcom_relay.o

runtests: test_satcom_relay.out
	./test_satcom_relay.out
