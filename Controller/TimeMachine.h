#ifndef __TIME_MACHINE_H
#define __TIME_MACHINE_H

#include "spec.h"

class TimeMachine {
public:
  TimeMachine(const int_fast16_t t1, const int_fast16_t t2, const int_fast16_t t3, const int_fast16_t t4) :
    _time { t1, t2, t3, t4 } {}
  int_fast16_t inc(int_fast8_t idx, int_fast16_t increment);
  int_fast16_t get(int_fast8_t idx)
    { assert(idx < 4); return _time[idx]; }

protected:
  int_fast16_t _time[4];
};

#endif // __TIME_MACHINE_H