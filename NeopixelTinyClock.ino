#include <TinyWireM.h>
#include <TinyRTClib.h>
#include <Adafruit_NeoPixel.h>

#define PIXEL_NUMBER            60 // Neopixel ring length
#define PIXELS_PIN               0 // Neopixel ring pin (GPIO #0 /D0)
#define PIXELS_MIN_BRIGHTNESS   16 // Minimum brightness
#define PIXELS_MAX_BRIGHTNESS  128 // Maximum brightness
#define LDR_PIN                  2 // Analog PIN number
#define LDR_MIN                 50 // Minimum brightness threshold
#define LDR_MAX               1000 // maximum brightness threshold

RTC_DS1307 rtc;
Adafruit_NeoPixel ring = Adafruit_NeoPixel(PIXEL_NUMBER, PIXELS_PIN, NEO_GRB + NEO_KHZ800); // ring object

void setup() {
  ring.begin();
  TinyWireM.begin();
  rtc.begin();

  if (!rtc.isrunning()) {
      for(int i = 0; i < 15; i++ ) {
         ring.setPixelColor(0, 0xff0000);
         delay(25000);
      }         
  }
  
  // Get the current time
  DateTime n = rtc.now(); 
}

void loop() {
  // put your main code here, to run repeatedly:
}

void adjustBrightness() {
  int ldrValue = analogRead(LDR_PIN);
  // Map LDR reading range to neopixel brightness range
  ldrValue = map(ldrValue, LDR_MIN, LDR_MAX, PIXELS_MIN_BRIGHTNESS, PIXELS_MAX_BRIGHTNESS);
  // Constrain value to limits
  ldrValue = constrain(ldrValue, PIXELS_MIN_BRIGHTNESS, PIXELS_MAX_BRIGHTNESS);
  ring.setBrightness(ldrValue);
}
