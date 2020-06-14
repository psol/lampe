/*
 * Minimalist implementation of an interface to ST7735 controller over SPI.
 * This is tailored to suit the needs of the Lampe project so it does not include
 * drawing lines, pixels or indeed changing the background colour programmatically.
 *
 * It uses SPI.h to communicate with the ST7735 which offers good performance if
 * you avoid SPI.transfer(byte) and SPI.transfer16(word).
 * SPI.transfer(buffer, len) is fast enough for the intended purpose.
 */

#ifndef __PRESTO_ST7735_H
#define __PRESTO_ST7735_H

// packs RGB colour to a 16-bits words
// swaps the 2 bytes such that it can be used with SPI.transfer(buffer, len)
#define rgb565(r,g,b) (uint16_t)(b & 0xF8) | ((g & 0xE0) >> 5) | ((g & 0x1C) << 11) | ((r & 0xF8) << 5)
#define fg_colour rgb565(10,  0, 30)

#define X_PROPORTIONAL15X21     ';'

enum font { monospace5x7, proportional15x21, symbols25x16 };

class PrestoText {
public:
  PrestoText(byte x, byte y, uint16_t foreground) : _x(x), _y(y), _foreground(foreground) {}
  void write(const char* msg, const __FlashStringHelper* eraser = nullptr);
  void write(const __FlashStringHelper* msg, const __FlashStringHelper* eraser = nullptr);
  void xy(byte x, byte y)
    { _x = x; _y = y; }
  void x(byte x)
    { _x = x; }
  void moveX(byte by)
    { _x += by; }
  void y(byte y)
    { _y = y; }
  void foreground(uint16_t colour)
    { _foreground = colour; }
  byte x()
    { return _x; }
  byte y()
    { return _y; }
  uint16_t foreground()
    { return _foreground; }

protected:
  byte _x, _y;
  uint16_t _foreground;
  unsigned width(const __FlashStringHelper* eraser);
  virtual void draw(char c) = 0;
  virtual void erase(byte toX) = 0;
  virtual byte width(char c) = 0;
};

class PrestoST7735
{
public:
  // Enforce a singleton to enable some optimisation (e.g. buffer).
  static PrestoST7735& Instance();
  // Don't forget to delete these two otherwise you may accidentally
  // get copies of your singleton appearing.
  PrestoST7735(PrestoST7735 const&)   = delete;
  void operator=(PrestoST7735 const&) = delete;

  // useful code
  void begin();
  PrestoText& text(font f);
  void erase();
private:
  PrestoST7735() {} // Constructor? (the {} brackets) are needed here.
};

#endif  // __PRESTO_ST7735_H