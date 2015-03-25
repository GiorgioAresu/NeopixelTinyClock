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

// Raw time
byte seconds;
byte minutes;
byte hours;

// Time mapped to ring
byte prevSecondsPixel;
byte prevMinutesPixel;
byte prevHoursPixel;
byte secondsPixel;
byte minutesPixel;
byte hoursPixel;

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
  seconds = n.second();
  minutes = n.minute();
  hours = n.hour();
  if (hours >= 12) {
    hours -= 12;
  }
  
  attachPcInterrupt(SQW_PIN, secondPassed, FALLING);
}

void loop() {
  ring.setPixelColor(prevSecondsPixel, 0x000000);
  ring.setPixelColor(prevMinutesPixel, 0x000000);
  ring.setPixelColor(prevHoursPixel, 0x000000);
  ring.setPixelColor(secondsPixel, 0x000010);
  ring.setPixelColor(minutesPixel, 0x001000);
  ring.setPixelColor(hoursPixel, 0x100000);
  ring.show();
}

void secondPassed() {
  prevSecondsPixel = secondsPixel;
  prevMinutesPixel = minutesPixel;
  prevHoursPixel = hoursPixel;
  
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
  
  // Compute mappings from time to leds
  if (PIXEL_NUMBER != 60) {
    secondsPixel = map(seconds, 0, 60, 0, PIXEL_NUMBER-1);
    uint16_t minutesVal = ((uint16_t) minutes) * 60 + (uint16_t) seconds;
    minutesPixel = map(minutesVal, 0, 3599, 0, PIXEL_NUMBER-1);
  } else {
    secondsPixel = seconds;
    minutesPixel = minutes;
  }
  if (PIXEL_NUMBER != 12) {
    uint16_t hoursVal = ((uint16_t) hours) * 60 + (uint16_t) minutes;
    hoursPixel = map(hoursVal, 0, 719, 0, PIXEL_NUMBER-1);
  } else {
    hoursPixel = hours;
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
