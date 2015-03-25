#include <TinyWireM.h>
#include <TinyRTClib.h>
#include <Adafruit_NeoPixel.h>
#include <PinChangeInterrupt.h>

#define PIXEL_NUMBER            60 // Neopixel ring length
#define PIXELS_PIN               1 // Neopixel ring pin
#define PIXELS_MIN_BRIGHTNESS   16 // Minimum brightness
#define PIXELS_MAX_BRIGHTNESS  128 // Maximum brightness
#define LDR_PIN                  2 // Analog PIN number
#define LDR_MIN                 50 // Minimum brightness threshold
#define LDR_MAX               1000 // maximum brightness threshold
#define SQW_PIN                  3 // 1Hz square wave pin

RTC_DS1307 rtc;
Adafruit_NeoPixel ring = Adafruit_NeoPixel(PIXEL_NUMBER, PIXELS_PIN, NEO_GRB + NEO_KHZ800); // ring object

byte hours;
byte minutes;
byte seconds;

void setup() {
  ring.begin();
  TinyWireM.begin();
  rtc.begin();

  if (!rtc.isrunning()) {
      for(int i = 0; i < 15; i++ ) {
         ring.setPixelColor(0, 0xff0000);
         ring.show();
         delay(25000);
      }         
  }
  
  // Get the current time
  DateTime n = rtc.now();
  hours = n.hour();
  if (hours >= 12) {
    hours -= 12;
  }
  minutes = n.minute();
  seconds = n.second();
  
  attachPcInterrupt(SQW_PIN, secondPassed, FALLING);
}

void loop() {
  // put your main code here, to run repeatedly:
}

void secondPassed() {
  seconds += 1;
  if (seconds >= 60) {
    // Increment minutes
    seconds = 0;
    minutes += 1;
    if (minutes >= 60) {
      // Increment hours
      minutes = 0;
      hours += 1;
      if (hours >= 12) {
        hours = 0;
      }
    }
  }
}

void adjustBrightness() {
  int ldrValue = analogRead(LDR_PIN);
  // Map LDR reading range to neopixel brightness range
  ldrValue = map(ldrValue, LDR_MIN, LDR_MAX, PIXELS_MIN_BRIGHTNESS, PIXELS_MAX_BRIGHTNESS);
  // Constrain value to limits
  ldrValue = constrain(ldrValue, PIXELS_MIN_BRIGHTNESS, PIXELS_MAX_BRIGHTNESS);
  ring.setBrightness(ldrValue);
}
