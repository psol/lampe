/*
 * Simple wrapper for a rotary encoder.
 * The return values are +/5 such that it can be directly added/removed to the minutes.
 */

#ifndef __ROTARY_ENCODER_H
#define __ROTARY_ENCODER_H

enum REValue {
  STILL,
  CW = 5,   // convenience: time evolve in 5 minutes increment
  CCW = -5
};

class RotaryEncoder {
public:
  RotaryEncoder(uint8_t clock, uint8_t data, uint8_t button) :
    _clock(clock), _data(data), _button(button), _state(0), _click(HIGH) {}
  void begin();
  REValue read();
  bool click();

protected:
  uint8_t _clock;
  uint8_t _data;
  uint8_t _button;
  uint_fast16_t _state;
  int _click;
};

#endif // __ROTARY_ENCODER_H