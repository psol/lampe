/*
 * Demonstrates the use of ST7735 for the control panel of Lampe.
 * It shows the 4 times (2 intervals), one of them is controlled by a rotary encoder.
 * It simulates showing the "current time" with a blinking ':', the light being turned on
 * and off as well as reporting on I2C exchanges.
 */

#include <SPI.h>
#include <EEPROM.h>
#include "spec.h"
#include "PrestoST7735.h"
#include "RotaryEncoder.h"
#include "TimeMachine.h"

#define MODE_PIN    12
#define ADDRESS_PTR 0
#define REMOTE_PTR  ADDRESS_PTR + 1
#define ZONE_PTR    ADDRESS_PTR + 2
#define SYSTEM_MODE LOW
#define LIGHT_MODE  HIGH

PrestoST7735& st7735 = PrestoST7735::Instance();
RotaryEncoder encoders[4] = {
  RotaryEncoder( 8,  7,  6),
  RotaryEncoder( 3,  4,  5),
  RotaryEncoder(A0, A1, A2),
  RotaryEncoder(A4, A5, A3),
};
TimeMachine timeMachine(6 * 60, 8 * 60, 18 * 60, 23.5 * 60);
uint_fast8_t address;
uint_fast8_t remote = 0;
union {
   int8_t i8;
   uint8_t ui8;
} zone;

inline uint_fast8_t constrainAddress(uint_fast8_t address) {
  // EEPROM initialises to 0xFF, defaults to the address of the LED controller
  if(address == 0xFF) address = 0x42;
  return constrain(address, 0x40, 0x49);
}

inline int8_t constrainZone(int8_t zone) {
  return constrain(zone, -11, +11);
}

void writeTime(uint_fast16_t time) {
  static PrestoText& numbers = st7735.text(proportional15x21);
  static char buffer[13];

  sprintf(buffer,"%02d:%02d", time / 60, time % 60);
  numbers.write(buffer);
}

void updateSystem(bool write = true) {
  if(write) {
    EEPROM.update(ADDRESS_PTR, address);
    EEPROM.update(REMOTE_PTR, remote);
    EEPROM.update(ZONE_PTR, zone.ui8);
  }
  address = constrainAddress(EEPROM.read(ADDRESS_PTR));
  remote = constrain(EEPROM.read(REMOTE_PTR), 0, 1);
  zone.ui8 = constrainZone(EEPROM.read(ZONE_PTR));
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
  static PrestoText& mono    = st7735.text(monospace5x7);
  static PrestoText& symbols = st7735.text(symbols25x16);
  static char buffer[5];
  // the font definition packs the characters… '<' is erase
  static const char *timeFormat = "%d";

  REValue r = encoders[0].read();
  // due to font limitation, we cannot display a hexadecimal number that needs a letter
  // the range is large enough to avoid conflicts, in practice; not worth a large font set
  address = address + increment(r);
  if(address > 0x49) {
    remote = !remote;
    address = 0x40;
  }
  else if(address < 0x40) {
    remote = !remote;
    address = 0x49;
  }
  address = constrainAddress(address);
  redraw = redraw || r != STILL;

  r = encoders[1].read();
  zone.i8 = constrainZone(zone.i8 + increment(r));
  redraw = redraw || r != STILL;

  if(redraw) {
    mono.xy(3, 22);
    mono.write(F("Canal I2C et fonction :"));
    numbers.xy(3, 32);
    sprintf(buffer, "%#2x", address);
    // the font definition packs the characters…
    // replaces 'x' with the packed glyph
    buffer[1] = X_PROPORTIONAL15X21;
    numbers.write(buffer);
    symbols.xy(73, 36);
    symbols.foreground(fg_colour);
    symbols.write(remote ? F("2") : F("0"), F("0"));
    mono.xy(99, 45);
    mono.write(remote ? F("distant") : F("LED"), F("distant"));

    mono.xy(3, 60);
    mono.write(F("GMT +/- :"));
    numbers.xy(3, 70);
    sprintf(buffer, timeFormat, zone.i8);
    numbers.write(buffer, F("-00"));
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
  updateSystem(false);
}

void loop() {
  static unsigned long second = millis() + 1000;
  static unsigned long fail   = millis() + random(6000, 20000);
  static unsigned long light  = 0;
  static bool semicolon = true;
  static bool transmit = true;
  static bool fluo = true;
  static PrestoText& mono    = st7735.text(monospace5x7);
  static PrestoText& symbols = st7735.text(symbols25x16);
  static int prev_mode = -1;
  int mode = digitalRead(MODE_PIN);
  if(mode != prev_mode) {
    st7735.erase();
    updateSystem(prev_mode == SYSTEM_MODE);
  }
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
    if(mode == SYSTEM_MODE)
      updateSystem();

    mono.xy(128, 5);
    mono.write(semicolon ? F("10:21") : F("10 21"));
    symbols.foreground(fg_colour);
    symbols.foreground(semicolon || !transmit ? fg_colour : rgb565(50, 75, 75));
    symbols.xy(140, 107);
    symbols.write(transmit ? F("2") : F("3"));
    semicolon = !semicolon;
    second = millis() + 1000;
  }
  if(mode == SYSTEM_MODE)
    systemScreenLoop(mode != prev_mode);
  else
    lightScreenLoop(mode != prev_mode);
  prev_mode = mode;
}
