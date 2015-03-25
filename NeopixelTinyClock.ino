#include <TinyWireM.h>
#include <TinyRTClib.h>
#include <Adafruit_NeoPixel.h>

#define PIXEL_NUMBER            60 // Neopixel strip length
#define PIXELS_PIN               0 // Neopixel strip pin (GPIO #0 /D0)

RTC_DS1307 rtc;
Adafruit_NeoPixel ring = Adafruit_NeoPixel(PIXEL_NUMBER, PIXELS_PIN, NEO_GRB + NEO_KHZ800); // strip object

void setup() {
  ring.begin();
  TinyWireM.begin();
  rtc.begin();

  if (! rtc.isrunning()) {
      for( int i = 0; i < 15; i++ ) {
         strip.setPixelColor( 0, 0xff0000 );
         delay(25000);
      }         
  }
  
  // Get the current time
  DateTime n = rtc.now(); 
}

void loop() {
  // put your main code here, to run repeatedly:

}
