/*
 * Demonstrates the use of ST7735 for the control panel of Lampe.
 * It shows the 4 times (2 intervals) and simulates the rotation of an encoder.
 * It also shows the current time and simulates the light being turned on and off
 * as well as reporting I2C exchanges.
 */

#include <SPI.h>
#include "PrestoST7735.h"
#include "spec.h"

void setup() {
#ifndef NDEBUG
  Serial.begin(9600);
#endif
  SPI.begin();
  PrestoST7735& st7735 = PrestoST7735::Instance();
  st7735.begin();
  st7735.erase();
  PrestoText& numbers = st7735.text(proportional15x21);
  numbers.xy(3, 70);
  numbers.write(F("20:00"));
  numbers.moveX(21);
  numbers.write(F("23:30"));
}

void loop() {
  static unsigned long second = millis() + 1000;
  static unsigned long simule = 0;
  static unsigned long fail   = millis() + random(6000, 20000);
  static unsigned long light  = 0;
  static int h = 8, m = 0;
  static char buffer[13];
  static bool semicolon = true;
  static bool transmit = true;
  static bool fluo = true;
  static PrestoST7735& st7735 = PrestoST7735::Instance();
  static PrestoText& mono = st7735.text(monospace5x7);
  static PrestoText& numbers = st7735.text(proportional15x21);
  static PrestoText& symbols = st7735.text(symbols25x16);
  if(millis() > fail) {
    if(transmit)
      fail = millis() + random(2000, 4000);
    else
      fail = millis() + random(6000, 20000);
    transmit = !transmit;
  }
  if(millis() > light) {
    symbols.foreground(fg_colour);
    symbols.xy(3, 107);
    symbols.write(fluo ? F("0") : F("1"));
    fluo = !fluo;
    light = millis() + 10000;
  }
  if(millis() > second) {
    mono.xy(128, 5);
    mono.write(semicolon ? F("10:21") : F("10 21"));
    symbols.foreground(fg_colour);
    symbols.foreground(semicolon || !transmit ? fg_colour : rgb565(50, 75, 75));
    symbols.xy(140, 107);
    symbols.write(transmit ? F("2") : F("3"));
    semicolon = !semicolon;
    second = millis() + 1000;
  }
  if(millis() > simule) {
    m += 5;
    if(m > 55) {
      h++;
      m = 0;
    }
    if(h > 20) h = 15;
    numbers.xy(3, 32);
    numbers.write(F("06:00"));
    numbers.moveX(21);
    sprintf(buffer,"%02d:%02d", h, m);
    numbers.write(buffer);
    simule = millis() + random(500, 3500);
  }
}
