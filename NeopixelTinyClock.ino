#include <TinyWireM.h>
#include <TinyRTClib.h>
#include <Adafruit_NeoPixel.h>
#include <PinChangeInterrupt.h>

#define PIXEL_NUMBER            60 // Neopixel ring length
#define PIXELS_PIN               1 // Neopixel ring pin
#define PIXELS_MIN_BRIGHTNESS    4 // Minimum brightness
#define PIXELS_MAX_BRIGHTNESS  128 // Maximum brightness
#define LDR_PIN                  2 // Analog PIN number
#define LDR_MIN                 50 // Minimum brightness threshold
#define LDR_MAX               1000 // maximum brightness threshold
#define SQW_PIN                  3 // 1Hz square wave pin
#define ANIM_DURATION         1000 // Duration in ms

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

bool skipFrame;

unsigned long baseTime;

void setup() {
  ring.begin();
  TinyWireM.begin();
  rtc.begin();

  if (!rtc.isrunning()) {
         delay(25000);
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
  skipFrame = false;
  unsigned long relTime = constrain(millis() - baseTime, (unsigned long)0, (unsigned long)ANIM_DURATION);
  byte animTime = map(relTime, 0, ANIM_DURATION, 0, 255);
  
  adjustBrightness();
  
  uniformPixelsColor(0x000000);
  
  if (prevHoursPixel != hoursPixel) {
    ring.setPixelColor(prevHoursPixel, 1-animTime, 0, 0);
    ring.setPixelColor(hoursPixel, animTime, 0, 0);
  } else {
    ring.setPixelColor(hoursPixel, 255, 0, 0);
  }
  if (prevMinutesPixel != minutesPixel) {
    ring.setPixelColor(prevMinutesPixel, 0, 1-animTime, 0);
    ring.setPixelColor(minutesPixel, 0, animTime, 0);
  } else {
    ring.setPixelColor(minutesPixel, 0, 255, 0);
  }
  if (prevSecondsPixel != secondsPixel) {
    ring.setPixelColor(prevSecondsPixel, 0, 0, 1-animTime);
    ring.setPixelColor(secondsPixel, 0, 0, animTime);
  } else {
    ring.setPixelColor(secondsPixel, 0, 0, 255);
  }
  
  if (!skipFrame) {
    ring.show();
  }
}

void secondPassed() {
  byte backupPrevSecondsPixel = prevSecondsPixel;
  byte backupPrevMinutesPixel = prevMinutesPixel;
  byte backupPrevHoursPixel = prevHoursPixel;
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
  secondsPixel = (PIXEL_NUMBER == 60) ? seconds : map(seconds, 0, 59, 0, PIXEL_NUMBER-1);
  uint16_t minutesVal = ((uint16_t) minutes) * 60 + (uint16_t) seconds;
  minutesPixel = (PIXEL_NUMBER == 60) ? minutes : map(minutesVal, 0, 3599, 0, PIXEL_NUMBER-1);
  uint16_t hoursVal = ((uint16_t) hours) * 60 + (uint16_t) minutes;
  hoursPixel = (PIXEL_NUMBER == 12) ? hours : map(hoursVal, 0, 719, 0, PIXEL_NUMBER-1);
  
  // Compute baseTime for the animation
  if (prevSecondsPixel == secondsPixel) {
    prevSecondsPixel = backupPrevSecondsPixel;
  } else {
    baseTime = millis();
    skipFrame = true;
  }
  if (seconds == 0 && prevMinutesPixel == minutesPixel) {
    prevMinutesPixel = backupPrevMinutesPixel;
  }
  if (minutes == 0 && prevHoursPixel == hoursPixel) {
    prevHoursPixel = backupPrevHoursPixel;
  }
}

void uniformPixelsColor(uint32_t color) {
  for (uint16_t i=0; i<PIXEL_NUMBER; i++) {
    ring.setPixelColor(i, color);
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
