//BOARD: Feather Sense nRF52840 (https://www.adafruit.com/product/4516) (https://learn.adafruit.com/adafruit-feather-sense/arduino-support-setup)
//FeatherWing: Ultimate GPS (https://www.adafruit.com/product/3133) 
//FeatherWing: OLED (https://www.adafruit.com/product/3045)

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>
#include <Adafruit_GPS.h>

#define OLED true
#define USE_GPS true
#define GPSECHO false
#define LED 13

uint32_t timer = millis();
uint32_t trigger = 0;
uint32_t triggerdelay = 1000;
uint8_t len;

#ifdef USE_GPS
  #define GPSSerial Serial1
  Adafruit_GPS GPSModule(&GPSSerial);
#endif

#ifdef OLED
  Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);
  #define BUTTON_A  9
  #define BUTTON_B  6
  #define BUTTON_C  5
#endif


void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  delay(5000);

  Serial.begin(115200);
  Serial.println("Adafruit GPS library basic parsing test!");
  
  #ifdef USE_GPS
    GPSModule.begin(9600);
    GPSModule.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    GPSModule.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
    delay(1000);
  #endif
  
  #ifdef OLED
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
    display.display();
    pinMode(BUTTON_A, INPUT_PULLUP);
    pinMode(BUTTON_B, INPUT_PULLUP);
    pinMode(BUTTON_C, INPUT_PULLUP);
    char line[100];
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
  #endif
}


void loop() {
  char line[100];
  #ifdef USE_GPS
    char c = GPSModule.read();
    if (GPSECHO)
      if (c) Serial.print(c);
    if (GPSModule.newNMEAreceived()) {
      Serial.print(GPSModule.lastNMEA()); // this also sets the newNMEAreceived() flag to false
      if (!GPSModule.parse(GPSModule.lastNMEA())) // this also sets the newNMEAreceived() flag to false
        return; // we can fail to parse a sentence in which case we should just wait for another
    }
  #endif
//TODO: Convert to CURRENT - PREVIOUS.  millis wraps to zero after 49 days
  if (trigger < millis()){
    if(GPSModule.fix>0){
      trigger = millis() + triggerdelay;
      display.clearDisplay();
      display.setCursor(0,0);
      sprintf(line, "Lat: %f, Lon: %f\n",GPSModule.latitude,GPSModule.longitude);
      display.print(line);
      display.display();
    }
  }
}