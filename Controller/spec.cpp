#include <arduino.h>
#include "spec.h"

#ifndef NDEBUG
void __assert (const char *func, const char *file, int line, const char *failedexpr)
{
  Serial.print(F("assert in "));
  Serial.print(func);
  Serial.print(F("() #"));
  Serial.print(line);
  Serial.print(F(" - "));
  Serial.println(failedexpr);
  delay(1000);  // 1s to send over serial
  abort();
}
#endif