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
#define LDR_MAX               1000 // Maximum brightness threshold
#define SQW_PIN                  3 // 1Hz square wave pin
#define ANIM_DURATION         1000 // Duration in ms
#define SECS_COLOR        0x0000ff // Seconds indicator color
#define MINS_COLOR        0x00ff00 // Minutes indicator color
#define HOURS_COLOR       0xff0000 // Hours indicator color
#define WHITE_DOTS_COLOR  0x383838 // Background dots for low light environments. If too low it won't turn on.

RTC_DS1307 rtc;
Adafruit_NeoPixel ring = Adafruit_NeoPixel(PIXEL_NUMBER, PIXELS_PIN, NEO_GRB + NEO_KHZ800);

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

// Other global variables
byte brightness;           // Keep brightness level
bool skipFrame;            // Skip frame drawing when interrupt updates time while the loop is running
float animTime;            // [0.0, 1.0]
unsigned long baseTime;    // Time of the last time update that changed pixels

void setup() {
  // Initialize hardware
  ring.begin();
  TinyWireM.begin();
  rtc.begin();
  
  // Get the current time
  DateTime n = rtc.now();
  seconds = n.second();
  minutes = n.minute();
  hours = n.hour();
  if (hours >= 12) {
    hours -= 12;
  }
  
  // Attach interrupt to SQW pin to handle seconds passing
  attachPcInterrupt(SQW_PIN, secondPassed, FALLING);
}

void loop() {
  skipFrame = false;
  
  // Compute the animation frame
  uint16_t relTime = constrain((uint16_t) (millis() - baseTime), 0, ANIM_DURATION);
  byte animTimeRaw = map(relTime, 0, ANIM_DURATION, 0, 255);
  animTime = animTimeRaw / 255.0;
  
  adjustBrightness();
  
  // Set background color
  if (map(brightness, PIXELS_MIN_BRIGHTNESS, PIXELS_MAX_BRIGHTNESS, 0, 10)<1) {
    // Really dark environment (<10%)
    // If ring is 60 pixel long then color every hour as a real clock,
    // otherwise just 3, 6, 9, 12
    byte mod = (PIXEL_NUMBER == 60) ? 5 : PIXEL_NUMBER / 12;
    for (byte i=PIXEL_NUMBER; i; i--) {
      ring.setPixelColor(i-1, ((i-1) % mod == 0) ? WHITE_DOTS_COLOR : 0);
    }
  } else {
    for (byte i=PIXEL_NUMBER; i; i--) {
      ring.setPixelColor(i-1, 0);
    }
  }
  
  // Animate each "clock arm"
  animationStep(SECS_COLOR, prevSecondsPixel, secondsPixel);
  animationStep(MINS_COLOR, prevMinutesPixel, minutesPixel);
  animationStep(HOURS_COLOR, prevHoursPixel, hoursPixel);
  
  // Skip frame draw if the interrupt changed interested pixels,
  // otherwise the animation could glitch
  if (!skipFrame) {
    ring.show();
  }
}

void animationStep(uint32_t color, byte previousPixel, byte currentPixel) {
  if (currentPixel == previousPixel) {
    uint32_t currentPixelColor = color | ring.getPixelColor(currentPixel);
    ring.setPixelColor(currentPixel, currentPixelColor);
  } else {
    // Compute right base color given target color and animation time
    uint8_t r = color >> 16;
    uint8_t g = color >> 8;
    uint8_t b = color >> 0;
    // Compute respective colors and set them
    ring.setPixelColor(previousPixel, getRightColorForFrame(r ,g ,b, 1-animTime) | ring.getPixelColor(previousPixel));
    ring.setPixelColor(currentPixel, getRightColorForFrame(r ,g ,b, animTime) | ring.getPixelColor(currentPixel));
  }
}

uint32_t getRightColorForFrame(byte r, byte g, byte b, float animTime) {
  // Fade components appropriately and merge them
  byte r1 = r * animTime;
  byte g1 = g * animTime;
  byte b1 = b * animTime;
  return ((uint32_t)r1 << 16) | ((uint16_t)g1 << 8) | b1;
}

void secondPassed() {
  // Update values (keeping them because they need to be restored in some cases)
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
      
      // Modulo 12
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

static byte adjustBrightness() {
  unsigned int ldrValue = analogRead(LDR_PIN);
  // Map LDR reading range to neopixel brightness range
  ldrValue = map(ldrValue, LDR_MIN, LDR_MAX, PIXELS_MIN_BRIGHTNESS, PIXELS_MAX_BRIGHTNESS);
  // Constrain value to limits
  ldrValue = constrain(ldrValue, PIXELS_MIN_BRIGHTNESS, PIXELS_MAX_BRIGHTNESS);
  ring.setBrightness(ldrValue);
  brightness = ldrValue;
}
