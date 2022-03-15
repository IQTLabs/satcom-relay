#include <Arduino.h>
#include <cstdio>

// TODO: Adafruit GPS library uses min() with different types in parseStr()
inline long int min(long int x, int y) { return min(x, (long int)y); }

// TODO: move to library.
class Uart {
  public:
    Uart() { clear(); };
    void begin(int _baud) { (void)_baud; };
    bool available() { return p < l; }
    char read() { return buffer[p++]; }
    void setbuf(const char *test_str) { clear(); buffer = test_str; l = strlen(test_str); }
    void clear() { p = 0; l = 0; w = 0; buffer = NULL; }
    int write(const uint8_t &s) { writeBuffer[w++] = s; return 1; }
    int write(const uint8_t *s, int c) { for (byte i = 0; i < c; ++i) { write(s[i]); } return c; }
    void println() { write('\n'); }
    byte p;
    byte l;
    byte w;
    const char *buffer;
    char writeBuffer[255] = {0};
};

typedef Uart HardwareSerial;
