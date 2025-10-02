/*
 * NJU3711_7Segment_Single_Demo.ino
 * Example Arduino sketch showing how to use the NJU3711 7-segment display driver
 * 
 * Wiring (CLR pin strapped HIGH on PCB):
 * Arduino -> NJU3711 -> 7-Segment Display
 * Pin 2   -> DATA
 * Pin 3   -> CLK 
 * Pin 4   -> STB 
 * 5V      -> VDD 
 * GND     -> VSS 
 * 
 * This class extends the NJU3711 driver to provide specialised functions
 * for controlling 7-segment LED displays.
 * 
 * NOTE: LEDs are ON when port is LOW, OFF when port is HIGH
 * 
 * Pin Mapping (P1-P8 to 7-segment display):
 * 
 * Standard 7-segment layout:
 *     AAA
 *    F   B
 *    F   B
 *     GGG
 *    E   C
 *    E   C
 * DP  DDD 
 * 
 * Bit Position: 7   6   5   4   3   2   1   0
 * Shifts to   : P8  P7  P6  P5  P4  P3  P2  P1
 * Segment     : A   F   B   G   E   C   D   DP
 *
 * Segment A=P8(bit7)
 * Segment B=P6(bit5)
 * Segment C=P3(bit2)
 * Segment D=P2(bit1)
 * Segment E=P4(bit3)
 * Segment F=P7(bit6)
 * Segment G=P5(bit4)
 * Decimal DP=P1(bit0)
 * 
 * NOTE: LEDs are ON when NJU3711 output is LOW, OFF when HIGH
 * Connect current-limiting resistors (typically 220-470Ω) between 
 * 7-segment display pins and +5V (since LEDs are active LOW).
 * 
 * Author: justdienow
 * Version: 1.0
 * 
 */

#include "NJU3711_7Segment.h"

// Create 7-segment display driver
// Using ACTIVE_LOW since your LEDs turn on when port is LOW
NJU3711_7Segment display(2, 3, 4, ACTIVE_LOW);

// Example state variables
unsigned long lastUpdate = 0;
int currentDemo = 1;
int demoStep = 0;
bool demoDirection = true;

void setup() {
    Serial.begin(9600);
    Serial.println("NJU3711 7-Segment Display Example");
    Serial.println("==================================");
    
    // Initialize the display
    display.begin();
    display.setStepDelay(1); // Fast timing
    
    // Start with a test pattern
    display.displayAll();
    delay(1000);
    display.displayBlank();
    delay(500);
    
    Serial.println("Commands:");
    Serial.println("1 - Digit counter (0-9)");
    Serial.println("2 - Hex counter (0-F)");
    Serial.println("3 - Character display");
    Serial.println("4 - Animation: Rotate clockwise");
    Serial.println("5 - Animation: Rotate counter-clockwise");
    Serial.println("6 - Animation: Loading bar");
    Serial.println("7 - Animation: Chase");
    Serial.println("8 - Individual segment test");
    Serial.println("9 - Special characters");
    Serial.println("0 - Display test pattern");
    Serial.println("c - Clear display");
    Serial.println("s - Stop animations");
    Serial.println("d - Toggle decimal point");
    Serial.println();
    
    // Start with digit counter
    currentDemo = 1;
    lastUpdate = millis();
}

void loop() {
    // CRITICAL: Must call update() regularly for non-blocking operation
    display.update();
    
    // Handle serial commands
    handleSerialInput();
    
    // Run current demo
    runCurrentDemo();
    
    // Show status occasionally
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 10000) { // Every 10 seconds
        Serial.print("Status: Demo ");
        Serial.print(currentDemo);
        Serial.print(", Busy: ");
        Serial.print(display.isBusy() ? "Yes" : "No");
        Serial.print(", Animating: ");
        Serial.println(display.isAnimating() ? "Yes" : "No");
        lastStatus = millis();
    }
}

void handleSerialInput() {
    if (Serial.available()) {
        char cmd = Serial.read();
        
        switch (cmd) {
            case '1':
                currentDemo = 1;
                demoStep = 0;
                display.stopAnimation();
                Serial.println("Demo 1: Digit counter (0-9)");
                break;
                
            case '2':
                currentDemo = 2;
                demoStep = 0;
                display.stopAnimation();
                Serial.println("Demo 2: Hex counter (0-F)");
                break;
                
            case '3':
                currentDemo = 3;
                demoStep = 0;
                display.stopAnimation();
                Serial.println("Demo 3: Character display");
                break;
                
            case '4':
                display.startAnimation(ANIM_ROTATE_CW, 200000); // 200ms
                Serial.println("Animation: Rotate clockwise");
                break;
                
            case '5':
                display.startAnimation(ANIM_ROTATE_CCW, 200000); // 200ms
                Serial.println("Animation: Rotate counter-clockwise");
                break;
                
            case '6':
                display.startAnimation(ANIM_LOADING, 150000); // 150ms
                Serial.println("Animation: Loading bar");
                break;
                
            case '7':
                display.startAnimation(ANIM_CHASE, 100000); // 100ms
                Serial.println("Animation: Chase");
                break;
                
            case '8':
                currentDemo = 8;
                demoStep = 0;
                display.stopAnimation();
                Serial.println("Demo 8: Individual segment test");
                break;
                
            case '9':
                currentDemo = 9;
                demoStep = 0;
                display.stopAnimation();
                Serial.println("Demo 9: Special characters");
                break;
                
            case '0':
                display.test();
                Serial.println("Running test pattern");
                break;
                
            case 'c':
            case 'C':
                display.displayBlank();
                Serial.println("Display cleared");
                break;
                
            case 's':
            case 'S':
                display.stopAnimation();
                Serial.println("Animations stopped");
                break;
                
            case 'd':
            case 'D':
                display.toggleDecimalPoint();
                Serial.print("Decimal point: ");
                Serial.println(display.getDecimalPointState() ? "ON" : "OFF");
                break;
        }
        
        // Clear any remaining characters
        while (Serial.available()) Serial.read();
    }
}

void runCurrentDemo() {
    if (display.isAnimating()) return; // Don't interfere with animations
    
    // Update demos every 500ms
    if (millis() - lastUpdate < 500) return;
    
    if (!display.isBusy()) {
        switch (currentDemo) {
            case 1: // Digit counter
                display.displayDigit(demoStep % 10);
                demoStep++;
                break;
                
            case 2: // Hex counter
                display.displayHex(demoStep % 16);
                demoStep++;
                break;
                
            case 3: { // Character display
                const char chars[] = "0123456789ABCDEF-_ ";
                display.displayChar(chars[demoStep % strlen(chars)]);
                demoStep++;
                break;
            }
            
            case 8: { // Individual segment test
                display.displayBlank();
                delay(100);
                display.setSegment(demoStep % 7, true); // Don't include DP in auto test
                demoStep++;
                break;
            }
            
            case 9: { // Special characters
                switch (demoStep % 6) {
                    case 0: display.displayMinus(); break;
                    case 1: display.displayUnderscore(); break;
                    case 2: display.displayDegree(); break;
                    case 3: display.displayError(); break;
                    case 4: display.displayAll(); break;
                    case 5: display.displayBlank(); break;
                }
                demoStep++;
                break;
            }
        }
        lastUpdate = millis();
    }
}

// Additional utility functions for advanced demos
void demonstrateCountdown() {
    Serial.println("Countdown from 9 to 0:");
    for (int i = 9; i >= 0; i--) {
        while (display.isBusy()) {
            display.update();
        }
        display.displayDigit(i);
        delay(1000);
    }
    display.displayBlank();
}

void demonstrateAllDigits() {
    Serial.println("Displaying all digits 0-9:");
    for (int i = 0; i <= 9; i++) {
        while (display.isBusy()) {
            display.update();
        }
        display.displayDigit(i, i % 2 == 0); // Show DP on even numbers
        delay(800);
    }
}

void demonstrateAllHex() {
    Serial.println("Displaying all hex values 0-F:");
    for (int i = 0; i <= 15; i++) {
        while (display.isBusy()) {
            display.update();
        }
        display.displayHex(i);
        delay(600);
    }
}

void demonstrateSegmentControl() {
    Serial.println("Individual segment control:");
    
    // Turn on segments one by one
    display.displayBlank();
    delay(500);
    
    const char* segmentNames[] = {"A", "B", "C", "D", "E", "F", "G", "DP"};
    
    for (int i = 0; i < 8; i++) {
        while (display.isBusy()) {
            display.update();
        }
        display.setSegment(i, true);
        Serial.print("Segment ");
        Serial.print(segmentNames[i]);
        Serial.println(" ON");
        delay(500);
    }
    
    delay(1000);
    
    // Turn off segments one by one
    for (int i = 0; i < 8; i++) {
        while (display.isBusy()) {
            display.update();
        }
        display.setSegment(i, false);
        Serial.print("Segment ");
        Serial.print(segmentNames[i]);
        Serial.println(" OFF");
        delay(500);
    }
}