#include <Arduino.h>
#include "TimeMachine.h"
#include "RotaryEncoder.h"

#define EOD  0x5A0
#define LESS CW

int_fast16_t TimeMachine::inc(int_fast8_t idx, int_fast16_t increment) {
  assert(idx < 4);

  _time[idx] += increment;
  switch(idx) {
    case 0:
      if(_time[0] >= _time[1])
        _time[0] = 0;
      else if(_time[0] < 0)
        _time[0] = _time[1] - LESS;
      break;
    
    case 1:
      if(_time[0] >= _time[1])
        _time[1] = _time[2];
      else if(_time[1] >= _time[2]) {
        if(_time[1] >= _time[3])
          _time[1] = _time[0] + LESS;
        else
          _time[2] = _time[1];
      }
      break;
      
    case 2:
      if(_time[2] >= _time[3])
        _time[2] = _time[1];
      else if(_time[1] >= _time[2]) {
        if(_time[2] <= _time[0])
          _time[2] = _time[3] - LESS;
        else
          _time[1] = _time[2];
      }
      break;
    
    case 3:
      if(_time[2] >= _time[3])
        _time[3] = EOD - CW;
      else if(_time[3] >= EOD)
        _time[3] = _time[2] + LESS;
      break;
  }
  return _time[idx];
}