NeopixelTinyClock
===============

A configurable LED clock that uses an ATtiny85, an Adafruit Neopixel ring or strip, an RTC clock with an SQW pin set to 1Hz (I used a DS3231 board) and an LDR.

The main difference between this code and the others I found is that this use interrupts to stay in sync with the time while providing a nice and smooth animation, as opposed to poll the RTC continuously.
I tried to be as efficient as possible in terms of program footprint (but I'm sure it could be done better) to leave space for a bit of tinkering.
I'd like to switch between a couple of display modes using EEPROM and a fast power on-off-on cycle since we are using all IO pins (except for the RST, but that would require an HVSP programmer), but we're already hitting the limits of this tiny little controller's memory.

You need to have these libraries installed:
- https://github.com/adafruit/Adafruit_NeoPixel
- https://github.com/adafruit/TinyWireM
- https://github.com/adafruit/TinyRTCLib
- http://code.google.com/p/arduino-tiny/
