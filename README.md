# lampe
A more intuitive (or so I hope) ambiant light. Arduino project.

The light will turn itself on and off during two time intervals in the morning
and in the evening. Controls is through 4 rotary encoders which should provide a very
intuitive interface: to change a time, just turn the corresponding encoder.
No mode to remember, no complex configuration. I have plan to use a GPS receiver such
that I don't need to set the clock ever.

## Controller
This is the controller panel. It will communicate with the light through I2C.

### PrestoST7735
I have had some going back and forth on the display. Initially I wanted to use 1.8 TFT
display but I was disappointed byt the performance of the Adafruit GFX library.
Specifically I found that it was slow and included many features that I did not need.
I toyed with the idea of using 4x20 LCD displays instead but they did not looked right.

During my research, I stumbled into uTFT from cpldcpu (inspired by Tobias Weis who was
himself inspired by Adafruit!) and I realised that it would be easy to create a custom
implementation that focused on my needs.

I took the challenge as an opportunity to learn how SPI screens are driven (and on SPI
itself). It turned out easier and less intimidating than I initially thought it would be.

The fonts come from the GLCD library (again, like Adafruit). Indeed you can trace some of
the logic back to those libraries.

PrestoST7735 is **not** a library however, it's a custom implementation of ST7735 to
support this project. For example, the background colour is a constant, it is not possible
to draw lines or pixels because I don't need those. Note that it would be easy to add
those features but they would consume program space.

I use the Arduino SPI library which has very good performance if you use
`SPI.transfer(buffer,len)`, at least on the AVR processors. The `SPI.transfer(byte)` and
`SPI.transfer16(word)` are slower.

### Wiring
Although I target an Arduino Nano Every, I used a regular Arduino Nano for development.
The display is marked "1.8 TFT SPI 128*160 V1.1," which I bought from AZ Delivery.

| Arduino Nano | TFT 1.8 inches | Note         |
|-------------:|:---------------|:-------------|
| 3.3V         | LED            |              |
| 13           | SCK            | SCLK         |
| 11           | SDA            | MOSI         |
| 9            | A0             | Command/data |
| 5V           | RESET          | Never reset  |
| 10           | CS             | SS           |
| GND          | GND            |              |
| 5V           | VCC            |              |

Command/data is not part of the SPI protocol. When set HIGH, the ST7735 interprets
data on the wire as RGB for its screen buffer. When set LOW, the ST7735 interprets
it as a command. See `dataMode()`and `transferCommand()` in `PrestoST7735.cpp` for
code samples.

### spec.h
I find assertions are more effective than unit tests to verify format specifications
for most projects. Some experience with the Eiffel language has re-enforced that bias.

I did not write much formal specifications in this project but I wanted, at least,
to use the C++ `assert()` macro. `spec.h` exposes it over the serial port, to enable
testing from the Arduino IDE. It also declares a macro to log to the same serial port.

If you uncomment the line that defines NDEBUG, the assertions and logging are removed
from the project. I have not found how to define NDEBUG through the Arduino IDE.

### RotaryEncoder

The whole point of this project is to drive the led strip through 4 rotary encoders,
settings two time intervals by direct manipulation.
The class is a very simple wrapper for the rotary encoders, it includes simple debouncing.

### Light and System loops

There are two different loops (controlled by pin 12, connect to an SPST switch).
The primary loop is the light loop and it controls the 2 periods on which the LED
are turned on.

The secondary loop is the system loop which is a configuration screen for the I2C
"channel" or address. The role that this controller is in (controlling the LED or
a remote controller), the time zone (GMT +/- hours) and ultimately the quality
of the light (intensity and colour).

## Symbols
This is a compiler for symbol descriptor, given a set bitmap images (ideally monochrome),
it compiles a byte that describe the symbols. This is used to generate the symbols.h
file in the Controller project.
