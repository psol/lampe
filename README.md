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

The fonts come from the GLCD library (again, like Adafruit). Indeed you can trace some of
the logic back to those libraries.

PrestoST7735 is ***not** a library however, it's a custom implementation of ST7735 to
support this project. For example, the background colour is a constant, it is not possible
to draw lines or pixels because I don't need those. Note that it would be easy to add
those features but they would consome program space.

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

## Symbols
This is a compiler for symbol descriptor, given a set bitmap images (ideally monochrome),
it compiles a byte that describe the symbols. This is used to generate the symbols.h
file in the Controller project.

### spec.h
I find assertions are more effective than unit tests to verify format specifications
for most projects. Some experience with the Eiffel language has re-enforced that bias.

I did not write much formal specifications in this project but I wanted, at least,
to use the C++ `assert()` macro. `spec.h` exposes it over the serial port, to enable
testing from the Arduino IDE. It also declares a macro to log to the same serial port.

If you uncomment the line that defines NDEBUG, the assertions and logging are removed
from the project. I have not found how to define NDEBUG through the Arduino IDE.