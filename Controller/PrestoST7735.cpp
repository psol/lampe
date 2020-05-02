/*
 * It is easy to interface with the ST7735.
 * First it sends some initialisation voodoo, which I copied from the Adafruit
 * library.
 * To draw on the display, first the code sets a viewbox by sending the coordinates
 * through a command and then it sends the data in 16-bits RGB format.
 * Since it uses an 8 words buffer, to  the code always sets a 1*8 viewbox
 * Three important concepts:
 * - the command/data pin tells the ST7735 whether the data it receives through SPI is
 *   a command (e.g. to initialise or set the viewbox) or if it is RGB data
 * - the RGB data is packed from 3 bytes into a word so precision is lost. The macro
 *   rgb565 packs the colour by simply dropping the lower precision bits (preserving
 *   respectively 5, 6 and 5 bits).
 *   Note that the colour format is selected during the initialisation sequence,
 *   other options are 12 and 18 bits
 * - SPI.transfer(buffer, len) is faster than SPI.transfer(byte)
 *
 * The monochrome glyphs are defined in byte arrays. To draw a character, the code
 * accesses the definition from program memory, and interprets the bits as indication
 * to draw foreground or background colour.
 */

#include <arduino.h>
#include <SPI.h>
#include "PrestoST7735.h"
#include "ST7735_cmd.h"
#include "font5x7.h"
#include "font15x21.h"
#include "symbols.h"
#include "spec.h"

// global functions for low-level access to the display
// access is mitigated through the singleton and the text abstract class

#define MONO_WIDTH                5
#define MONO_HEIGHT               7
#define PROP_DEF_LEN ((15 * 3) + 1)
#define PROP_WIDTH               15
#define PROP_HEIGHT              21
#define BYTE                      8
#define WORD                     16
#define DC_PIN                    9   // data/command
#define CS_PIN                   10   // chip select

#define bg_colour rgb565(80, 120, 120)

uint16_t buffer[BYTE];  // shared buffer

inline void selectSlave() {
  digitalWrite(CS_PIN, LOW);
}

inline void dataMode() {
  digitalWrite(DC_PIN, HIGH);
}

inline void transferCommand(byte c) {
  digitalWrite(DC_PIN, LOW);
  SPI.transfer(c);
}

void viewbox(byte x0, byte y0, byte x1, byte y1) {
  assert(x0 < ST7735_WIDTH);
  assert(x1 < ST7735_WIDTH);
  assert(y0 < ST7735_HEIGHT);
  assert(y1 < ST7735_HEIGHT);

  transferCommand(ST7735_CASET); // Column addr set
  dataMode();
  buffer[0] = x0 << 8;
  buffer[1] = x1 << 8;
  SPI.transfer(&buffer,2 * sizeof(uint16_t));

  transferCommand(ST7735_RASET); // Row addr set
  dataMode();
  buffer[0] = y0 << 8;
  buffer[1] = y1 << 8;
  SPI.transfer(&buffer,2 * sizeof(uint16_t));

  transferCommand(ST7735_RAMWR); // write to RAM
}

// Rather than a bazillion writecommand() and writedata() calls, screen
// initialization commands and arguments are organized in these tables
// stored in PROGMEM.  The table may look bulky, but that's mostly the
// formatting -- storage-wise this is hundreds of bytes more compact
// than the equivalent code.  Companion function follows.
#define DELAY 0x80
const byte PROGMEM InitCmd[] = {            // Initialization commands for 7735B screens
    20,                       // 19 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, no args, w/delay
      50,                     //     50 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, no args, w/delay
      100,                    //     255 = 500 ms delay
    0x26 , 1,  			// 3: Set default gamma
      0x04,                     //     16-bit colour
    0xb1, 2,              	// 4: Frame Rate
      0x0b,
      0x14,
    0xc0, 2,                    // 5: VRH1[4:0] & VC[2:0]
      0x08,
      0x00,
    0xc1, 1,                    // 6: BT[2:0]
      0x05,
    0xc5, 2,                    // 7: VMH[6:0] & VML[6:0]
      0x41,
      0x30,
    0xc7, 1,                    // 8: LCD Driving control
      0xc1,
    0xEC, 1,                    // 9: Set pumping colour freq
      0x1b,
    0x3a , 1 + DELAY,  	        // 10: Set colour format
      0x55,                     //     16-bit colour
      100,
    0x2a, 4,                    // 11: Set Column Address
      0x00,
      0x00,
      0x00,
      0x7f,
    0x2b, 4,                    // 12: Set Page Address
      0x00,
      0x00,
      0x00,
      0x9f,
    0x36, 1,                    // 12+1: Set Scanning Direction
      0xc8,
    0xb7, 1,			// 14: Set Source Output Direciton
      0x00,
    0xf2, 1,			// 15: Enable Gamma bit
      0x00,
    0xe0, 15 + DELAY,		// 16: magic
      0x28, 0x24, 0x22, 0x31,
      0x2b, 0x0e, 0x53, 0xa5,
      0x42, 0x16, 0x18, 0x12,
      0x1a, 0x14, 0x03,
      50,
    0xe1, 15 + DELAY,		// 17: more magic
      0x17, 0x1b, 0x1d, 0x0e,
      0x14, 0x11, 0x2c, 0xa5,
      0x3d, 0x09, 0x27, 0x2d,
      0x25, 0x2b, 0x3c, 
      50, 
    ST7735_NORON  ,   DELAY,  // 17: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,   DELAY,  // 18: Main screen turn on, no args, w/delay
      255,                    //     255 = 500 ms delay
    ST7735_MADCTL , 1,        // 19: Rotate screen
      ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_RGB
};

// Companion code to the above tables.  Reads and issues
// a series of LCD commands stored in PROGMEM byte array.
void commandList(const byte *addr) {
  selectSlave();
  uint_fast8_t numCommands = pgm_read_byte(addr++);     // Number of commands to follow
  while(numCommands--) {                   // For each command...
    transferCommand(pgm_read_byte(addr++));   //   Read, issue command
    uint_fast8_t numArgs  = pgm_read_byte(addr++);     // Number of args to follow
    uint_fast16_t ms      = numArgs & DELAY;           // If hibit set, delay follows args
    numArgs &= ~DELAY;                     //   Mask out delay bit
    dataMode();
    while(numArgs--) {                   //   For each argument...
      SPI.transfer(pgm_read_byte(addr++));  //     Read, issue argument
    }

    if(ms) {
      ms = pgm_read_byte(addr++); // Read post-command delay time (ms)
      if(ms == 255) ms = 500;
      delay(ms);
    }
  }
}

void writeLine(byte line, byte x, byte y, byte heigth, uint16_t colour) {
  viewbox(x, y, x, y + heigth);
  dataMode();
  for(byte h = 0;h < heigth;) {
    buffer[h++] = line & 0x01 ? colour : bg_colour;
    line >>= 1;
  }
  SPI.transfer(buffer, heigth * sizeof(uint16_t));
}

// concrete descendants of the abstract ST7735Text class

class Monospace5x7: public PrestoText {
public:
  Monospace5x7() : PrestoText(0, 0, fg_colour) {};
protected:
  void draw(char c);
};

class Proportional15x21: public PrestoText {
public:
  Proportional15x21() : PrestoText(0, 0, fg_colour) {};
protected:
  void draw(char c);
};

class Symbols25x16: public PrestoText {
public:
  Symbols25x16() : PrestoText(0, 0, fg_colour) {};
protected:
  void draw(char c);
};

// implementations

static PrestoST7735& PrestoST7735::Instance() {
  static PrestoST7735 instance; // Guaranteed to be destroyed.
                                // Instantiated on first use.
  return instance;
}

PrestoText& PrestoST7735::text(font f) {
  static Monospace5x7 mono5x7;
  static Proportional15x21 prop15x21;
  static Symbols25x16 symbols25x16;

  if(f == font::monospace5x7)
    return mono5x7;
  else if(f == font::proportional15x21)
    return prop15x21;
  else if(f == font::symbols25x16)
    return symbols25x16;
  assert(false);
}

void PrestoST7735::erase() {
  selectSlave();
  viewbox(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1);
  dataMode();
  for(byte y = ST7735_HEIGHT;y > 0;y--) {
    for(byte x = ST7735_WIDTH;x > 0;x--) {
      buffer[0] = bg_colour;
      SPI.transfer(&buffer, 2);
    }
  }
}

void PrestoText::write(const char* msg) {
  char* current = msg;
  while(*current != '\0') {
    draw(*current++);
  }
}

void PrestoText::write(const __FlashStringHelper* msg) {
  PGM_P current = reinterpret_cast<PGM_P>(msg);
  unsigned char c = pgm_read_byte(current++);
  while(c != '\0') {
    draw(c);
    c = pgm_read_byte(current++);
  }
}

void Monospace5x7::draw(char c) {
  assert(_x + MONO_WIDTH < ST7735_WIDTH);
  assert(_y + MONO_HEIGHT < ST7735_HEIGHT);

  uint16_t glyph = Font5x7 + ((c - ' ') * MONO_WIDTH);
  selectSlave();
  // a line = the font height
  // we draw the character from left to right
  for(byte i = 0;i < MONO_WIDTH;i++)
    writeLine(pgm_read_byte(glyph++), _x++, _y, BYTE, _foreground);
  _x++; // spacing between glyph
}

void Proportional15x21::draw(char c) {
  assert(_y + PROP_HEIGHT < ST7735_HEIGHT);
  
  uint16_t glyph = Liberation_Sans15x21_Numbers + (((c - '.')) * PROP_DEF_LEN);
  selectSlave();
  // a line = the font height
  // we draw the glyph from left to right
  byte width = pgm_read_byte(glyph++);
  assert(_x + width < ST7735_WIDTH);
  for(byte i = 0;i < width;i++) {
    writeLine(pgm_read_byte(glyph++), _x,   _y,        BYTE, _foreground);
    writeLine(pgm_read_byte(glyph++), _x,   _y + BYTE, BYTE, _foreground);
    writeLine(pgm_read_byte(glyph++), _x++, _y + WORD, 5,    _foreground);
  }
  // digits are proportional but, in practice, of similar width
  // this ensures erasing the previous digit entirely
  if(width < PROP_WIDTH) {
    writeLine(0, _x, _y,        BYTE, _foreground);
    writeLine(0, _x, _y + BYTE, BYTE, _foreground);
    writeLine(0, _x, _y + WORD, 5,    _foreground);
  }
  _x++; // spacing between glyphs
}

void Symbols25x16::draw(char c) {
  assert(_y + SYMBOLS_HEIGHT < ST7735_HEIGHT);
  
  byte y0 = _y + BYTE;
  uint16_t glyph = symbols + (((c - '0')) * SYMBOLS_DEF_LEN);
  
  selectSlave();
  // a line = the font height
  // we draw the glyph from left to right
  byte width = pgm_read_byte(glyph++);
  assert(_x + width < ST7735_WIDTH);
  for(byte i = 0;i < width;i++) {
    writeLine(pgm_read_byte(glyph++), _x,   y0, BYTE, _foreground);
    writeLine(pgm_read_byte(glyph++), _x++, _y, BYTE, _foreground);
  }
  _x++; // spacing between glyphs
}

void PrestoST7735::begin() {
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  pinMode(DC_PIN, OUTPUT);
  pinMode(CS_PIN, OUTPUT);
  delay(100);
  commandList(InitCmd);
}