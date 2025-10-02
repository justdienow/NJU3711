# Advanced Usage Guide

Advanced techniques and applications for the NJU3711 library.

## Table of Contents

- [Multiple NJU3711 Devices](#multiple-nju3711-devices)
- [Cascaded 6-Digit Displays](#cascaded-6-digit-displays)
- [Building a Digital Clock](#building-a-digital-clock)
- [Custom Animations](#custom-animations)
- [Performance Optimization](#performance-optimization)
- [Driving High-Power Loads](#driving-high-power-loads)
- [Advanced Multiplexing Techniques](#advanced-multiplexing-techniques)
- [Real-World Applications](#real-world-applications)

---

## Multiple NJU3711 Devices

You can use multiple NJU3711 ICs independently to control more outputs.

### Independent Control (Different Pins)

Each IC gets its own set of control pins:

```cpp
#include <NJU3711.h>

// First device on pins 2, 3, 4
NJU3711 expander1(2, 3, 4);

// Second device on pins 5, 6, 7
NJU3711 expander2(5, 6, 7);

void setup() {
    expander1.begin();
    expander2.begin();
    
    // Optional: Different timing for each
    expander1.setStepDelay(1);   // Fast
    expander2.setStepDelay(10);  // Slower
}

void loop() {
    // Update both devices
    expander1.update();
    expander2.update();
    
    // Control independently
    if (!expander1.isBusy()) {
        expander1.write(0xAA); // Pattern 1
    }
    
    if (!expander2.isBusy()) {
        expander2.write(0x55); // Pattern 2
    }
}
```

### Synchronized Patterns

Create complementary or synchronized patterns across multiple devices:

```cpp
void loop() {
    expander1.update();
    expander2.update();
    
    // Wait for both to be ready
    if (!expander1.isBusy() && !expander2.isBusy()) {
        uint8_t pattern = millis() / 10; // Slow counter
        
        // Synchronized
        expander1.write(pattern);
        expander2.write(pattern);
        
        // Or complementary
        // expander1.write(pattern);
        // expander2.write(~pattern); // Inverted
    }
}
```

### Wave Patterns Across Multiple Devices

```cpp
unsigned long lastUpdate = 0;
float phase1 = 0, phase2 = 0;

void loop() {
    expander1.update();
    expander2.update();
    
    if (millis() - lastUpdate >= 50) {
        if (!expander1.isBusy() && !expander2.isBusy()) {
            // Create sine wave patterns
            uint8_t pattern1 = 0;
            uint8_t pattern2 = 0;
            
            for (int bit = 0; bit < 8; bit++) {
                // Device 1: Sine wave
                float angle1 = phase1 + (bit * PI / 4);
                if (sin(angle1) > 0) {
                    pattern1 |= (1 << bit);
                }
                
                // Device 2: Cosine wave (90° phase shift)
                float angle2 = phase2 + (bit * PI / 4);
                if (cos(angle2) > 0) {
                    pattern2 |= (1 << bit);
                }
            }
            
            expander1.write(pattern1);
            expander2.write(pattern2);
            
            phase1 += 0.1;
            phase2 += 0.1;
            lastUpdate = millis();
        }
    }
}
```

---

## Cascaded 6-Digit Displays

Build large displays by cascading multiple 3-digit modules.

### Hardware Setup

Two 3-digit PCBs sharing the DATA, CLK, and STB bus:

```
Arduino     PCB 1        PCB 2
            ┌─────┐      ┌─────┐
Pin 2 ────→ │DATA │────→ │DATA │
Pin 3 ────→ │CLK  │────→ │CLK  │
Pin 4 ────→ │STB  │────→ │STB  │
            └─────┘      └─────┘

Individual digit control:
Pin 5  → Digit 1 (hundreds)
Pin 6  → Digit 2 (tens)
Pin 7  → Digit 3 (ones)
Pin 8  → Digit 4 (hundreds)
Pin 9  → Digit 5 (tens)
Pin 10 → Digit 6 (ones)
```

### Software Implementation

Since the library's Multi class only supports 3 digits, we'll manually handle 6 digits:

```cpp
#include <NJU3711_7Segment.h>

NJU3711_7Segment display(2, 3, 4, ACTIVE_LOW);

// Digit control pins
const uint8_t digitPins[6] = {5, 6, 7, 8, 9, 10};

// Display buffer
uint8_t digits[6] = {0};
bool digitDP[6] = {false};

// Multiplexing state
uint8_t currentDigit = 0;
unsigned long lastRefresh = 0;
const unsigned long REFRESH_INTERVAL = 2000; // 2ms per digit

void setup() {
    display.begin();
    
    // Setup digit control pins
    for (int i = 0; i < 6; i++) {
        pinMode(digitPins[i], OUTPUT);
        digitalWrite(digitPins[i], HIGH); // OFF
    }
}

void loop() {
    display.update();
    refreshDisplay();
    
    // Your application code
    updateNumber();
}

void refreshDisplay() {
    unsigned long currentTime = micros();
    
    if (currentTime - lastRefresh >= REFRESH_INTERVAL) {
        // Turn off current digit
        digitalWrite(digitPins[currentDigit], HIGH);
        
        // Move to next digit
        currentDigit = (currentDigit + 1) % 6;
        
        // Write new segment data
        if (!display.isBusy()) {
            display.displayDigit(digits[currentDigit], digitDP[currentDigit]);
        }
        
        // Turn on new digit
        delayMicroseconds(10); // Brief blanking
        digitalWrite(digitPins[currentDigit], LOW);
        
        lastRefresh = currentTime;
    }
}

void displayNumber(long number) {
    // Extract digits (rightmost = ones)
    for (int i = 5; i >= 0; i--) {
        digits[i] = number % 10;
        number /= 10;
    }
}

void updateNumber() {
    static long counter = 0;
    static unsigned long lastCount = 0;
    
    if (millis() - lastCount >= 1) {
        displayNumber(counter);
        counter++;
        if (counter > 999999) counter = 0;
        lastCount = millis();
    }
}
```

### Advanced: Custom Digit Mapping

If your digits aren't in left-to-right order:

```cpp
// Physical layout: [D3][D1][D2] [D6][D4][D5]
const uint8_t digitMap[6] = {1, 2, 0, 4, 5, 3}; // Logical to physical

void refreshDisplay() {
    // ... previous code ...
    
    // Use mapped pin
    uint8_t physicalDigit = digitMap[currentDigit];
    digitalWrite(digitPins[physicalDigit], LOW);
    
    // ... rest of code ...
}
```

---

## Building a Digital Clock

Complete example of a 6-digit clock (HH:MM:SS).

### Hardware

- 6 digit displays (two 3-digit PCBs)
- DS3231 RTC module (for timekeeping)
- Optional: Buttons for setting time

### Full Clock Code

```cpp
#include <NJU3711_7Segment.h>
#include <Wire.h>
#include <RTClib.h> // DS3231 library

RTC_DS3231 rtc;
NJU3711_7Segment display(2, 3, 4, ACTIVE_LOW);

const uint8_t digitPins[6] = {5, 6, 7, 8, 9, 10};
uint8_t digits[6] = {0};
bool digitDP[6] = {false};

uint8_t currentDigit = 0;
unsigned long lastRefresh = 0;
unsigned long lastTimeUpdate = 0;
bool colonBlink = false;

void setup() {
    Serial.begin(9600);
    display.begin();
    
    // Initialize RTC
    if (!rtc.begin()) {
        Serial.println("RTC not found!");
        while (1);
    }
    
    // Uncomment to set time initially
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    
    // Setup digit pins
    for (int i = 0; i < 6; i++) {
        pinMode(digitPins[i], OUTPUT);
        digitalWrite(digitPins[i], HIGH);
    }
}

void loop() {
    display.update();
    refreshDisplay();
    updateTime();
    updateColonBlink();
}

void updateTime() {
    // Update every second
    if (millis() - lastTimeUpdate >= 1000) {
        DateTime now = rtc.now();
        
        uint8_t hours = now.hour();
        uint8_t minutes = now.minute();
        uint8_t seconds = now.second();
        
        // Convert to digits
        digits[0] = hours / 10;
        digits[1] = hours % 10;
        digits[2] = minutes / 10;
        digits[3] = minutes % 10;
        digits[4] = seconds / 10;
        digits[5] = seconds % 10;
        
        lastTimeUpdate = millis();
        
        Serial.print(hours);
        Serial.print(":");
        Serial.print(minutes);
        Serial.print(":");
        Serial.println(seconds);
    }
}

void updateColonBlink() {
    static unsigned long lastBlink = 0;
    
    // Blink colon every 500ms
    if (millis() - lastBlink >= 500) {
        colonBlink = !colonBlink;
        
        // Set decimal points for colons
        digitDP[1] = colonBlink; // After hours
        digitDP[3] = colonBlink; // After minutes
        
        lastBlink = millis();
    }
}

void refreshDisplay() {
    if (micros() - lastRefresh >= 2000) {
        digitalWrite(digitPins[currentDigit], HIGH);
        currentDigit = (currentDigit + 1) % 6;
        
        if (!display.isBusy()) {
            display.displayDigit(digits[currentDigit], digitDP[currentDigit]);
        }
        
        delayMicroseconds(10);
        digitalWrite(digitPins[currentDigit], LOW);
        lastRefresh = micros();
    }
}
```

### Adding Time Setting Buttons

```cpp
const uint8_t BUTTON_HOUR = 11;
const uint8_t BUTTON_MIN = 12;

void setup() {
    // ... previous setup ...
    
    pinMode(BUTTON_HOUR, INPUT_PULLUP);
    pinMode(BUTTON_MIN, INPUT_PULLUP);
}

void loop() {
    display.update();
    refreshDisplay();
    updateTime();
    updateColonBlink();
    checkButtons();
}

void checkButtons() {
    static unsigned long lastPress = 0;
    static bool hourPressed = false;
    static bool minPressed = false;
    
    // Debounce
    if (millis() - lastPress < 200) return;
    
    // Hour button
    if (digitalRead(BUTTON_HOUR) == LOW && !hourPressed) {
        hourPressed = true;
        adjustHour();
        lastPress = millis();
    } else if (digitalRead(BUTTON_HOUR) == HIGH) {
        hourPressed = false;
    }
    
    // Minute button
    if (digitalRead(BUTTON_MIN) == LOW && !minPressed) {
        minPressed = true;
        adjustMinute();
        lastPress = millis();
    } else if (digitalRead(BUTTON_MIN) == HIGH) {
        minPressed = false;
    }
}

void adjustHour() {
    DateTime now = rtc.now();
    uint8_t newHour = (now.hour() + 1) % 24;
    rtc.adjust(DateTime(now.year(), now.month(), now.day(), 
                        newHour, now.minute(), now.second()));
}

void adjustMinute() {
    DateTime now = rtc.now();
    uint8_t newMin = (now.minute() + 1) % 60;
    rtc.adjust(DateTime(now.year(), now.month(), now.day(), 
                        now.hour(), newMin, 0)); // Reset seconds
}
```

---

## Custom Animations

Create your own display animations.

### Scrolling Text

```cpp
#include <NJU3711_7Segment.h>

NJU3711_7Segment display(2, 3, 4, ACTIVE_LOW);

const char* message = "HELLO   "; // Add spaces for smooth scroll
int scrollPos = 0;
unsigned long lastScroll = 0;

void loop() {
    display.update();
    
    if (millis() - lastScroll >= 500) {
        if (!display.isBusy()) {
            char currentChar = message[scrollPos];
            display.displayChar(currentChar);
            
            scrollPos++;
            if (scrollPos >= strlen(message)) {
                scrollPos = 0;
            }
            
            lastScroll = millis();
        }
    }
}
```

### Custom Segment Animation

```cpp
void loop() {
    display.update();
    
    static int step = 0;
    static unsigned long lastUpdate = 0;
    
    if (millis() - lastUpdate >= 100) {
        if (!display.isBusy()) {
            // Create custom pattern
            uint8_t pattern = 0;
            
            // Rotating segments with trail
            for (int i = 0; i < 3; i++) {
                int segIdx = (step + i) % 6;
                uint8_t segments[] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F};
                pattern |= (1 << segments[segIdx]);
            }
            
            display.displayRaw(pattern);
            step++;
            lastUpdate = millis();
        }
    }
}
```

### Fading Effect (Simulated with PWM)

```cpp
void loop() {
    display.update();
    
    static int brightness = 0;
    static int direction = 1;
    static unsigned long lastUpdate = 0;
    
    if (millis() - lastUpdate >= 20) {
        // Use rapid blinking to simulate brightness
        int onTime = map(brightness, 0, 255, 0, 2000);
        
        if (!display.isBusy()) {
            if ((millis() % 2000) < onTime) {
                display.displayDigit(8);
            } else {
                display.displayBlank();
            }
        }
        
        brightness += direction;
        if (brightness >= 255 || brightness <= 0) {
            direction = -direction;
        }
        
        lastUpdate = millis();
    }
}
```

---

## Performance Optimization

### Queue Management

Monitor and optimize queue usage:

```cpp
void loop() {
    expander.update();
    
    // Check queue before adding operations
    if (expander.getQueueSize() < 6) {
        // Safe to add more operations
        expander.write(newData);
    } else {
        Serial.println("Queue nearly full!");
    }
    
    // Debug: Show queue status
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug >= 1000) {
        Serial.print("Queue size: ");
        Serial.println(expander.getQueueSize());
        lastDebug = millis();
    }
}
```

### Burst Operations

Send multiple operations efficiently:

```cpp
void sendBurst() {
    // Queue multiple operations at once
    for (int i = 0; i < 8 && !expander.isBusy(); i++) {
        expander.setBit(i);
        delay(1); // Small delay between queue adds
    }
}

void loop() {
    expander.update();
    
    static unsigned long lastBurst = 0;
    if (millis() - lastBurst >= 5000) {
        sendBurst();
        lastBurst = millis();
    }
}
```

### Minimize Update Calls in Complex Code

```cpp
void loop() {
    // Call update() once at the top
    expander.update();
    
    // Do all your work
    readSensors();
    processData();
    updateDisplay();
    
    // DON'T call update() multiple times unless needed
}
```

### Timing Optimization

```cpp
void setup() {
    expander.begin();
    
    // Optimize for your hardware
    expander.setStepDelay(1); // Fastest (5MHz capable)
    
    // Or slower for long wires / breadboard
    // expander.setStepDelay(5); // More conservative
}

void loop() {
    expander.update();
    
    // Measure performance
    unsigned long startTime = micros();
    expander.write(0xFF);
    while (expander.isBusy()) {
        expander.update();
    }
    unsigned long endTime = micros();
    
    Serial.print("Operation took: ");
    Serial.print(endTime - startTime);
    Serial.println(" microseconds");
}
```

### Memory Optimization

The library uses minimal memory, but here's how to check:

```cpp
void setup() {
    Serial.begin(9600);
    
    Serial.print("NJU3711 object size: ");
    Serial.println(sizeof(NJU3711));
    
    Serial.print("7Segment object size: ");
    Serial.println(sizeof(NJU3711_7Segment));
    
    Serial.print("Multi object size: ");
    Serial.println(sizeof(NJU3711_7Segment_Multi));
    
    // Check free RAM
    Serial.print("Free RAM: ");
    Serial.println(freeRam());
}

int freeRam() {
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
```

---

## Driving High-Power Loads

Use the NJU3711 to control relays, motors, or high-power LEDs.

### Driving Relays

```
NJU3711        Transistor      Relay
   │             ┌───┐
P1 ├──[1kΩ]── B │   │ C ──┬─── Relay Coil (+)
                E │   │     │
                └─┴─┘     [Diode]
                  │         │
                 GND    Relay Coil (-)
                            │
                           GND

Note: Diode (1N4148) protects against back-EMF
      Use NPN transistor (2N2222, 2N3904, etc.)
```

**Code:**
```cpp
#include <NJU3711.h>

NJU3711 expander(2, 3, 4);

const uint8_t RELAY1 = 0; // P1
const uint8_t RELAY2 = 1; // P2

void setup() {
    expander.begin();
}

void loop() {
    expander.update();
    
    if (!expander.isBusy()) {
        // Turn on relay 1
        expander.setBit(RELAY1);
        delay(2000);
        
        // Turn off relay 1, turn on relay 2
        expander.clearBit(RELAY1);
        expander.setBit(RELAY2);
        delay(2000);
        
        // Turn off relay 2
        expander.clearBit(RELAY2);
        delay(2000);
    }
}
```

### Driving High-Power LEDs

For LEDs requiring more than 25mA:

```
NJU3711        MOSFET        LED
   │           ┌────┐
P1 ├──[10kΩ]─ G│    │D ──[R]──▷|── +12V
               S│    │
               └────┘
                 │
                GND

R = Current limiting resistor
Use N-Channel MOSFET (2N7000, IRLZ44N, etc.)
10kΩ pull-down prevents floating gate
```

**Code:**
```cpp
void setup() {
    expander.begin();
}

void loop() {
    expander.update();
    
    if (!expander.isBusy()) {
        // PWM-like control
        for (int brightness = 0; brightness < 100; brightness++) {
            expander.setBit(0);
            delayMicroseconds(brightness * 10);
            expander.clearBit(0);
            delayMicroseconds((100 - brightness) * 10);
        }
    }
}
```

### Driving LED Strips (via MOSFET)

```cpp
#include <NJU3711.h>

NJU3711 expander(2, 3, 4);

// Each bit controls one LED strip color
const uint8_t RED_STRIP = 0;
const uint8_t GREEN_STRIP = 1;
const uint8_t BLUE_STRIP = 2;

void loop() {
    expander.update();
    
    if (!expander.isBusy()) {
        // Color cycling
        setColor(255, 0, 0);     // Red
        delay(1000);
        setColor(0, 255, 0);     // Green
        delay(1000);
        setColor(0, 0, 255);     // Blue
        delay(1000);
        setColor(255, 255, 255); // White
        delay(1000);
    }
}

void setColor(uint8_t r, uint8_t g, uint8_t b) {
    if (r > 127) expander.setBit(RED_STRIP);
    else expander.clearBit(RED_STRIP);
    
    if (g > 127) expander.setBit(GREEN_STRIP);
    else expander.clearBit(GREEN_STRIP);
    
    if (b > 127) expander.setBit(BLUE_STRIP);
    else expander.clearBit(BLUE_STRIP);
}
```

---

## Advanced Multiplexing Techniques

### Brightness Control via Duty Cycle

```cpp
#include <NJU3711_7Segment_Multi.h>

NJU3711_7Segment_Multi display(2, 3, 4, 5, 6, 7, ACTIVE_LOW);

uint8_t brightness = 50; // 0-100%

void loop() {
    display.update();
    
    // Adjust multiplexing for brightness
    unsigned long delayTime = map(brightness, 0, 100, 500, 3000);
    display.setMultiplexDelay(delayTime);
    
    // Your code...
}

void increaseBrightness() {
    brightness = min(100, brightness + 10);
}

void decreaseBrightness() {
    brightness = max(0, brightness - 10);
}
```

### Adaptive Refresh Rate

Adjust refresh based on content:

```cpp
void loop() {
    display.update();
    
    // Count lit segments
    int litSegments = countLitSegments();
    
    // More segments = faster refresh needed
    if (litSegments > 18) {
        display.setMultiplexDelay(1500); // Fast
    } else if (litSegments > 12) {
        display.setMultiplexDelay(2000); // Normal
    } else {
        display.setMultiplexDelay(2500); // Slow (saves power)
    }
}

int countLitSegments() {
    int count = 0;
    for (int i = 0; i < 3; i++) {
        uint8_t segments = getDigitSegments(i);
        for (int bit = 0; bit < 8; bit++) {
            if (segments & (1 << bit)) count++;
        }
    }
    return count;
}
```

### Power-Saving Mode

```cpp
unsigned long lastActivity = 0;
bool powerSaveMode = false;

void loop() {
    display.update();
    
    // Enter power save after 30 seconds of inactivity
    if (millis() - lastActivity > 30000 && !powerSaveMode) {
        enterPowerSave();
    }
    
    // Check for activity (button press, sensor, etc.)
    if (activityDetected()) {
        exitPowerSave();
        lastActivity = millis();
    }
}

void enterPowerSave() {
    powerSaveMode = true;
    display.disableAllDigits();
    // Or dim display
    // display.setMultiplexDelay(5000); // Very slow = very dim
}

void exitPowerSave() {
    powerSaveMode = false;
    display.enableAllDigits();
    display.setMultiplexDelay(2000); // Normal
}
```

---

## Real-World Applications

### Voltmeter (0-99.9V)

```cpp
#include <NJU3711_7Segment_Multi.h>

NJU3711_7Segment_Multi display(2, 3, 4, 5, 6, 7, ACTIVE_LOW);

const int VOLTAGE_INPUT = A0;
const float VOLTAGE_REF = 5.0;
const float VOLTAGE_DIVIDER = 20.0; // 100kΩ / 5kΩ divider

void setup() {
    display.begin();
    analogReference(EXTERNAL); // Use external AREF if needed
}

void loop() {
    display.update();
    
    static unsigned long lastRead = 0;
    if (millis() - lastRead >= 100) {
        // Read voltage (average of 10 samples)
        float voltage = 0;
        for (int i = 0; i < 10; i++) {
            int raw = analogRead(VOLTAGE_INPUT);
            voltage += (raw / 1023.0) * VOLTAGE_REF * VOLTAGE_DIVIDER;
            delay(1);
        }
        voltage /= 10.0;
        
        // Display as XX.X format
        int displayValue = (int)(voltage * 10);
        display.displayNumber(displayValue, 2); // DP at position 2
        
        lastRead = millis();
    }
}
```

### RPM Counter

```cpp
#include <NJU3711_7Segment_Multi.h>

NJU3711_7Segment_Multi display(2, 3, 4, 5, 6, 7, ACTIVE_LOW);

const int SENSOR_PIN = 2; // Interrupt pin
volatile unsigned long pulseCount = 0;
unsigned long lastCalculation = 0;
int rpm = 0;

void setup() {
    display.begin();
    pinMode(SENSOR_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), countPulse, FALLING);
}

void loop() {
    display.update();
    
    // Calculate RPM every second
    if (millis() - lastCalculation >= 1000) {
        noInterrupts();
        unsigned long pulses = pulseCount;
        pulseCount = 0;
        interrupts();
        
        // Convert pulses to RPM (adjust for your sensor)
        rpm = pulses * 60; // If 1 pulse per revolution
        
        display.displayNumber(rpm);
        lastCalculation = millis();
    }
}

void countPulse() {
    pulseCount++;
}
```

### Temperature Display with Alarm

```cpp
#include <NJU3711_7Segment_Multi.h>
#include <DHT.h>

#define DHTPIN 8
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
NJU3711_7Segment_Multi display(2, 3, 4, 5, 6, 7, ACTIVE_LOW);

const int ALARM_TEMP = 30; // °C
bool alarmActive = false;

void setup() {
    display.begin();
    dht.begin();
}

void loop() {
    display.update();
    
    static unsigned long lastRead = 0;
    if (millis() - lastRead >= 2000) {
        float temp = dht.readTemperature();
        
        if (!isnan(temp)) {
            int displayTemp = (int)temp;
            display.displayTemperature(displayTemp);
            
            // Check alarm
            if (displayTemp >= ALARM_TEMP && !alarmActive) {
                alarmActive = true;
                startAlarmAnimation();
            } else if (displayTemp < ALARM_TEMP) {
                alarmActive = false;
            }
        } else {
            display.displayError();
        }
        
        lastRead = millis();
    }
    
    if (alarmActive) {
        handleAlarm();
    }
}

void startAlarmAnimation() {
    // Flash display
}

void handleAlarm() {
    static unsigned long lastBlink = 0;
    static bool blinkState = false;
    
    if (millis() - lastBlink >= 500) {
        if (blinkState) {
            display.enableAllDigits();
        } else {
            display.disableAllDigits();
        }
        blinkState = !blinkState;
        lastBlink = millis();
    }
}
```

### Stopwatch with Lap Times

```cpp
#include <NJU3711_7Segment_Multi.h>

NJU3711_7Segment_Multi display(2, 3, 4, 5, 6, 7, ACTIVE_LOW);

const int BUTTON_START = 11;
const int BUTTON_LAP = 12;

unsigned long startTime = 0;
unsigned long elapsedTime = 0;
bool running = false;
bool showingLap = false;
unsigned long lapTime = 0;

void setup() {
    display.begin();
    pinMode(BUTTON_START, INPUT_PULLUP);
    pinMode(BUTTON_LAP, INPUT_PULLUP);
}

void loop() {
    display.update();
    checkButtons();
    updateDisplay();
}

void checkButtons() {
    static bool startPressed = false;
    static bool lapPressed = false;
    static unsigned long lastPress = 0;
    
    if (millis() - lastPress < 200) return; // Debounce
    
    // Start/Stop button
    if (digitalRead(BUTTON_START) == LOW && !startPressed) {
        startPressed = true;
        toggleTimer();
        lastPress = millis();
    } else if (digitalRead(BUTTON_START) == HIGH) {
        startPressed = false;
    }
    
    // Lap button
    if (digitalRead(BUTTON_LAP) == LOW && !lapPressed && running) {
        lapPressed = true;
        recordLap();
        lastPress = millis();
    } else if (digitalRead(BUTTON_LAP) == HIGH) {
        lapPressed = false;
    }
}

void toggleTimer() {
    if (running) {
        // Stop
        running = false;
        elapsedTime = millis() - startTime;
    } else {
        // Start
        running = true;
        startTime = millis() - elapsedTime;
        showingLap = false;
    }
}

void recordLap() {
    lapTime = millis() - startTime;
    showingLap = true;
}

void updateDisplay() {
    unsigned long displayTime;
    
    if (showingLap) {
        displayTime = lapTime;
    } else if (running) {
        displayTime = millis() - startTime;
    } else {
        displayTime = elapsedTime;
    }
    
    // Format as MM:SS.x (minutes:seconds.tenths)
    int minutes = (displayTime / 60000) % 100;
    int seconds = (displayTime / 1000) % 60;
    int tenths = (displayTime / 100) % 10;
    
    int displayValue = minutes * 1000 + seconds * 10 + tenths;
    display.displayNumber(displayValue);
    
    // Set decimal points for formatting
    display.setDecimalPoint(1, true); // After minutes
    display.setDecimalPoint(2, true); // After seconds
}
```

---

## Performance Benchmarking

Test your setup's performance:

```cpp
void benchmarkOperations() {
    Serial.println("\n=== Performance Benchmark ===");
    
    // Test 1: Single write operation
    unsigned long start = micros();
    expander.write(0xFF);
    while (expander.isBusy()) {
        expander.update();
    }
    unsigned long duration = micros() - start;
    Serial.print("Single write: ");
    Serial.print(duration);
    Serial.println(" µs");
    
    // Test 2: Queue throughput
    start = millis();
    int operations = 0;
    while (millis() - start < 1000) {
        expander.update();
        if (!expander.isBusy()) {
            expander.write(random(256));
            operations++;
        }
    }
    Serial.print("Operations per second: ");
    Serial.println(operations);
    
    // Test 3: Bit manipulation speed
    start = micros();
    for (int i = 0; i < 8; i++) {
        expander.setBit(i);
        while (expander.isBusy()) expander.update();
    }
    duration = micros() - start;
    Serial.print("8 bit operations: ");
    Serial.print(duration);
    Serial.println(" µs");
    
    Serial.println("=== Benchmark Complete ===\n");
}
```

---

## Tips and Best Practices

### 1. Always Call `update()`

```cpp
// GOOD
void loop() {
    display.update(); // First thing!
    // Your code...
}

// BAD
void loop() {
    // Your code...
    display.update(); // Too late!
}
```

### 2. Check Before Queueing

```cpp
// GOOD
if (!expander.isBusy()) {
    expander.write(newData);
}

// BAD
expander.write(newData); // Might fail if busy!
```

### 3. Use Appropriate Timing

```cpp
// For breadboard/long wires
expander.setStepDelay(5);

// For PCB/short traces
expander.setStepDelay(1);
```

### 4. Monitor Queue Size

```cpp
if (expander.getQueueSize() > 6) {
    Serial.println("Warning: Queue filling up!");
}
```

### 5. Handle Errors Gracefully

```cpp
if (!display.displayDigit(value)) {
    Serial.println("Failed to queue display operation");
    // Take corrective action
}
```

---

## See Also

- [API Reference](API_Reference.md) - Complete method documentation
- [Hardware Guide](Hardware_Guide.md) - Wiring and electrical specifications
- [Getting Started](Getting_Started.md) - Beginner tutorials
- [Troubleshooting](Troubleshooting.md) - Common issues and solutions