/*
 * NJU3711_7Segment_Multi_Demo.ino
 * Example showing how to use the NJU3711 to drive 3 multiplexed 7-segment displays
 * 
 * This demo shows how to display numbers from 0-999 on three 7-segment displays
 * using high-side PNP transistor switching.
 * 
 * Wiring:
 * Arduino -> NJU3711
 * Pin 2   -> DATA (NJU3711 pin 8)
 * Pin 3   -> CLK  (NJU3711 pin 9)
 * Pin 4   -> STB  (NJU3711 pin 10)
 * 
 * Arduino -> Digit Control Transistors (PNP)
 * Pin 5   -> TR1 Base (Digit 1 - rightmost/ones)
 * Pin 6   -> TR2 Base (Digit 2 - middle/tens)
 * Pin 7   -> TR3 Base (Digit 3 - leftmost/hundreds)
 * 
 * Transistor connections:
 * - Emitters to +5V
 * - Collectors to common anodes of each 7-segment display
 * - Base resistors already on PCB
 * 
 * NJU3711 outputs to 7-segment cathodes through current-limiting resistors
 */

#include "NJU3711_7Segment_Multi.h"

// Pin definitions
#define DATA_PIN   2
#define CLOCK_PIN  3
#define STROBE_PIN 4
#define DIGIT1_PIN 5  // Rightmost digit (ones)
#define DIGIT2_PIN 6  // Middle digit (tens)
#define DIGIT3_PIN 7  // Leftmost digit (hundreds)

// Create multi-digit display object
NJU3711_7Segment_Multi display(DATA_PIN, CLOCK_PIN, STROBE_PIN, 
                               DIGIT1_PIN, DIGIT2_PIN, DIGIT3_PIN,
                               ACTIVE_LOW);

// Demo variables
unsigned long lastCountUpdate = 0;
uint16_t counter = 0;
int demoMode = 0;

void setup() {
    Serial.begin(9600);
    Serial.println("NJU3711 Multi-Digit Display Demo");
    Serial.println("=================================");
    
    // Initialize the display
    display.begin();
    display.setStepDelay(1);          // Fast timing for NJU3711
    display.setMultiplexDelay(2000);  // 2ms between digit switches
    display.setBlankingTime(50);      // 50us blanking time (increase if ghosting)
    
    // Test pattern - all segments on all digits
    display.displayAll();
    delay(1000);
    
    // Clear and start
    display.clearDisplay();
    delay(500);
    
    Serial.println("\nCommands:");
    Serial.println("1 - Count up 0-999");
    Serial.println("2 - Count down 999-0");
    Serial.println("3 - Display specific number (send 3 digits after command)");
    Serial.println("4 - Leading zeros ON/OFF toggle");
    Serial.println("5 - Decimal point demo");
    Serial.println("6 - Temperature display demo");
    Serial.println("7 - Error display");
    Serial.println("8 - Individual digit control demo");
    Serial.println("9 - Multiplexing speed test");
    Serial.println("0 - Test pattern (all segments)");
    Serial.println("b - Adjust blanking time (for ghosting issues)");
    Serial.println("c - Clear display");
    Serial.println();
    
    // Start with counting demo
    demoMode = 1;
}

void loop() {
    // CRITICAL: Must call update() frequently for multiplexing
    display.update();
    
    // Handle serial commands
    handleSerialCommands();
    
    // Run current demo
    runDemo();
}

void handleSerialCommands() {
    if (Serial.available()) {
        char cmd = Serial.read();
        
        switch (cmd) {
            case '1':
                demoMode = 1;
                counter = 0;
                Serial.println("Demo: Counting up 0-999");
                break;
                
            case '2':
                demoMode = 2;
                counter = 999;
                Serial.println("Demo: Counting down 999-0");
                break;
                
            case '3': {
                Serial.println("Enter 3-digit number (000-999):");
                while (Serial.available() < 3) {
                    display.update(); // Keep multiplexing while waiting
                }
                int hundreds = Serial.read() - '0';
                int tens = Serial.read() - '0';
                int ones = Serial.read() - '0';
                uint16_t number = hundreds * 100 + tens * 10 + ones;
                display.displayNumber(number);
                Serial.print("Displaying: ");
                Serial.println(number);
                demoMode = 0; // Stop auto demos
                break;
            }
                
            case '4': {
                static bool leadingZeros = false;
                leadingZeros = !leadingZeros;
                display.setLeadingZeros(leadingZeros);
                Serial.print("Leading zeros: ");
                Serial.println(leadingZeros ? "ON" : "OFF");
                break;
            }
                
            case '5':
                demoMode = 5;
                Serial.println("Demo: Decimal point positions");
                break;
                
            case '6':
                demoMode = 6;
                Serial.println("Demo: Temperature display");
                break;
                
            case '7':
                display.displayError();
                Serial.println("Displaying: Err");
                demoMode = 0;
                break;
                
            case '8':
                demoMode = 8;
                Serial.println("Demo: Individual digit control");
                break;
                
            case '9':
                demoMode = 9;
                Serial.println("Demo: Multiplexing speed test");
                break;
                
            case '0':
                display.displayAll();
                Serial.println("Test pattern: All segments ON");
                demoMode = 0;
                break;
                
            case 'b':
            case 'B': {
                static unsigned long blankingTime = 50;
                blankingTime += 50;
                if (blankingTime > 500) blankingTime = 50;
                display.setBlankingTime(blankingTime);
                Serial.print("Blanking time set to: ");
                Serial.print(blankingTime);
                Serial.println(" microseconds");
                Serial.println("(Increase if seeing ghosting between digits)");
                break;
            }
            
            case 'c':
            case 'C':
                display.clearDisplay();
                Serial.println("Display cleared");
                demoMode = 0;
                break;
        }
        
        // Clear any remaining serial data
        while (Serial.available()) Serial.read();
    }
}

void runDemo() {
    switch (demoMode) {
        case 1: // Count up
            if (millis() - lastCountUpdate >= 100) { // Update every 100ms
                display.displayNumber(counter);
                counter++;
                if (counter > 999) counter = 0;
                lastCountUpdate = millis();
            }
            break;
            
        case 2: // Count down
            if (millis() - lastCountUpdate >= 100) {
                display.displayNumber(counter);
                if (counter > 0) {
                    counter--;
                } else {
                    counter = 999;
                }
                lastCountUpdate = millis();
            }
            break;
            
        case 5: // Decimal point demo
            if (millis() - lastCountUpdate >= 1000) {
                static uint8_t dpPosition = 1;
                display.displayNumber(123, dpPosition);
                Serial.print("Number 123 with DP at position ");
                Serial.println(dpPosition);
                dpPosition++;
                if (dpPosition > 3) dpPosition = 1;
                lastCountUpdate = millis();
            }
            break;
            
        case 6: { // Temperature demo
            static int16_t temp = -20;
            if (millis() - lastCountUpdate >= 500) {
                display.displayTemperature(temp, true);
                Serial.print("Temperature: ");
                Serial.print(temp);
                Serial.println("°C");
                temp += 5;
                if (temp > 40) temp = -20;
                lastCountUpdate = millis();
            }
            break;
        }
            
        case 8: { // Individual digit control
            static uint8_t digitPos = 0;
            static uint8_t digitVal = 0;
            if (millis() - lastCountUpdate >= 250) {
                display.clearDisplay();
                display.setDigit(digitPos, digitVal, false);
                digitVal++;
                if (digitVal > 9) {
                    digitVal = 0;
                    digitPos++;
                    if (digitPos > 2) digitPos = 0;
                }
                lastCountUpdate = millis();
            }
            break;
        }
            
        case 9: { // Multiplexing speed test
            static unsigned long multiplexDelay = 500;
            static bool increasing = true;
            
            display.displayNumber(888); // Show all 8s
            
            if (millis() - lastCountUpdate >= 100) {
                display.setMultiplexDelay(multiplexDelay);
                Serial.print("Multiplex delay: ");
                Serial.print(multiplexDelay);
                Serial.println(" microseconds");
                
                if (increasing) {
                    multiplexDelay += 500;
                    if (multiplexDelay >= 10000) {
                        increasing = false;
                    }
                } else {
                    multiplexDelay -= 500;
                    if (multiplexDelay <= 500) {
                        increasing = true;
                        demoMode = 1; // Go back to counting
                        Serial.println("Speed test complete, returning to counting...");
                    }
                }
                lastCountUpdate = millis();
            }
            break;
        }
    }
}

// Example functions for specific use cases

void displayVoltage(float voltage) {
    // Display voltage like "12.5" - would need 4 digits for full implementation
    // For 3 digits, display as "125" meaning 12.5V
    uint16_t value = (uint16_t)(voltage * 10);
    if (value > 999) value = 999;
    display.displayNumber(value, 2); // Put decimal point at position 2
}

void displayRPM(uint16_t rpm) {
    // Display RPM values
    if (rpm > 999) {
        // For values over 999, you might want to show "---" or scale down
        display.displayDashes();
    } else {
        display.displayNumber(rpm);
    }
}

void displayPercentage(uint8_t percent) {
    // Display percentage (0-100)
    if (percent > 100) percent = 100;
    display.displayNumber(percent);
    // Could add 'P' on a 4th digit if available
}

// Test function to verify each digit works independently
void testDigits() {
    Serial.println("Testing each digit independently...");
    
    // Disable multiplexing temporarily
    display.disableMultiplex();
    
    // Test each digit
    for (int digit = 0; digit < 3; digit++) {
        Serial.print("Testing digit ");
        Serial.println(digit + 1);
        
        // Clear all
        display.deselectAllDigits();
        
        // Show '8' on this digit only
        display.displayRaw(0xFF, true); // All segments + DP
        
        // Turn on only this digit
        digitalWrite(digit == 0 ? DIGIT1_PIN : 
                    digit == 1 ? DIGIT2_PIN : DIGIT3_PIN, LOW);
        
        delay(1000);
        
        // Turn off
        digitalWrite(digit == 0 ? DIGIT1_PIN : 
                    digit == 1 ? DIGIT2_PIN : DIGIT3_PIN, HIGH);
    }
    
    // Re-enable multiplexing
    display.enableMultiplex();
    Serial.println("Digit test complete");
}

// Function to demonstrate smooth brightness control (PWM-like effect)
// by varying the multiplex delay
void demonstrateBrightness() {
    Serial.println("Demonstrating brightness control...");
    
    display.displayNumber(888); // Show all 8s
    
    // Vary multiplex timing to simulate brightness
    for (int i = 0; i < 50; i++) {
        unsigned long delayTime = map(i, 0, 50, 500, 5000);
        display.setMultiplexDelay(delayTime);
        
        // Run the display update many times to see the effect
        for (int j = 0; j < 100; j++) {
            display.update();
            delayMicroseconds(100);
        }
    }
    
    // Return to normal
    display.setMultiplexDelay(2000);
}