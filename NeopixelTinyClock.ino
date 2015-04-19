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
#define SECS_SHADE_RADIUS        2 // Number of pixels to shade besides the center
#define MINS_COLOR        0x00ff00 // Minutes indicator color
#define MINS_SHADE_RADIUS        0 // Number of pixels to shade besides the center
#define HOURS_COLOR       0xff0000 // Hours indicator color
#define HOURS_SHADE_RADIUS       1 // Number of pixels to shade besides the center
#define WHITE_DOTS_COLOR  0x383838 // Background dots for low light environments. If too low it won't turn on.
#define AUTO_DST              true // Automatically adjust time to european DST

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
float prevAnimTime;
unsigned long baseTime;    // Time of the last time update that changed pixels
bool dst;                  // DST

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
  if (AUTO_DST) {
    if (dst = isDST()) {
      hours +=1;
    }
    if (hours >= 24) {
      hours -= 24;
    } else if (hours >= 12) {
      hours -= 12;
    }
  } else {
    if (hours >= 12) {
      hours -= 12;
    }
  }
  
  // Attach interrupt to SQW pin to handle seconds passing
  attachPcInterrupt(SQW_PIN, secondPassed, FALLING);
}

void loop() {
  skipFrame = false;
  
  // Compute the animation frame
  prevAnimTime = animTime;
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
  
  // Animate each clock arm
  animationStep(SECS_COLOR, prevSecondsPixel, secondsPixel, SECS_SHADE_RADIUS);
  animationStep(HOURS_COLOR, prevHoursPixel, hoursPixel, HOURS_SHADE_RADIUS);
  animationStep(MINS_COLOR, prevMinutesPixel, minutesPixel, MINS_SHADE_RADIUS);
  
  // Skip frame draw if the interrupt changed interested pixels,
  // otherwise the animation could glitch
  if (!skipFrame) {
    ring.show();
  }
}

void animationStep(uint32_t color, byte previousPixel, byte currentPixel, byte shadeRadius) {  
  uint8_t delta = (currentPixel==previousPixel) ? 0 : 1;
  
  for (int16_t i=shadeRadius; i+shadeRadius+delta >= 0; i--) {    
    // Compute shade for last target time and new target time.
    uint8_t absI = (i>=0) ? i : -i;
    uint8_t absIDelta = (i-delta>=0) ? i-delta : -(i-delta);
    // When target pixels change we fade the old target to the new one using animation time
    float x = shadingAmount(absIDelta, shadeRadius)*(1-animTime) + shadingAmount((i>=0) ? i : -i, shadeRadius)*animTime;
    
    int16_t pixel = currentPixel-i;
    // Ensure pixel stays within range (modulo operator)
    if (pixel >= PIXEL_NUMBER) {
      pixel -= PIXEL_NUMBER;
    } else if (pixel < 0) {
      pixel += PIXEL_NUMBER;
    }
    
    // Now we need to shiftout color components separately to fade them.
    // Separate color components
    uint8_t r = color >> 16;
    uint8_t g = color >> 8;
    uint8_t b = color >> 0;
    // Fade components appropriately
    uint8_t r1 = r * x;
    uint8_t g1 = g * x;
    uint8_t b1 = b * x;
    
    ring.setPixelColor(pixel, ((uint32_t)r1 << 16) | ((uint16_t)g1 << 8) | b1);
  }
}

float shadingAmount(byte distance, byte maxDistance) {
  float x = 1-distance/(maxDistance+1.0);
  if (x>1) {
    x = 1;
  } else if (x<0) {
    x = 0;
  }
  x=x*x*x;
  return x;
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
      
      if (AUTO_DST) {
        // Check for DST
        if (dst == isDST()) {
          hours += 1;
        } else {
          dst != dst;
          if (dst) {
            hours += 2;
          }
        }
      } else {
        hours += 1;
      }
      
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

bool isDST() {
  DateTime n = rtc.now();
  uint8_t dow = n.dayOfWeek();
  uint8_t day = n.day();
  uint8_t month = n.month();
  
  //January, February, November and December are out.
  if (month < 3 || month > 10) {
    return false;
  }
  //April to September are in
  if (month > 3 && month < 10) {
    return true;
  }
  
  int previousSunday = day - dow;
  //In March, we are DST if our previous Sunday was on or after the 25th.
  if (month == 3) {
    return previousSunday >= 25;
  }
  
  //In October we must be before the last Sunday to be DST.
  //That means the previous Sunday must be before the 25th.
  return previousSunday < 25;
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
