# Getting Started with NJU3711

A beginner-friendly tutorial to get your first NJU3711 project working.

## Table of Contents

- [What You'll Need](#what-youll-need)
- [Installation](#installation)
- [Project 1: Blinking LED](#project-1-blinking-led)
- [Project 2: LED Bar Graph](#project-2-led-bar-graph)
- [Project 3: Single 7-Segment Display](#project-3-single-7-segment-display)
- [Project 4: 3-Digit Counter](#project-4-3-digit-counter)
- [Understanding Non-Blocking Code](#understanding-non-blocking-code)
- [Next Steps](#next-steps)

---

## What You'll Need

### Hardware

**Minimum (LED test):**
- Arduino Uno (or compatible)
- NJU3711 IC (DIP14 package recommended for breadboard)
- Breadboard
- Jumper wires
- 1× LED (any color)
- 1× 220Ω resistor

**For 7-segment display:**
- Common cathode 7-segment display
- 8× 220Ω resistors (one per segment)

**For 3-digit display:**
- 3× Common cathode 7-segment displays
- 24× 220Ω resistors
- 3× 2N3906 PNP transistors (or similar)
- 3× 1kΩ resistors (base resistors)

### Software

- Arduino IDE (1.8.x or 2.x)
- NJU3711 library (you're reading its docs!)

---

## Installation

### Step 1: Download the Library

1. Download the library as a ZIP file from GitHub
2. In Arduino IDE: **Sketch → Include Library → Add .ZIP Library**
3. Select the downloaded ZIP file
4. Restart Arduino IDE

### Step 2: Verify Installation

1. Go to **File → Examples → NJU3711**
2. You should see example sketches:
   - NJU3711_Basic
   - NJU3711_7Segment_Single_Demo
   - NJU3711_Advanced
   - NJU3711_7Segment_Cascade_Demo
   - NJU3711_7Segment_Cascade_Clock_Demo

If you see these, you're ready to go!

---

## Project 1: Blinking LED

Let's start simple - control a single LED with the NJU3711.

### Wiring

```
Arduino          NJU3711          LED
                 ┌─────┐
Pin 2  ────────→ │DATA │
Pin 3  ────────→ │CLK  │
Pin 4  ────────→ │STB  │
                 │     │
+5V    ────────→ │VDD  │
+5V    ────────→ │CLR  │
GND    ────────→ │VSS  │
                 │     │
                 │ P1  ├──[220Ω]──▷|───GND
                 └─────┘           LED
```

**Breadboard Layout:**
1. Place NJU3711 on breadboard (DIP14 package)
2. Connect VDD (pin 14) to +5V rail
3. Connect VSS (pin 4) to GND rail
4. Connect CLR (pin 11) to +5V rail
5. Connect DATA (pin 8) to Arduino pin 2
6. Connect CLK (pin 9) to Arduino pin 3
7. Connect STB (pin 10) to Arduino pin 4
8. Connect LED anode (+) to P1 (pin 12) through 220Ω resistor
9. Connect LED cathode (-) to GND

### Code

```cpp
#include <NJU3711.h>

// Create NJU3711 instance
NJU3711 expander(2, 3, 4); // DATA=2, CLK=3, STB=4

void setup() {
    Serial.begin(9600);
    Serial.println("NJU3711 Blinking LED");
    
    // Initialize the expander
    expander.begin();
}

void loop() {
    // CRITICAL: Must call update() every loop
    expander.update();
    
    // Turn on bit 0 (P1) - LED on
    expander.setBit(0);
    delay(500);
    
    // Wait for operation to complete
    while (expander.isBusy()) {
        expander.update();
    }
    
    // Turn off bit 0 (P1) - LED off
    expander.clearBit(0);
    delay(500);
    
    // Wait for operation to complete
    while (expander.isBusy()) {
        expander.update();
    }
}
```

### What's Happening?

1. **`expander.begin()`** - Initializes the IC
2. **`expander.update()`** - Processes the state machine (MUST call every loop!)
3. **`expander.setBit(0)`** - Turns on P1 (sets bit 0 HIGH)
4. **`expander.clearBit(0)`** - Turns off P1 (sets bit 0 LOW)
5. **`while (expander.isBusy())`** - Waits for operation to finish

**Upload and test!** Your LED should blink on and off every 500ms.

---

## Project 2: LED Bar Graph

Let's control 8 LEDs to make a bar graph.

### Wiring

Connect 8 LEDs to P1-P8, each with a 220Ω current-limiting resistor:

```
NJU3711
   │
   ├─ P1 ──[220Ω]──▷|──GND
   ├─ P2 ──[220Ω]──▷|──GND
   ├─ P3 ──[220Ω]──▷|──GND
   ├─ P4 ──[220Ω]──▷|──GND
   ├─ P5 ──[220Ω]──▷|──GND
   ├─ P6 ──[220Ω]──▷|──GND
   ├─ P7 ──[220Ω]──▷|──GND
   └─ P8 ──[220Ω]──▷|──GND
```

### Code

```cpp
#include <NJU3711.h>

NJU3711 expander(2, 3, 4);

void setup() {
    Serial.begin(9600);
    Serial.println("LED Bar Graph");
    expander.begin();
}

void loop() {
    expander.update();
    
    // Animated bar graph - fill from bottom to top
    for (int i = 0; i <= 8; i++) {
        // Wait until ready
        while (expander.isBusy()) {
            expander.update();
        }
        
        // Create a bar of 'i' LEDs lit
        uint8_t pattern = (1 << i) - 1; // Example: i=3 gives 0b00000111
        expander.write(pattern);
        
        delay(200);
    }
    
    delay(500);
    
    // Clear all
    while (expander.isBusy()) {
        expander.update();
    }
    expander.clear();
    delay(500);
}
```

### Understanding the Pattern

The pattern `(1 << i) - 1` creates a bar graph:

| i | Binary | LEDs Lit |
|---|--------|----------|
| 0 | 0b00000000 | None |
| 1 | 0b00000001 | P1 |
| 2 | 0b00000011 | P1, P2 |
| 3 | 0b00000111 | P1, P2, P3 |
| 4 | 0b00001111 | P1-P4 |
| 8 | 0b11111111 | All |

**Try it!** You should see the LEDs fill up like a bar graph.

---

## Project 3: Single 7-Segment Display

Now let's display digits on a 7-segment display!

### Wiring

```
           NJU3711                7-Segment Display
           ┌─────┐                (Common Cathode)
           │ P8  ├──[220Ω]──────→ A (pin 7)
           │ P7  ├──[220Ω]──────→ F (pin 9)
           │ P6  ├──[220Ω]──────→ B (pin 6)
           │ P5  ├──[220Ω]──────→ G (pin 10)
           │ P4  ├──[220Ω]──────→ E (pin 1)
           │ P3  ├──[220Ω]──────→ C (pin 4)
           │ P2  ├──[220Ω]──────→ D (pin 2)
           │ P1  ├──[220Ω]──────→ DP (pin 5)
           └─────┘
                             Common Cathode → GND
```

**Important:** 
- Each segment needs its own 220Ω resistor
- Connect common cathode pin(s) to GND
- Verify your display's pinout (they vary!)

### Code

```cpp
#include <NJU3711_7Segment.h>

// Use ACTIVE_LOW for common cathode displays
NJU3711_7Segment display(2, 3, 4, ACTIVE_LOW);

int counter = 0;

void setup() {
    Serial.begin(9600);
    Serial.println("7-Segment Counter");
    
    display.begin();
    
    // Test all segments
    display.displayAll();
    delay(1000);
    display.displayBlank();
    delay(500);
}

void loop() {
    // CRITICAL: Must call update() every loop
    display.update();
    
    // Display current digit
    if (!display.isBusy()) {
        display.displayDigit(counter);
        
        Serial.print("Displaying: ");
        Serial.println(counter);
        
        counter++;
        if (counter > 9) {
            counter = 0;
        }
        
        delay(1000); // Wait 1 second between digits
    }
}
```

### What's Happening?

1. **`NJU3711_7Segment`** - Specialized class for 7-segment displays
2. **`ACTIVE_LOW`** - LEDs turn ON when output is LOW (common cathode)
3. **`displayDigit(counter)`** - Shows digits 0-9 automatically!
4. The library handles all the segment patterns for you

**Upload and test!** You should see digits 0-9 counting up.

### Try These Variations

**Display with decimal point:**
```cpp
display.displayDigit(5, true); // Shows "5."
```

**Display hexadecimal:**
```cpp
display.displayHex(0xA); // Shows "A"
display.displayHex(0xF); // Shows "F"
```

**Display characters:**
```cpp
display.displayChar('E');  // Shows "E"
display.displayChar('-');  // Shows "-"
```

**Animations:**
```cpp
display.startAnimation(ANIM_ROTATE_CW, 200000); // Spinning segments
display.startAnimation(ANIM_LOADING, 150000);   // Loading bar
```

---

## Project 4: 3-Digit Counter

Let's build a 3-digit counter (000-999) with multiplexing!

### Wiring

This is more complex - we need transistors to control each digit.

```
Arduino    NJU3711    Segments (shared to all 3 displays)
Pin 2  ──→ DATA  ──→ A, B, C, D, E, F, G, DP (through resistors)
Pin 3  ──→ CLK
Pin 4  ──→ STB

Digit Selection (High-Side PNP Transistors):

Pin 5 ──[1kΩ]── Base (2N3906)
                Emitter ──→ +5V
                Collector ──→ Display 1 Common

Pin 6 ──[1kΩ]── Base (2N3906)
                Emitter ──→ +5V
                Collector ──→ Display 2 Common

Pin 7 ──[1kΩ]── Base (2N3906)
                Emitter ──→ +5V
                Collector ──→ Display 3 Common
```

**Circuit Explanation:**

1. **Shared segments:** All three displays connect to the same NJU3711 outputs
2. **Individual commons:** Each display's common pin connects to a PNP transistor
3. **PNP transistors:** When Arduino pin is LOW, transistor turns ON, digit lights up
4. **Multiplexing:** Only one digit is on at a time, but cycles so fast they all appear lit

### Breadboard Tips

1. Use three separate 7-segment displays
2. Wire all A pins together, all B pins together, etc.
3. Add 220Ω resistor to each NJU3711 output before the shared connection
4. Use 1kΩ base resistors for the PNP transistors
5. Keep wiring neat - this circuit has many connections!

### Code

```cpp
#include <NJU3711_7Segment_Multi.h>

// Create 3-digit display
// Parameters: DATA, CLK, STB, Digit1, Digit2, Digit3, Mode
NJU3711_7Segment_Multi display(2, 3, 4, 5, 6, 7, ACTIVE_LOW);

int counter = 0;

void setup() {
    Serial.begin(9600);
    Serial.println("3-Digit Counter");
    
    display.begin();
    
    // Optional: Set multiplexing speed
    display.setMultiplexDelay(2000); // 2ms per digit (default)
    
    // Test pattern
    display.displayAll();
    delay(1000);
    display.clearDisplay();
    delay(500);
}

void loop() {
    // CRITICAL: Must call update() every loop
    display.update();
    
    // Display the counter
    if (!display.isBusy() && !display.isMultiplexing()) {
        display.displayNumber(counter);
        
        Serial.print("Count: ");
        Serial.println(counter);
        
        counter++;
        if (counter > 999) {
            counter = 0;
        }
        
        delay(100); // Count up every 100ms
    }
}
```

### What's Happening?

1. **`NJU3711_7Segment_Multi`** - Handles multiplexing automatically
2. **`display.update()`** - Manages both NJU3711 operations AND multiplexing
3. **`displayNumber(counter)`** - Shows 0-999, handles digit separation
4. **Multiplexing** - Happens in background, you don't see it!

**Upload and test!** You should see a 3-digit counter from 000-999.

### Try These Variations

**Count with leading zeros:**
```cpp
display.setLeadingZeros(true);
display.displayNumber(5); // Shows "005" instead of "  5"
```

**Display with decimal point:**
```cpp
display.displayNumber(123, 2); // Shows "12.3"
display.displayNumber(45, 3);  // Shows " 4.5"
```

**Display special messages:**
```cpp
display.displayError();        // Shows "Err"
display.displayDashes();       // Shows "---"
display.displayTemperature(72); // Shows " 72"
```

**Control individual digits:**
```cpp
display.setDigit(0, 1);        // Leftmost = "1"
display.setDigit(1, 2, true);  // Middle = "2."
display.setDigit(2, 3);        // Rightmost = "3"
// Result: "12.3"
```

**Adjust brightness (via timing):**
```cpp
// Brighter (longer on-time per digit)
display.setMultiplexDelay(3000); // 3ms per digit

// Dimmer (shorter on-time per digit)
display.setMultiplexDelay(1000); // 1ms per digit
```

---

## Understanding Non-Blocking Code

The NJU3711 library uses **non-blocking** operation. This is important!

### What Does Non-Blocking Mean?

**Blocking code** (BAD):
```cpp
void loop() {
    // This locks up for 1 second - nothing else can happen
    shiftOut(dataPin, clockPin, MSBFIRST, data);
    delay(1000);
}
```

**Non-blocking code** (GOOD):
```cpp
void loop() {
    // update() processes quickly, then returns
    // Your code continues running
    expander.update();
    
    // You can do other things while operations complete
    readSensors();
    checkButtons();
}
```

### Why Does This Matter?

With non-blocking code, you can:
- Read sensors while updating displays
- Respond to button presses immediately
- Run multiple tasks "simultaneously"
- Create smooth animations

### The Two Critical Rules

**Rule 1: Always call `update()` in `loop()`**

```cpp
void loop() {
    expander.update(); // MUST be here, every loop!
    
    // Your code...
}
```

**Rule 2: Check `isBusy()` before queuing operations**

```cpp
void loop() {
    expander.update();
    
    // Only send new data when ready
    if (!expander.isBusy()) {
        expander.write(newData);
    }
}
```

### Example: Non-Blocking Sensor Reading

```cpp
#include <NJU3711_7Segment.h>

NJU3711_7Segment display(2, 3, 4, ACTIVE_LOW);

int analogValue = 0;
unsigned long lastRead = 0;

void setup() {
    display.begin();
}

void loop() {
    // Always update display
    display.update();
    
    // Read sensor every 100ms (non-blocking)
    if (millis() - lastRead >= 100) {
        analogValue = analogRead(A0) / 114; // Scale to 0-9
        lastRead = millis();
    }
    
    // Update display when ready
    if (!display.isBusy()) {
        display.displayDigit(analogValue);
    }
    
    // No delay() - everything runs smoothly!
}
```

This code:
- Reads sensor 10 times per second
- Updates display continuously
- Never blocks - always responsive!

---

## Common Beginner Mistakes

### Mistake 1: Forgetting `update()`

```cpp
void loop() {
    // WRONG - update() missing!
    expander.write(0xFF);
    delay(1000);
}
```

**Fix:**
```cpp
void loop() {
    expander.update(); // Add this!
    expander.write(0xFF);
    delay(1000);
}
```

### Mistake 2: Not Waiting for Operations

```cpp
void loop() {
    expander.update();
    
    // WRONG - might be busy from previous operation
    expander.write(0x01);
    expander.write(0x02);
    expander.write(0x03);
}
```

**Fix:**
```cpp
void loop() {
    expander.update();
    
    if (!expander.isBusy()) {
        expander.write(0x01);
    }
}
```

### Mistake 3: Wrong Display Mode

```cpp
// Common cathode display, but using wrong mode
NJU3711_7Segment display(2, 3, 4, ACTIVE_HIGH); // WRONG!
```

**Fix:**
```cpp
// Common cathode needs ACTIVE_LOW
NJU3711_7Segment display(2, 3, 4, ACTIVE_LOW); // Correct!
```

### Mistake 4: Missing Current-Limiting Resistors

```cpp
// LEDs connected directly to NJU3711 outputs - NO!
// LEDs will burn out or damage the IC
```

**Fix:**
- Always use 220-470Ω resistors with LEDs
- One resistor per LED/segment
- Place between NJU3711 output and LED

### Mistake 5: Forgetting Pull-Down/Pull-Up for FETs

When using MOSFETs (your PCB uses transistors, but if you're experimenting):

```cpp
// Gate left floating - unpredictable behavior
```

**Fix:**
- Add 10kΩ pull-down resistor from gate to ground (N-Channel)
- Add 10kΩ pull-up resistor from gate to VDD (P-Channel)

---

## Troubleshooting Guide

### Nothing Works

**Check List:**
1. Power connected? (VDD to +5V, VSS to GND)
2. CLR pin HIGH? (connected to +5V)
3. `begin()` called in `setup()`?
4. `update()` called in `loop()`?
5. USB cable connected to Arduino?

**Test Code:**
```cpp
void setup() {
    Serial.begin(9600);
    expander.begin();
    Serial.println("Starting...");
}

void loop() {
    expander.update();
    
    if (!expander.isBusy()) {
        Serial.println("Writing 0xFF");
        expander.write(0xFF);
        delay(1000);
    }
}
```

### LEDs Always On or Always Off

**Possible causes:**
1. Wrong display mode (ACTIVE_LOW vs ACTIVE_HIGH)
2. Common cathode/anode reversed
3. Missing resistors
4. Damaged LEDs

**Test:**
```cpp
// Try both modes
display.setDisplayMode(ACTIVE_LOW);
display.displayDigit(8);
delay(2000);

display.setDisplayMode(ACTIVE_HIGH);
display.displayDigit(8);
delay(2000);
```

### Display Shows Wrong Numbers

**Possible causes:**
1. Segments wired to wrong NJU3711 pins
2. Display pinout different from standard

**Solution:**
- Verify your display's datasheet
- Use `displayRaw()` to test individual segments:

```cpp
// Test segment A (should be top)
display.displayRaw(0b10000000);
delay(1000);

// Test segment B (should be top-right)
display.displayRaw(0b00100000);
delay(1000);

// Continue for all segments...
```

### Multiplexed Display Flickers

**Possible causes:**
1. Multiplex delay too long
2. `update()` not called frequently enough
3. `delay()` blocking the code

**Solutions:**
```cpp
// Reduce multiplex delay
display.setMultiplexDelay(1500); // Try 1.5ms

// Remove delay() calls
// Use millis() timing instead

// Ensure update() is first in loop()
void loop() {
    display.update(); // FIRST!
    // Other code...
}
```

### Ghosting Between Digits

**Possible causes:**
1. Blanking time too short
2. Slow transistor switching

**Solution:**
```cpp
// Increase blanking time
display.setBlankingTime(100); // Try 100µs
```

---

## Next Steps

Congratulations! You've learned:
- Basic NJU3711 operation
- LED control
- 7-segment displays
- Multiplexed multi-digit displays
- Non-blocking programming

### Where to Go Next

1. **[Advanced Usage](Advanced_Usage.md)**
   - Multiple NJU3711 devices
   - Complex animations
   - Cascaded displays (6+ digits)
   - Performance optimization

2. **[API Reference](API_Reference.md)**
   - Complete method documentation
   - All parameters and return values
   - Every class and function

3. **[Hardware Guide](Hardware_Guide.md)**
   - Detailed wiring diagrams
   - PCB design considerations
   - Power calculations
   - Component selection

4. **Example Sketches**
   - Study the included examples
   - Modify them for your projects
   - Learn advanced techniques

### Project Ideas

**Easy:**
- Temperature display (with sensor)
- Countdown timer
- Button-controlled counter
- Dice simulator

**Medium:**
- Digital clock (with RTC module)
- Voltmeter (0-99.9V range)
- RPM counter (with Hall sensor)
- Stopwatch with lap times

**Advanced:**
- 6-digit clock with date
- Frequency counter
- Calculator (with keypad)
- Game score display

### Getting Help

If you're stuck:

1. Check the [Troubleshooting Guide](Troubleshooting.md)
2. Review the example code
3. Verify your wiring against the Hardware Guide
4. Post questions on Arduino forums with:
   - Your code
   - Wiring diagram or photo
   - What you expected vs what happened

---

## Quick Reference Card

### Essential Code Template

```cpp
#include <NJU3711.h>

NJU3711 expander(2, 3, 4);

void setup() {
    Serial.begin(9600);
    expander.begin();
}

void loop() {
    expander.update(); // ALWAYS CALL THIS!
    
    if (!expander.isBusy()) {
        // Your code here
    }
}
```

### Most Used Methods

| Method | Purpose |
|--------|---------|
| `begin()` | Initialize in setup() |
| `update()` | Call every loop() |
| `isBusy()` | Check if ready |
| `write(data)` | Write 8-bit value |
| `setBit(n)` | Turn on one output |
| `clearBit(n)` | Turn off one output |
| `clear()` | Turn off all outputs |

### 7-Segment Display Methods

| Method | Purpose |
|--------|---------|
| `displayDigit(n)` | Show digit 0-9 |
| `displayHex(n)` | Show hex 0-F |
| `displayChar(c)` | Show character |
| `displayBlank()` | Clear display |
| `setDecimalPoint(bool)` | Control DP |

### Multi-Digit Display Methods

| Method | Purpose |
|--------|---------|
| `displayNumber(n)` | Show 0-999 |
| `setLeadingZeros(bool)` | Show/hide leading zeros |
| `setMultiplexDelay(us)` | Adjust brightness |
| `displayError()` | Show "Err" |

---

Happy making!

For more help, see:
- [API Reference](API_Reference.md)
- [Hardware Guide](Hardware_Guide.md)
- [Advanced Usage](Advanced_Usage.md)
- [Troubleshooting](Troubleshooting.md)