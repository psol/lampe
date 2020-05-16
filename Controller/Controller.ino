/*
 * Demonstrates the use of ST7735 for the control panel of Lampe.
 * It shows the 4 times (2 intervals), one of them is controlled by a rotary encoder.
 * It simulates showing the "current time" with a blinking ':', the light being turned on
 * and off as well as reporting on I2C exchanges.
 */

#include <SPI.h>
#include "PrestoST7735.h"
#include "RotaryEncoder.h"
#include "spec.h"

PrestoST7735& st7735 = PrestoST7735::Instance();
RotaryEncoder encoders[4] = {
  RotaryEncoder(8, 7, 6),
  RotaryEncoder(0, 0, 0),
  RotaryEncoder(0, 0, 0),
  RotaryEncoder(0, 0, 0),
};

void writeTime(uint_fast16_t time) {
  static PrestoText& numbers = st7735.text(proportional15x21);
  static char buffer[13];

  sprintf(buffer,"%02d:%02d", time / 60, time % 60);
  numbers.write(buffer);
}

void setup() {
#ifndef NDEBUG
  Serial.begin(9600);
#endif
  SPI.begin();
  for(int i = 0;i < sizeof(encoders) / sizeof(encoders[0]) ;i++)
    encoders[i].begin();
  st7735.begin();
  st7735.erase();
  PrestoText& numbers = st7735.text(proportional15x21);
  PrestoText& symbols = st7735.text(symbols25x16);
  numbers.xy(3, 70);
  numbers.write(F("20:00"));
  numbers.moveX(21);
  numbers.write(F("23:30"));
  numbers.xy(3, 32);
  numbers.write(F("06:00"));
  numbers.moveX(21);
  numbers.write(F("08:00"));
  symbols.xy(3, 107);
  symbols.write(F("1"));
}

void loop() {
  static unsigned long second = millis() + 1000;
  static unsigned long fail   = millis() + random(6000, 20000);
  static int time = 8 * 60;
  static bool semicolon = true;
  static bool transmit = true;
  static bool fluo = true;
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
  if(encoders[0].click()) {
    symbols.foreground(fg_colour);
    symbols.xy(3, 107);
    symbols.write(fluo ? F("0") : F("1"));
    fluo = !fluo;
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
  REValue r = encoders[0].read();
  if(r != STILL) {
    time = time + r;
    if(time > 20 * 60)
      time = 20 * 60;
    else if(time < 6 * 60)
      time = 6 * 60;
    numbers.xy(3, 32);
    numbers.write(F("06:00"));
    numbers.moveX(21);
    writeTime(time);
  }
}
