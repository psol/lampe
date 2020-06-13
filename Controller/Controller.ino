/*
 * Demonstrates the use of ST7735 for the control panel of Lampe.
 * It shows the 4 times (2 intervals), one of them is controlled by a rotary encoder.
 * It simulates showing the "current time" with a blinking ':', the light being turned on
 * and off as well as reporting on I2C exchanges.
 */

#include <SPI.h>
#include "PrestoST7735.h"
#include "RotaryEncoder.h"
#include "TimeMachine.h"
#include "spec.h"

#define MODE_PIN 12

PrestoST7735& st7735 = PrestoST7735::Instance();
RotaryEncoder encoders[4] = {
  RotaryEncoder( 8,  7,  6),
  RotaryEncoder( 3,  4,  5),
  RotaryEncoder(A0, A1, A2),
  RotaryEncoder(A4, A5, A3),
};
TimeMachine timeMachine(6 * 60, 8 * 60, 18 * 60, 23.5 * 60);
uint_fast8_t address = 0x42;
int_fast8_t zone = 2;

void writeTime(uint_fast16_t time) {
  static PrestoText& numbers = st7735.text(proportional15x21);
  static char buffer[13];

  sprintf(buffer,"%02d:%02d", time / 60, time % 60);
  numbers.write(buffer);
}

inline int_fast8_t increment(REValue value) {
  switch(value) {
    case CW:
    case FCW:
      return 1;
    case CCW:
    case FCCW:
      return -1;
    default:
      assert(false); // fall to STILL
    case STILL:
      return 0;
  }
}

void lightScreenLoop(bool redraw = false) {
  static PrestoText& numbers = st7735.text(proportional15x21);

  for(int i = 0;i < 4;i++) {
    REValue r = encoders[i].read();
    redraw = redraw || r != STILL;
    if(r != STILL)
      timeMachine.inc(i, r);
  }

  if(redraw) {
    numbers.xy(3, 32);
    writeTime(timeMachine.get(0));
    numbers.moveX(21);
    writeTime(timeMachine.get(1));
    numbers.xy(3, 70);
    writeTime(timeMachine.get(2));
    numbers.moveX(21);
    writeTime(timeMachine.get(3));
  }
}

void systemScreenLoop(bool redraw = false) {
  static PrestoText& numbers = st7735.text(proportional15x21);
  static PrestoText& mono = st7735.text(monospace5x7);
  static char buffer[5];
  // the font definition packs the characters… '<' is erase
  static const char *timeFormat = "%d<<";
  // assert that the erase character has not been re-assigned
  assert(timeFormat[2] == ERASE_PROPORTIONAL15X21);
  assert(timeFormat[3] == ERASE_PROPORTIONAL15X21);

  REValue r = encoders[0].read();
  // due to font limitation, we cannot display a hexadecimal number that needs a letter
  // the range is large enough to avoid conflicts, in practice; not worth a large font set
  address = constrain(address + increment(r), 0x40, 0x49);
  redraw = redraw || r != STILL;

  r = encoders[1].read();
  zone = constrain(zone + increment(r), -11, +11);
  redraw = redraw || r != STILL;

  if(redraw) {
    mono.xy(3, 22);
    mono.write(F("Adresse I2C:"));
    numbers.xy(3, 32);
    sprintf(buffer, "%#2x", address);
    buffer[1] = X_PROPORTIONAL15X21;  // the font definition packs the characters…
    numbers.write(buffer);

    mono.xy(3, 60);
    mono.write(F("GMT +/-:"));
    numbers.xy(3, 70);
    sprintf(buffer, timeFormat, zone);
    numbers.write(buffer);
  }
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
  pinMode(MODE_PIN, INPUT_PULLUP);
}

void loop() {
  static unsigned long second = millis() + 1000;
  static unsigned long fail   = millis() + random(6000, 20000);
  static unsigned long light  = 0;
  static bool semicolon = true;
  static bool transmit = true;
  static bool fluo = true;
  static PrestoText& mono = st7735.text(monospace5x7);
  static PrestoText& symbols = st7735.text(symbols25x16);
  static int mode = -1;
  int r = digitalRead(MODE_PIN);
  if(mode != r)
    st7735.erase();
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
  if(r == LOW)
    systemScreenLoop(mode != r);
  else
    lightScreenLoop(mode != r);
  mode = r;
}
