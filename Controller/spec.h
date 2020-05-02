/*
 * Redirect C++ "assert.h" outout to the serial port (ie Arduino IDE).
 * Also offer logging that is disable through the same #define.
 * The sketch MUST:
 * - include this file
 * - implement the following in setup:
 *   #ifndef NDEBUG
 *     Serial.begin(9600);
 *   #endif
 */

#ifndef __SPEC_H
#define __SPEC_H

// comment out to disable assertions (saving program memory and SRAM)
// #define NDEBUG

#define __ASSERT_USE_STDERR
#include <assert.h>

#ifdef NDEBUG
void __assert (const char *func, const char *file, int line, const char *failedexpr);
#endif // NDEBUG

#ifdef NDEBUG
#define srlog(msg)   ((void)0)
#define srlogln(msg) ((void)0)
#else
#define srlog(msg)   Serial.print(msg)
#define srlogln(msg) Serial.println(msg)
#endif // NDEBUG

#endif // __SPEC_H
