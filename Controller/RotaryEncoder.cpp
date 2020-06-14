/*
 * The debounce filter comes from:
 * https://www.best-microcontroller-projects.com/rotary-encoder.html
 */

#include <arduino.h>
#include "spec.h"
#include "RotaryEncoder.h"

void RotaryEncoder::begin() {
  pinMode(_clock, INPUT_PULLUP);
  pinMode(_data, INPUT_PULLUP);
  pinMode(_button, INPUT_PULLUP);
}

REValue RotaryEncoder::read() {
  _state = (_state << 1) | digitalRead(_clock) | 0xe000;

  if(_state == 0xf000) {
    _state = 0x0000;
    if(digitalRead(_button) == HIGH)
      return digitalRead(_data) ? CW : CCW;
    else
      return digitalRead(_data) ? FCW : FCCW;
  }

  return STILL;
}
