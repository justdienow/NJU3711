# Troubleshooting Guide

Solutions to common problems with the NJU3711 library.

## Table of Contents

- [Display Issues](#display-issues)
- [Hardware Problems](#hardware-problems)
- [Software/Code Issues](#softwarecode-issues)
- [Multiplexing Problems](#multiplexing-problems)
- [Performance Issues](#performance-issues)
- [Electrical Problems](#electrical-problems)
- [Diagnostic Tools](#diagnostic-tools)

---

## Software/Code Issues

### Code Won't Compile

**Error: "NJU3711.h: No such file or directory"**

**Solutions:**
1. Install library via Arduino IDE
2. Restart Arduino IDE after installation
3. Verify: File → Examples → NJU3711 appears
4. Check: Sketch → Include Library → NJU3711 listed

**Error: "'NJU3711' does not name a type"**

**Solution:**
```cpp
// Add at top of sketch
#include <NJU3711.h>
```

**Error: "no matching function for call to 'NJU3711::NJU3711(int, int, int)'"**

**Solution:**
```cpp
// Use uint8_t or cast
NJU3711 expander(2, 3, 4); // Correct

// Or explicit cast
NJU3711 expander((uint8_t)2, (uint8_t)3, (uint8_t)4);
```

### Operations Don't Execute

**Symptoms:** Commands sent but nothing happens

**Possible Causes & Solutions:**

1. **Missing `update()` call**
   ```cpp
   void loop() {
       expander.update(); // CRITICAL!
       
       if (!expander.isBusy()) {
           expander.write(0xFF);
       }
   }
   ```

2. **Queue full**
   ```cpp
   void loop() {
       expander.update();
       
       // Check queue before adding
       if (expander.getQueueSize() < 7) {
           expander.write(data);
       } else {
           Serial.println("Queue full!");
       }
   }
   ```

3. **Not waiting for busy**
   ```cpp
   // WRONG
   expander.write(0x01);
   expander.write(0x02); // May fail if busy
   
   // CORRECT
   expander.write(0x01);
   while (expander.isBusy()) {
       expander.update();
   }
   expander.write(0x02);
   ```

4. **Return value ignored**
   ```cpp
   // Check if operation queued successfully
   if (!expander.write(0xFF)) {
       Serial.println("Failed to queue operation!");
   }
   ```

### Display Shows Random Values

**Symptoms:** Unpredictable display output

**Possible Causes & Solutions:**

1. **Uninitialized variables**
   ```cpp
   // BAD
   int value;
   display.displayDigit(value); // Random!
   
   // GOOD
   int value = 0;
   display.displayDigit(value);
   ```

2. **Buffer overflow**
   ```cpp
   uint8_t digits[3]; // Array size 3
   digits[5] = 7; // BAD - out of bounds!
   ```

3. **Race condition**
   ```cpp
   // BAD - modifying during display
   volatile int counter = 0;
   
   void loop() {
       display.displayNumber(counter); // May read partial update
   }
   
   void interrupt() {
       counter++; // Modifying during display
   }
   
   // GOOD - atomic access
   void loop() {
       noInterrupts();
       int localCounter = counter;
       interrupts();
       display.displayNumber(localCounter);
   }
   ```

4. **Floating point precision**
   ```cpp
   // Be careful with float to int conversion
   float voltage = 12.34;
   int displayValue = (int)(voltage * 10); // 123
   display.displayNumber(displayValue, 2); // "12.3"
   ```

### Memory Issues

**Error: "Low memory available, stability problems may occur"**

**Solutions:**

1. **Reduce global variables**
   ```cpp
   // BAD
   char buffer[1000]; // 1KB wasted
   
   // GOOD
   char buffer[50]; // Only what's needed
   ```

2. **Use PROGMEM for constants**
   ```cpp
   // Store in flash instead of RAM
   const char message[] PROGMEM = "Long string here...";
   ```

3. **Avoid String class**
   ```cpp
   // BAD (uses heap)
   String text = "Hello";
   
   // GOOD (uses stack)
   char text[] = "Hello";
   ```

4. **Check free RAM**
   ```cpp
   int freeRam() {
       extern int __heap_start, *__brkval;
       int v;
       return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
   }
   
   void setup() {
       Serial.begin(9600);
       Serial.print("Free RAM: ");
       Serial.println(freeRam());
   }
   ```

---

## Multiplexing Problems

### Ghosting Between Digits

**Symptoms:** Previous digit visible on next digit

**Possible Causes & Solutions:**

1. **Blanking time too short**
   ```cpp
   // Increase blanking period
   display.setBlankingTime(100); // 100µs
   ```

2. **Slow transistor switching**
   - Check base resistor (should be ~1kΩ)
   - Try faster transistors (2N3904, 2N2222)
   - Add pull-up/pull-down to base

3. **Capacitive coupling**
   - Shorten wires
   - Separate segment and digit wires
   - Add small capacitor (100pF) base to ground

4. **Display latency**
   - Some displays have slow phosphor
   - Increase blanking time
   - Try different display

**Test Code:**
```cpp
void testGhosting() {
    // Display pattern that shows ghosting
    display.setDigit(0, 8); // All segments
    display.setDigit(1, 0); // No segments
    display.setDigit(2, 8); // All segments
    
    // Increase blanking until ghosting disappears
    for (int blanking = 10; blanking <= 200; blanking += 10) {
        display.setBlankingTime(blanking);
        Serial.print("Blanking: ");
        Serial.println(blanking);
        delay(2000);
    }
}
```

### Uneven Brightness Between Digits

**Symptoms:** Some digits brighter than others

**Possible Causes & Solutions:**

1. **Different resistor values**
   - Check all resistors with multimeter
   - Replace with matched set
   - Use 1% tolerance resistors

2. **Transistor Vce variation**
   - Different transistors have different drops
   - Use matched transistors
   - Or use same part number from same batch

3. **Timing variation**
   - Check with oscilloscope
   - Verify equal on-time per digit
   - Adjust in code if needed

4. **Display quality variation**
   - Some segments naturally dimmer
   - Try different displays
   - Compensate in software

**Compensation Code:**
```cpp
// Adjust on-time per digit for brightness matching
const unsigned long digitDelays[3] = {2000, 2100, 1900}; // µs

void refreshDisplay() {
    // Use digitDelays[currentDigit] instead of fixed delay
    if (micros() - lastRefresh >= digitDelays[currentDigit]) {
        // ... switching code ...
    }
}
```

### Digits Turn On Out of Order

**Symptoms:** Wrong digit sequence

**Possible Causes & Solutions:**

1. **Incorrect pin mapping**
   ```cpp
   // Verify physical layout matches code
   // Leftmost should be digit 0, rightmost digit 2
   
   // If physical order is different:
   const uint8_t digitMap[3] = {2, 0, 1}; // Logical to physical
   
   void refreshDisplay() {
       // Use mapped pin
       digitalWrite(digitPins[digitMap[currentDigit]], LOW);
   }
   ```

2. **Wiring error**
   - Trace each digit control wire
   - Verify pin assignments
   - Test each digit individually

**Test Code:**
```cpp
void testDigitOrder() {
    // Light up one digit at a time
    for (int d = 0; d < 3; d++) {
        display.disableAllDigits();
        display.enableDigit(d);
        display.displayDigit(d);
        
        Serial.print("Digit ");
        Serial.print(d);
        Serial.println(" should be lit");
        delay(2000);
    }
}
```

---

## Performance Issues

### Slow Response

**Symptoms:** Delay between command and display update

**Possible Causes & Solutions:**

1. **Step delay too long**
   ```cpp
   // Default is 1µs (fast)
   expander.setStepDelay(1);
   
   // If you increased it, reduce back
   // expander.setStepDelay(100); // Too slow!
   ```

2. **Queue backed up**
   ```cpp
   void loop() {
       expander.update();
       
       // Monitor queue
       if (expander.getQueueSize() > 0) {
           Serial.print("Queue: ");
           Serial.println(expander.getQueueSize());
       }
   }
   ```

3. **Blocking delays**
   ```cpp
   // BAD - blocks everything
   void loop() {
       expander.update();
       delay(1000); // Everything stops!
   }
   
   // GOOD - non-blocking
   void loop() {
       expander.update();
       
       static unsigned long last = 0;
       if (millis() - last >= 1000) {
           // Do something
           last = millis();
       }
   }
   ```

4. **Slow sensor reading**
   ```cpp
   // BAD - blocking sensor read
   void loop() {
       display.update();
       int value = analogRead(A0); // ~100µs each
       delay(10); // More blocking!
       value += analogRead(A0);
   }
   
   // GOOD - spread out over time
   void loop() {
       display.update();
       
       static unsigned long lastRead = 0;
       if (millis() - lastRead >= 100) {
           int value = analogRead(A0);
           // Process...
           lastRead = millis();
       }
   }
   ```

### High CPU Usage

**Symptoms:** Arduino slow or unresponsive

**Possible Causes & Solutions:**

1. **Calling update() too much**
   ```cpp
   // BAD - excessive calls
   void loop() {
       for (int i = 0; i < 100; i++) {
           expander.update(); // Only need once!
       }
   }
   
   // GOOD - once per loop
   void loop() {
       expander.update();
       // Other code...
   }
   ```

2. **Tight loops without delays**
   ```cpp
   // BAD - consumes all CPU
   void loop() {
       expander.update();
       while (expander.isBusy()) {
           expander.update();
       }
       // No breathing room!
   }
   
   // GOOD - allow other processing
   void loop() {
       expander.update();
       
       if (!expander.isBusy()) {
           // Do work
       }
       // Loop continues, allows interrupts, etc.
   }
   ```

3. **Complex calculations**
   ```cpp
   // Spread heavy calculations over time
   void loop() {
       display.update();
       
       static int calculationStep = 0;
       
       // Do one step per loop
       switch (calculationStep) {
           case 0: calculatePart1(); break;
           case 1: calculatePart2(); break;
           case 2: calculatePart3(); break;
       }
       
       calculationStep = (calculationStep + 1) % 3;
   }
   ```

---

## Electrical Problems

### Display Dims When Updated

**Symptoms:** Brightness drops during writes

**Possible Causes & Solutions:**

1. **Power supply current limit**
   ```
   Problem: Supply can't provide enough current
   Test: Measure voltage during operation (should stay >4.5V)
   Fix: Use higher-current supply (500mA+)
   ```

2. **Voltage drop in wiring**
   ```
   Problem: Long or thin wires
   Test: Measure voltage at IC vs at power supply
   Fix: Use shorter, thicker wires (22 AWG or better)
   Fix: Add bulk capacitor (100µF) near display
   ```

3. **USB current limit**
   ```
   Problem: USB ports limited to 500mA
   Fix: Use external power supply
   Fix: Power displays separately from Arduino
   ```

**Power Distribution:**
```
Wall Adapter (5V, 1A)
    │
    ├─→ Arduino VIN
    │
    └─→ Display Power (separate rail)
         │
         └─→ 100µF capacitor to GND
```

### Noise on Display

**Symptoms:** Random flickering, segments turn on/off

**Possible Causes & Solutions:**

1. **Missing decoupling capacitors**
   ```
   Add near IC:
   - 100nF ceramic between VDD and VSS
   - 10µF electrolytic at power input
   ```

2. **Ground loops**
   ```
   - Use star ground topology
   - Connect all grounds to single point
   - Keep ground wires short
   ```

3. **EMI from other devices**
   ```
   - Separate signal wires from power wires
   - Add ferrite beads on long cables
   - Shield sensitive wires
   ```

4. **PWM interference**
   ```
   - Move PWM pins away from display signals
   - Add filtering on PWM outputs
   - Use hardware PWM if possible
   ```

### ESD Damage

**Symptoms:** IC works then suddenly fails

**Prevention:**

1. **Handling**
   - Touch grounded surface before handling IC
   - Use ESD-safe workstation
   - Store in anti-static bags

2. **Circuit protection**
   ```
   Add to inputs:
   - 100Ω series resistor
   - TVS diode to ground
   - 10kΩ pull-up/down
   ```

3. **Installation**
   - Power off before inserting IC
   - Insert into socket (don't solder directly during testing)
   - Ground yourself while working

---

## Diagnostic Tools

### Test Pattern Generator

```cpp
void runDiagnostics() {
    Serial.println("\n=== NJU3711 Diagnostics ===");
    
    // Test 1: All outputs
    Serial.println("Test 1: All outputs HIGH");
    expander.write(0xFF);
    delay(2000);
    
    // Test 2: Walking bit
    Serial.println("Test 2: Walking bit");
    for (int i = 0; i < 8; i++) {
        while (expander.isBusy()) expander.update();
        expander.write(1 << i);
        Serial.print("Bit ");
        Serial.println(i);
        delay(500);
    }
    
    // Test 3: Alternating pattern
    Serial.println("Test 3: Alternating");
    for (int i = 0; i < 10; i++) {
        while (expander.isBusy()) expander.update();
        expander.write(i % 2 ? 0xAA : 0x55);
        delay(200);
    }
    
    // Test 4: Binary counter
    Serial.println("Test 4: Binary counter");
    for (int i = 0; i < 256; i++) {
        while (expander.isBusy()) expander.update();
        expander.write(i);
        delay(50);
    }
    
    expander.clear();
    Serial.println("=== Diagnostics Complete ===\n");
}
```

### 7-Segment Tester

```cpp
void test7Segment() {
    Serial.println("\n=== 7-Segment Test ===");
    
    // Test all segments individually
    Serial.println("Testing segments A-G, DP:");
    const char* segNames[] = {"A", "B", "C", "D", "E", "F", "G", "DP"};
    const uint8_t segments[] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G, SEG_DP};
    
    for (int i = 0; i < 8; i++) {
        while (display.isBusy()) display.update();
        display.displayBlank();
        delay(200);
        
        display.setSegment(segments[i], true);
        Serial.print("Segment ");
        Serial.print(segNames[i]);
        Serial.println(" ON");
        delay(1000);
    }
    
    // Test all digits
    Serial.println("Testing digits 0-9:");
    for (int i = 0; i < 10; i++) {
        while (display.isBusy()) display.update();
        display.displayDigit(i);
        Serial.print("Digit: ");
        Serial.println(i);
        delay(500);
    }
    
    // Test hex
    Serial.println("Testing hex A-F:");
    for (int i = 10; i < 16; i++) {
        while (display.isBusy()) display.update();
        display.displayHex(i);
        Serial.print("Hex: ");
        Serial.println(i, HEX);
        delay(500);
    }
    
    Serial.println("=== Test Complete ===\n");
}
```

### Multiplexing Monitor

```cpp
void monitorMultiplexing() {
    static unsigned long lastReport = 0;
    static unsigned long cycleCount = 0;
    static unsigned long lastCycle = 0;
    
    // Count cycles
    static uint8_t lastDigit = 0;
    if (currentDigit == 0 && lastDigit == 2) {
        cycleCount++;
        unsigned long now = micros();
        unsigned long cycleTime = now - lastCycle;
        lastCycle = now;
    }
    lastDigit = currentDigit;
    
    // Report every second
    if (millis() - lastReport >= 1000) {
        float refreshRate = cycleCount;
        
        Serial.print("Refresh rate: ");
        Serial.print(refreshRate);
        Serial.println(" Hz");
        
        if (refreshRate < 60) {
            Serial.println("WARNING: Refresh rate too low! (should be >80Hz)");
        }
        
        cycleCount = 0;
        lastReport = millis();
    }
}
```

### Voltage Monitor

```cpp
void monitorVoltages() {
    static unsigned long lastCheck = 0;
    
    if (millis() - lastCheck >= 5000) {
        // Read Arduino's VCC (approximate)
        long result;
        // Read 1.1V reference against AVcc
        ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
        delay(2);
        ADCSRA |= _BV(ADSC);
        while (bit_is_set(ADCSRA, ADSC));
        result = ADCL;
        result |= ADCH << 8;
        result = 1126400L / result; // Back-calculate AVcc in mV
        
        float vcc = result / 1000.0;
        Serial.print("VCC: ");
        Serial.print(vcc);
        Serial.println(" V");
        
        if (vcc < 4.5) {
            Serial.println("WARNING: Voltage too low!");
        }
        
        lastCheck = millis();
    }
}
```

---

## Getting Help

If you're still stuck after trying these solutions:

### Before Asking for Help

1. **Read the documentation:**
   - [API Reference](API_Reference.md)
   - [Hardware Guide](Hardware_Guide.md)
   - [Getting Started](Getting_Started.md)

2. **Run diagnostics:**
   - Use test pattern generator above
   - Check voltages with multimeter
   - Verify wiring against Hardware Guide

3. **Simplify:**
   - Start with simplest example code
   - Test with single LED before display
   - Remove extra components

4. **Document the problem:**
   - Take clear photos of wiring
   - Copy exact error messages
   - Note what you've already tried

### Where to Ask

1. **Arduino Forums** - General Arduino questions
2. **GitHub Issues** - Library-specific bugs
3. **Stack Overflow** - Programming questions
4. **Reddit r/arduino** - Community help

### What to Include

```
**Hardware:**
- Arduino model: Uno R3
- NJU3711 package: DIP14
- Display type: Common cathode 7-segment
- Power supply: USB (5V, 500mA)

**Software:**
- Arduino IDE version: 2.2.1
- Library version: 2.0.0
- Example: NJU3711_Basic

**Problem:**
- Nothing displays
- Power LED on Arduino is lit
- Measured VDD = 5.0V

**What I've Tried:**
- Checked all wiring 3 times
- Ran test code from Getting Started
- Tried different display (same result)
- Added Serial.println for debugging

**Code:**
[Paste minimal code that shows problem]

**Photos:**
[Attach clear photos of setup]
```

---

## Quick Diagnostic Checklist

Use this checklist for fast troubleshooting:

- [ ] Power connected? (VDD = 5V, VSS = 0V)
- [ ] CLR pin HIGH? (Connected to 5V or controlled HIGH)
- [ ] `begin()` called in `setup()`?
- [ ] `update()` called every `loop()`?
- [ ] Correct display mode? (ACTIVE_LOW vs ACTIVE_HIGH)
- [ ] Current-limiting resistors present? (220-470Ω)
- [ ] Wiring matches library pin mapping?
- [ ] No short circuits? (Use continuity tester)
- [ ] Decoupling capacitor near IC? (100nF)
- [ ] Code compiles without errors?
- [ ] Serial monitor shows expected output?
- [ ] All ground connections made?
- [ ] Transistors correct orientation? (E-B-C)
- [ ] For multiplexing: All digit pins working?
- [ ] Test with known-good simple example?

---

**Remember:** 90% of problems are wiring issues! Double-check your connections before assuming a code problem.

For more information, see:
- [API Reference](API_Reference.md)
- [Hardware Guide](Hardware_Guide.md)
- [Getting Started](Getting_Started.md)
- [Advanced Usage](Advanced_Usage.md)

## Display Issues

### Nothing Displays

**Symptoms:** All segments off, no response

**Possible Causes & Solutions:**

1. **Power not connected**
   ```
   Check: VDD (pin 14) = +5V
   Check: VSS (pin 4) = GND
   Test: Measure with multimeter
   ```

2. **CLR pin not HIGH**
   ```
   Check: CLR (pin 11) connected to +5V or controlled HIGH
   Test: Measure CLR pin voltage (should be ~5V)
   Fix: Connect CLR to +5V rail
   ```

3. **`begin()` not called**
   ```cpp
   void setup() {
       expander.begin(); // MUST call this!
   }
   ```

4. **`update()` not called**
   ```cpp
   void loop() {
       expander.update(); // MUST call every loop!
   }
   ```

5. **Wrong display mode**
   ```cpp
   // Try both modes
   display.setDisplayMode(ACTIVE_LOW);
   // or
   display.setDisplayMode(ACTIVE_HIGH);
   ```

**Quick Test:**
```cpp
void setup() {
    Serial.begin(9600);
    expander.begin();
    
    // Turn on all outputs
    expander.write(0xFF);
    Serial.println("All outputs HIGH");
}

void loop() {
    expander.update();
}
```

### All Segments Always On

**Symptoms:** Display shows "8." constantly

**Possible Causes & Solutions:**

1. **Wrong display mode (inverted)**
   ```cpp
   // For common cathode, use ACTIVE_LOW
   NJU3711_7Segment display(2, 3, 4, ACTIVE_LOW);
   
   // For common anode, use ACTIVE_HIGH
   NJU3711_7Segment display(2, 3, 4, ACTIVE_HIGH);
   ```

2. **Missing current-limiting resistors**
   - Segments may be damaged
   - Add 220-470Ω resistors to each segment
   - Replace damaged display

3. **Short circuit in wiring**
   - Check for solder bridges on PCB
   - Verify breadboard connections
   - Use continuity tester

4. **STB pin stuck LOW**
   - Data continuously latching
   - Check STB pin wiring
   - Measure STB voltage (should toggle)

**Test Code:**
```cpp
void loop() {
    display.update();
    
    // Cycle through digits
    for (int i = 0; i < 10; i++) {
        while (display.isBusy()) display.update();
        display.displayDigit(i);
        delay(500);
    }
}
```

### Wrong Segments Light Up

**Symptoms:** Display shows scrambled patterns

**Possible Causes & Solutions:**

1. **Incorrect segment mapping**
   - Your display pinout may differ
   - Verify with datasheet
   - Test each segment individually:
   
   ```cpp
   void testSegments() {
       display.update();
       
       // Test each segment
       for (int seg = 0; seg < 8; seg++) {
           while (display.isBusy()) display.update();
           
           display.displayBlank();
           delay(200);
           
           display.setSegment(seg, true);
           Serial.print("Segment ");
           Serial.print(seg);
           Serial.println(" ON");
           delay(1000);
       }
   }
   ```

2. **Crossed wires**
   - Swap wires to match library's mapping
   - Or modify library segment patterns

3. **Damaged display**
   - Some segments burned out
   - Test with known-good code
   - Replace if necessary

### Dim Display

**Symptoms:** Display barely visible

**Possible Causes & Solutions:**

1. **Resistors too high**
   ```
   Current: 220Ω typical
   Try: 150Ω or 180Ω for brighter display
   
   Calculate: R = (5V - Vf) / Iled
   Example: R = (5V - 2V) / 0.020A = 150Ω
   ```

2. **Power supply voltage low**
   ```
   Check: Measure VDD under load
   Should be: 4.5V minimum
   Fix: Use better power supply or separate supply for displays
   ```

3. **Multiplexing too fast (multi-digit)**
   ```cpp
   // Increase on-time per digit
   display.setMultiplexDelay(3000); // 3ms instead of 2ms
   ```

4. **Viewing angle**
   - Some displays are dim from certain angles
   - Adjust physical orientation

**Brightness Test:**
```cpp
void testBrightness() {
    display.begin();
    
    // Turn on all segments (brightest)
    display.displayAll();
    
    // Measure current draw to verify resistors
    // Should be ~80-120mA for 3-digit display
}
```

### Flickering Display

**Symptoms:** Display flickers or blinks

**Possible Causes & Solutions:**

1. **Power supply insufficient**
   ```
   Check: Current capacity
   Need: ~150mA for 3-digit display + Arduino
   Fix: Use 500mA+ power supply
   ```

2. **Loose connections**
   - Check all jumper wires
   - Resolder connections on PCB
   - Test continuity

3. **`update()` not called frequently**
   ```cpp
   void loop() {
       display.update(); // Must be FIRST!
       
       // Remove delay() calls
       // Use millis() timing instead
   }
   ```

4. **Multiplexing too slow**
   ```cpp
   // For 3 digits:
   display.setMultiplexDelay(2000); // 2ms = ~167Hz refresh
   
   // Too slow causes flicker:
   // display.setMultiplexDelay(5000); // 5ms = 67Hz (may flicker)
   ```

5. **Blocking code (delay)**
   ```cpp
   // BAD - blocks update()
   void loop() {
       display.update();
       delay(1000); // Stops multiplexing!
   }
   
   // GOOD - non-blocking
   void loop() {
       display.update();
       
       static unsigned long lastAction = 0;
       if (millis() - lastAction >= 1000) {
           // Do something
           lastAction = millis();
       }
   }
   ```

---

## Hardware Problems

### IC Gets Hot

**Symptoms:** NJU3711 IC warm or hot to touch

**Possible Causes & Solutions:**

1. **Output short circuit**
   - Check for shorts to ground/power
   - Each output limited to 25mA
   - Disconnect outputs and test

2. **Excessive current draw**
   - Too many LEDs without resistors
   - Calculate total current
   - Add/increase current-limiting resistors

3. **Wrong power supply voltage**
   - Check VDD = 5V (not 9V or 12V!)
   - Use regulated 5V supply
   - Measure actual voltage

4. **Damaged IC**
   - If hot with no load, likely damaged
   - Replace IC
   - Check for ESD damage

**Safe Operating Limits:**
```
Per Output: 25mA maximum
All Outputs: 200mA maximum (25mA × 8)
IC Logic: <1mA
Total: <201mA
```

### Intermittent Operation

**Symptoms:** Works sometimes, fails randomly

**Possible Causes & Solutions:**

1. **Loose connections**
   - Wiggle wires while running
   - Resolder all connections
   - Use proper connectors

2. **Breadboard issues**
   - Contacts worn out
   - Move to different area
   - Or switch to PCB

3. **ESD damage**
   - Handle IC carefully
   - Use anti-static precautions
   - May need replacement

4. **Power supply noise**
   ```
   Add: 100nF ceramic cap near VDD/VSS
   Add: 10µF electrolytic cap at power input
   Check: Clean 5V with oscilloscope
   ```

5. **Long wires**
   ```
   Problem: Capacitance, noise pickup
   Solution: Keep wires <6 inches
   Or: Add pull-up/pull-down resistors (10kΩ)
   Or: Increase step delay: expander.setStepDelay(5);
   ```

### Wrong Voltage Levels

**Symptoms:** Logic levels not correct

**Possible Causes & Solutions:**

1. **Mixed logic levels**
   - Arduino = 5V logic
   - Some boards = 3.3V logic
   - Use level shifter if needed

2. **Weak pull-up/pull-down**
   ```
   Add 10kΩ resistors:
   - Pull-down on N-Channel FET gates
   - Pull-up on P-Channel FET gates
   ```

3. **Voltage drop in wires**
   - Use thicker wires for power
   - Measure voltage at IC, not just Arduino
   - Add local decoupling capacitor

---

## See Also

- [API Reference](API_Reference.md) - Complete method documentation
- [Hardware Guide](Hardware_Guide.md) - Wiring and electrical specifications
- [Getting Started](Getting_Started.md) - Beginner tutorials
- [Advanced Usage](Advanced_Usage.md) - Complex applications
- [Troubleshooting](Troubleshooting.md) - Common issues and solutions