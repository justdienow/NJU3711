# NJU3711 Library API Reference

Complete API documentation for the NJU3711 Arduino library.

## Table of Contents

- [NJU3711 Base Class](#nju3711-base-class)
- [NJU3711_7Segment Class](#nju3711_7segment-class)
- [NJU3711_7Segment_Multi Class](#nju3711_7segment_multi-class)
- [Constants and Enumerations](#constants-and-enumerations)

---

## NJU3711 Base Class

The base class for controlling the NJU3711 8-bit serial-to-parallel converter.

### Constructors

#### `NJU3711(uint8_t dataPin, uint8_t clockPin, uint8_t strobePin)`

Creates an instance with CLR pin hardware-strapped HIGH.

**Parameters:**
- `dataPin` - Arduino pin connected to NJU3711 DATA (pin 8)
- `clockPin` - Arduino pin connected to NJU3711 CLK (pin 9)
- `strobePin` - Arduino pin connected to NJU3711 STB (pin 10)

**Example:**
```cpp
NJU3711 expander(2, 3, 4); // DATA=2, CLK=3, STB=4
```

#### `NJU3711(uint8_t dataPin, uint8_t clockPin, uint8_t strobePin, uint8_t clearPin)`

Creates an instance with software-controlled CLR pin.

**Parameters:**
- `dataPin` - Arduino pin connected to NJU3711 DATA
- `clockPin` - Arduino pin connected to NJU3711 CLK
- `strobePin` - Arduino pin connected to NJU3711 STB
- `clearPin` - Arduino pin connected to NJU3711 CLR (pin 11)

**Example:**
```cpp
NJU3711 expander(2, 3, 4, 5); // DATA=2, CLK=3, STB=4, CLR=5
```

### Core Methods

#### `void begin()`

Initializes the NJU3711 device. Must be called in `setup()`.

**Example:**
```cpp
void setup() {
    expander.begin();
}
```

#### `void update()`

**CRITICAL**: Must be called regularly in `loop()` for non-blocking operation.

**Example:**
```cpp
void loop() {
    expander.update(); // Call this every loop iteration
}
```

#### `bool isBusy()`

Checks if the device is currently processing operations.

**Returns:** `true` if busy, `false` if ready for new operations

**Example:**
```cpp
if (!expander.isBusy()) {
    expander.write(0xFF);
}
```

### Output Control Methods

#### `bool write(uint8_t data)`

Writes 8-bit data to parallel outputs (non-blocking).

**Parameters:**
- `data` - 8-bit value to output (0x00 to 0xFF)

**Returns:** `true` if operation queued successfully, `false` if queue full

**Example:**
```cpp
expander.write(0b10101010); // Alternating pattern
expander.write(0xFF);        // All outputs HIGH
expander.write(0x00);        // All outputs LOW
```

#### `bool writeImmediate(uint8_t data)`

Alias for `write()` - queues data for immediate writing.

#### `bool clear()`

Clears all outputs to LOW (hardware or software clear).

**Returns:** `true` if operation queued successfully

**Example:**
```cpp
expander.clear(); // All outputs go LOW
```

### Bit Manipulation Methods

#### `bool setBit(uint8_t bitPosition)`

Sets a single bit HIGH (0-7).

**Parameters:**
- `bitPosition` - Bit to set (0 = P1, 7 = P8)

**Returns:** `true` if operation queued successfully, `false` if invalid position or queue full

**Example:**
```cpp
expander.setBit(0); // Set P1 HIGH
expander.setBit(7); // Set P8 HIGH
```

#### `bool clearBit(uint8_t bitPosition)`

Clears a single bit to LOW (0-7).

**Parameters:**
- `bitPosition` - Bit to clear (0-7)

**Returns:** `true` if operation queued successfully

**Example:**
```cpp
expander.clearBit(3); // Set P4 LOW
```

#### `bool toggleBit(uint8_t bitPosition)`

Toggles a single bit (0-7).

**Parameters:**
- `bitPosition` - Bit to toggle (0-7)

**Returns:** `true` if operation queued successfully

**Example:**
```cpp
expander.toggleBit(5); // Toggle P6 state
```

#### `bool writeBit(uint8_t bitPosition, bool value)`

Writes a specific bit value (0-7).

**Parameters:**
- `bitPosition` - Bit to write (0-7)
- `value` - `true` for HIGH, `false` for LOW

**Returns:** `true` if operation queued successfully

**Example:**
```cpp
expander.writeBit(2, true);  // Set P3 HIGH
expander.writeBit(4, false); // Set P5 LOW
```

### Advanced Methods

#### `bool shift(uint8_t data)`

Shifts data into the shift register without latching to outputs.

**Parameters:**
- `data` - 8-bit value to shift in

**Returns:** `true` if operation queued successfully

**Note:** Data is shifted but not yet visible on outputs. Call `latch()` to make it visible.

#### `bool latch()`

Latches the shift register contents to the parallel outputs.

**Returns:** `true` if operation queued successfully

**Example:**
```cpp
expander.shift(0xFF);  // Shift data in (not visible yet)
expander.latch();      // Make data visible on outputs
```

#### `uint8_t getCurrentData()`

Gets the current data value in the output latches.

**Returns:** Current 8-bit output value

**Example:**
```cpp
uint8_t current = expander.getCurrentData();
Serial.println(current, BIN); // Print as binary
```

### Timing Control

#### `void setStepDelay(unsigned long delayMicros)`

Sets the minimum delay between state machine steps.

**Parameters:**
- `delayMicros` - Delay in microseconds (default: 1µs)

**Example:**
```cpp
expander.setStepDelay(1);  // Fast (1µs) - default
expander.setStepDelay(10); // Slower (10µs)
```

**Note:** Default 1µs timing supports up to 5MHz operation. Increase if you experience timing issues.

### Test Pattern Methods

#### `bool startTestPattern(uint8_t patternType, unsigned long patternDelay)`

Starts an automated test pattern.

**Parameters:**
- `patternType` - Pattern to run (1-4)
  - `1` - All on/off alternating
  - `2` - Alternating bits (0xAA/0x55)
  - `3` - Walking bit (single bit rotating)
  - `4` - Binary counter (0-255)
- `patternDelay` - Delay between pattern steps in microseconds (default: 500000 = 500ms)

**Returns:** `true` if pattern started successfully

**Example:**
```cpp
expander.startTestPattern(3, 200000); // Walking bit, 200ms delay
```

#### `void stopTestPattern()`

Stops the currently running test pattern.

**Example:**
```cpp
expander.stopTestPattern();
```

### Queue Management

#### `uint8_t getQueueSize()`

Gets the current number of operations in the queue.

**Returns:** Number of queued operations (0-8)

**Example:**
```cpp
Serial.print("Queue size: ");
Serial.println(expander.getQueueSize());
```

#### `void clearQueue()`

Clears all queued operations.

**Example:**
```cpp
expander.clearQueue(); // Cancel all pending operations
```

---

## NJU3711_7Segment Class

Extends `NJU3711` with 7-segment display support.

### Pin Mapping

The library uses this specific pin mapping:

| Bit | Port | Segment |
|-----|------|---------|
| 7   | P8   | A (top) |
| 6   | P7   | F (top left) |
| 5   | P6   | B (top right) |
| 4   | P5   | G (middle) |
| 3   | P4   | E (bottom left) |
| 2   | P3   | C (bottom right) |
| 1   | P2   | D (bottom) |
| 0   | P1   | DP (decimal point) |

### Constructors

#### `NJU3711_7Segment(uint8_t dataPin, uint8_t clockPin, uint8_t strobePin, DisplayMode mode = ACTIVE_LOW)`

Creates a 7-segment display driver with CLR strapped HIGH.

**Parameters:**
- `dataPin` - DATA pin
- `clockPin` - CLK pin
- `strobePin` - STB pin
- `mode` - Display mode (default: `ACTIVE_LOW`)
  - `ACTIVE_LOW` - LEDs on when output LOW (common for most displays)
  - `ACTIVE_HIGH` - LEDs on when output HIGH

**Example:**
```cpp
NJU3711_7Segment display(2, 3, 4, ACTIVE_LOW);
```

#### `NJU3711_7Segment(uint8_t dataPin, uint8_t clockPin, uint8_t strobePin, uint8_t clearPin, DisplayMode mode = ACTIVE_LOW)`

Creates a 7-segment display driver with software CLR control.

### Display Methods

#### `bool displayDigit(uint8_t digit, bool showDP = false)`

Displays a digit (0-9).

**Parameters:**
- `digit` - Digit to display (0-9)
- `showDP` - Show decimal point (default: false)

**Returns:** `true` if operation queued successfully

**Example:**
```cpp
display.displayDigit(8);        // Show "8"
display.displayDigit(5, true);  // Show "5."
```

#### `bool displayHex(uint8_t hexValue, bool showDP = false)`

Displays a hexadecimal value (0-F).

**Parameters:**
- `hexValue` - Hex value to display (0-15)
- `showDP` - Show decimal point

**Returns:** `true` if operation queued successfully

**Example:**
```cpp
display.displayHex(0xA);       // Show "A"
display.displayHex(0xF, true); // Show "F."
```

#### `bool displayChar(char character, bool showDP = false)`

Displays a character (0-9, A-Z, -, _, space).

**Parameters:**
- `character` - Character to display
- `showDP` - Show decimal point

**Returns:** `true` if operation queued successfully

**Example:**
```cpp
display.displayChar('A');
display.displayChar('-');      // Minus sign
display.displayChar('_');      // Underscore
display.displayChar(' ');      // Blank
```

#### `bool displayRaw(uint8_t segmentMask, bool showDP = false)`

Displays raw segment data.

**Parameters:**
- `segmentMask` - 8-bit segment mask
- `showDP` - Show decimal point

**Returns:** `true` if operation queued successfully

**Example:**
```cpp
// Bit 7=A, 6=F, 5=B, 4=G, 3=E, 2=C, 1=D, 0=DP
display.displayRaw(0b11111100); // Display "A"
```

### Segment Control Methods

#### `bool setSegment(uint8_t segment, bool state)`

Controls an individual segment.

**Parameters:**
- `segment` - Segment to control (use `SEG_A` through `SEG_DP` constants)
- `state` - `true` for on, `false` for off

**Returns:** `true` if operation queued successfully

**Example:**
```cpp
display.setSegment(SEG_A, true);   // Turn on top segment
display.setSegment(SEG_G, true);   // Turn on middle segment
display.setSegment(SEG_DP, false); // Turn off decimal point
```

#### `bool clearSegment(uint8_t segment)`

Turns off an individual segment.

**Parameters:**
- `segment` - Segment to turn off

**Returns:** `true` if operation queued successfully

#### `bool toggleSegment(uint8_t segment)`

Toggles an individual segment.

**Parameters:**
- `segment` - Segment to toggle

**Returns:** `true` if operation queued successfully

### Decimal Point Control

#### `bool setDecimalPoint(bool state)`

Controls the decimal point.

**Parameters:**
- `state` - `true` for on, `false` for off

**Returns:** `true` if operation queued successfully

**Example:**
```cpp
display.setDecimalPoint(true);  // DP on
display.setDecimalPoint(false); // DP off
```

#### `bool toggleDecimalPoint()`

Toggles the decimal point.

**Returns:** `true` if operation queued successfully

#### `bool getDecimalPointState()`

Gets the current decimal point state.

**Returns:** `true` if DP is on, `false` if off

### Display Mode

#### `void setDisplayMode(DisplayMode mode)`

Sets the display mode.

**Parameters:**
- `mode` - `ACTIVE_LOW` or `ACTIVE_HIGH`

#### `DisplayMode getDisplayMode()`

Gets the current display mode.

**Returns:** Current display mode

### Special Display Functions

#### `bool displayBlank()`

Blanks the display (all segments off).

**Returns:** `true` if operation queued successfully

#### `bool displayAll()`

Turns on all segments (test pattern).

**Returns:** `true` if operation queued successfully

#### `bool displayMinus()`

Displays a minus sign "-".

**Returns:** `true` if operation queued successfully

#### `bool displayUnderscore()`

Displays an underscore "_".

**Returns:** `true` if operation queued successfully

#### `bool displayDegree()`

Displays a degree symbol "°".

**Returns:** `true` if operation queued successfully

#### `bool displayError()`

Displays "E" for error.

**Returns:** `true` if operation queued successfully

### Animation Methods

#### `bool startAnimation(AnimationType type, unsigned long animDelay)`

Starts a display animation.

**Parameters:**
- `type` - Animation type:
  - `ANIM_ROTATE_CW` - Clockwise segment rotation
  - `ANIM_ROTATE_CCW` - Counter-clockwise rotation
  - `ANIM_BLINK` - Blinking display
  - `ANIM_CHASE` - Chasing segments
  - `ANIM_LOADING` - Loading bar effect
- `animDelay` - Delay between animation steps (microseconds)

**Returns:** `true` if animation started successfully

**Example:**
```cpp
display.startAnimation(ANIM_ROTATE_CW, 200000); // 200ms rotation
display.startAnimation(ANIM_LOADING, 150000);   // 150ms loading bar
```

#### `void stopAnimation()`

Stops the current animation.

#### `bool isAnimating()`

Checks if an animation is running.

**Returns:** `true` if animating, `false` otherwise

---

## NJU3711_7Segment_Multi Class

Extends `NJU3711_7Segment` for multiplexed multi-digit displays (3 digits).

### Constructors

#### `NJU3711_7Segment_Multi(uint8_t dataPin, uint8_t clockPin, uint8_t strobePin, uint8_t digit1Pin, uint8_t digit2Pin, uint8_t digit3Pin, DisplayMode mode = ACTIVE_LOW)`

Creates a 3-digit multiplexed display driver.

**Parameters:**
- `dataPin`, `clockPin`, `strobePin` - NJU3711 control pins
- `digit1Pin` - Control pin for leftmost digit (hundreds)
- `digit2Pin` - Control pin for middle digit (tens)
- `digit3Pin` - Control pin for rightmost digit (ones)
- `mode` - Display mode

**Example:**
```cpp
// Digits: 5, 6, 7 (left to right)
NJU3711_7Segment_Multi display(2, 3, 4, 5, 6, 7, ACTIVE_LOW);
```

### Display Methods

#### `bool displayNumber(uint16_t number)`

Displays a number (0-999).

**Parameters:**
- `number` - Number to display (0-999)

**Returns:** `true` if successful

**Example:**
```cpp
display.displayNumber(123);  // Shows "123"
display.displayNumber(5);    // Shows "  5" (no leading zeros by default)
display.displayNumber(999);  // Shows "999"
```

#### `bool displayNumber(uint16_t number, uint8_t decimalPosition)`

Displays a number with decimal point.

**Parameters:**
- `number` - Number to display (0-999)
- `decimalPosition` - Position for decimal point (0=none, 1=leftmost, 2=middle, 3=rightmost)

**Returns:** `true` if successful

**Example:**
```cpp
display.displayNumber(123, 2); // Shows "12.3"
display.displayNumber(45, 3);  // Shows " 4.5"
```

### Digit Control Methods

#### `bool setDigit(uint8_t position, uint8_t value, bool showDP = false)`

Sets an individual digit position.

**Parameters:**
- `position` - Digit position (0=leftmost, 2=rightmost)
- `value` - Digit value (0-9)
- `showDP` - Show decimal point

**Returns:** `true` if successful

**Example:**
```cpp
display.setDigit(0, 1);        // Leftmost digit = "1"
display.setDigit(1, 2, true);  // Middle digit = "2."
display.setDigit(2, 3);        // Rightmost digit = "3"
// Result: "12.3"
```

#### `bool setDigitChar(uint8_t position, char character, bool showDP = false)`

Sets a digit position with a character.

**Parameters:**
- `position` - Digit position (0-2)
- `character` - Character to display
- `showDP` - Show decimal point

**Returns:** `true` if successful

**Example:**
```cpp
display.setDigitChar(0, 'E');
display.setDigitChar(1, 'r');
display.setDigitChar(2, 'r');
// Result: "Err"
```

#### `bool setDigitRaw(uint8_t position, uint8_t segments, bool showDP = false)`

Sets a digit position with raw segment data.

**Parameters:**
- `position` - Digit position (0-2)
- `segments` - Raw segment mask
- `showDP` - Show decimal point

**Returns:** `true` if successful

### Digit Enable/Disable

#### `bool enableDigit(uint8_t position, bool enable = true)`

Enables or disables a digit position.

**Parameters:**
- `position` - Digit position (0-2)
- `enable` - `true` to enable, `false` to disable

**Returns:** `true` if successful

#### `bool disableDigit(uint8_t position)`

Disables a digit position.

#### `void enableAllDigits()`

Enables all digit positions.

#### `void disableAllDigits()`

Disables all digit positions (blank display).

### Display Formatting

#### `void setLeadingZeros(bool enable)`

Enables or disables leading zeros.

**Parameters:**
- `enable` - `true` to show leading zeros, `false` to hide

**Example:**
```cpp
display.setLeadingZeros(true);
display.displayNumber(5);     // Shows "005"

display.setLeadingZeros(false);
display.displayNumber(5);     // Shows "  5"
```

#### `void setBlankOnZero(bool enable)`

Blanks the display when value is zero.

**Parameters:**
- `enable` - `true` to blank on zero

**Example:**
```cpp
display.setBlankOnZero(true);
display.displayNumber(0);     // Display is blank
```

### Display Control

#### `void clearDisplay()`

Clears and blanks the entire display.

#### `void displayAll()`

Test pattern - all segments on all digits.

### Multiplexing Control

#### `void setMultiplexDelay(unsigned long delayMicros)`

Sets the time each digit stays on.

**Parameters:**
- `delayMicros` - Delay in microseconds (default: 2000µs = 2ms)

**Example:**
```cpp
display.setMultiplexDelay(2000);  // 2ms per digit (default)
display.setMultiplexDelay(1000);  // 1ms per digit (faster)
display.setMultiplexDelay(3000);  // 3ms per digit (may flicker)
```

**Note:** 
- 3 digits × 2ms = 6ms per refresh = ~167Hz refresh rate
- Faster is brighter but uses more power
- Too slow causes visible flicker

#### `void setBlankingTime(unsigned long blankingMicros)`

Sets the blanking time between digit transitions.

**Parameters:**
- `blankingMicros` - Blanking time in microseconds (default: 50µs)

**Note:** Prevents ghosting between digits.

#### `void enableMultiplex(bool enable = true)`

Enables or disables multiplexing.

**Parameters:**
- `enable` - `true` to enable multiplexing

#### `void disableMultiplex()`

Disables multiplexing.

#### `bool isMultiplexing()`

Checks if multiplexing is enabled.

**Returns:** `true` if multiplexing is active

### Special Displays

#### `bool displayError()`

Displays "Err" on the display.

**Returns:** `true` if successful

#### `bool displayDashes()`

Displays "---" on the display.

**Returns:** `true` if successful

#### `bool displayTemperature(int16_t temp, bool celsius = true)`

Displays a temperature value (-99 to 999).

**Parameters:**
- `temp` - Temperature value
- `celsius` - Reserved for future use (degree symbol not implemented)

**Returns:** `true` if successful

**Example:**
```cpp
display.displayTemperature(72);   // Shows " 72"
display.displayTemperature(-15);  // Shows "-15"
```

### Decimal Point Control

#### `bool setDecimalPoint(uint8_t position, bool state)`

Controls decimal point for a specific digit.

**Parameters:**
- `position` - Digit position (0-2)
- `state` - `true` for on, `false` for off

**Returns:** `true` if successful

#### `void clearAllDecimalPoints()`

Turns off all decimal points.

### Utility Methods

#### `uint16_t getCurrentValue()`

Gets the currently displayed numeric value.

**Returns:** Current display value (0-999)

#### `void selectDigit(uint8_t digit)`

**Advanced:** Manually selects a digit (for testing).

**Parameters:**
- `digit` - Digit to select (0-2)

#### `void deselectAllDigits()`

**Advanced:** Turns off all digits (for testing).

---

## Constants and Enumerations

### Segment Constants

```cpp
#define SEG_A   7  // Top segment
#define SEG_B   5  // Top right
#define SEG_C   2  // Bottom right
#define SEG_D   1  // Bottom
#define SEG_E   3  // Bottom left
#define SEG_F   6  // Top left
#define SEG_G   4  // Middle
#define SEG_DP  0  // Decimal point
```

### Display Modes

```cpp
enum DisplayMode {
    ACTIVE_LOW,   // LEDs on when output LOW (most common)
    ACTIVE_HIGH   // LEDs on when output HIGH
};
```

### Animation Types

```cpp
enum AnimationType {
    ANIM_ROTATE_CW,   // Clockwise rotation
    ANIM_ROTATE_CCW,  // Counter-clockwise rotation
    ANIM_BLINK,       // Blinking digit
    ANIM_FADE,        // Fade in/out (reserved)
    ANIM_CHASE,       // Chase segments
    ANIM_LOADING      // Loading bar effect
};
```

---

## Usage Notes

### Non-Blocking Operation

All write operations are **non-blocking** and queue-based:

```cpp
void loop() {
    display.update();  // MUST call every loop
    
    // Your code here - won't block
    if (!display.isBusy()) {
        display.displayDigit(counter++);
    }
}
```

### Timing Considerations

- Default step delay: 1µs (supports 5MHz operation)
- Increase if experiencing timing issues
- Multiplexing: 2ms/digit default (adjust for brightness)

### Current Limiting

- NJU3711 outputs can sink 25mA per pin
- **Always use current-limiting resistors** with LEDs
- Recommended: 220-470Ω for 7-segment displays
- Calculate: R = (Vsupply - Vf) / Idesired

### Memory Usage

- Queue size: 8 operations
- Check `getQueueSize()` if queuing many operations
- Call `clearQueue()` to cancel pending operations

---

## See Also

- [Hardware Guide](Hardware_Guide.md) - Wiring and connections
- [Getting Started](Getting_Started.md) - First project tutorial
- [Advanced Usage](Advanced_Usage.md) - Complex applications
- [Troubleshooting](Troubleshooting.md) - Common issues