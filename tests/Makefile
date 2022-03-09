APP_NAME := test_timediff
ARDUINO_LIBS := AUnit
include ../EpoxyDuino/EpoxyDuino.mk

test_timediff.o: test_timediff.ino ../satcom-relay/timediff.h

test_timediff.out: test_timediff.o

runtests: test_timediff.out
	./test_timediff.out
