/*
 * NJU3711_Basic.ino - Basic Example
 * 
 * This example demonstrates basic usage of the NJU3711 library
 * for controlling 8 parallel outputs from 3 serial pins.
 * 
 * Wiring (CLR pin strapped HIGH on PCB):
 * Arduino → NJU3711
 * Pin 2   → DATA (pin 8)
 * Pin 3   → CLK (pin 9)
 * Pin 4   → STB (pin 10)
 * 5V      → VDD (pin 14) and CLR (pin 11)
 * GND     → VSS (pin 4)
 * 
 * Connect LEDs with current-limiting resistors to P1-P8
 * (pins 12,13,1,2,3,5,6,7 respectively)
 * 
 * Author: justdienow
 * Version: 1.0
 * 
 */

#include <NJU3711.h>

// Create NJU3711 instance - CLR pin strapped HIGH
NJU3711 expander(2, 3, 4); // DATA, CLK, STB pins

unsigned long lastUpdate = 0;
uint8_t counter = 0;
int currentPattern = 0;

void setup() {
    Serial.begin(9600);
    Serial.println("NJU3711 Basic Example");
    Serial.println("====================");
    
    // Initialize the expander
    expander.begin();
    
    Serial.println("Commands:");
    Serial.println("1 - Binary counter");
    Serial.println("2 - Walking bit pattern");
    Serial.println("3 - Alternating pattern");
    Serial.println("4 - Individual bit control demo");
    Serial.println("c - Clear all outputs");
    Serial.println("f - Fill all outputs");
    Serial.println();
}

void loop() {
    // CRITICAL: Must call update() regularly for non-blocking operation
    expander.update();
    
    // Handle serial commands
    handleCommands();
    
    // Run current pattern
    runCurrentPattern();
    
    // Show status every 5 seconds
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 5000) {
        Serial.print("Current data: 0b");
        Serial.print(expander.getCurrentData(), BIN);
        Serial.print(" (0x");
        Serial.print(expander.getCurrentData(), HEX);
        Serial.println(")");
        lastStatus = millis();
    }
}

void handleCommands() {
    if (Serial.available()) {
        char cmd = Serial.read();
        
        switch (cmd) {
            case '1':
                currentPattern = 1;
                counter = 0;
                Serial.println("Starting binary counter pattern");
                break;
                
            case '2':
                currentPattern = 2;
                counter = 0;
                Serial.println("Starting walking bit pattern");
                break;
                
            case '3':
                currentPattern = 3;
                counter = 0;
                Serial.println("Starting alternating pattern");
                break;
                
            case '4':
                currentPattern = 4;
                counter = 0;
                Serial.println("Starting individual bit control demo");
                break;
                
            case 'c':
            case 'C':
                expander.clear();
                currentPattern = 0;
                Serial.println("Cleared all outputs");
                break;
                
            case 'f':
            case 'F':
                expander.write(0xFF);
                currentPattern = 0;
                Serial.println("Set all outputs HIGH");
                break;
        }
        
        // Clear remaining input
        while (Serial.available()) Serial.read();
    }
}

void runCurrentPattern() {
    // Don't interfere if device is busy
    if (expander.isBusy()) return;
    
    // Update patterns every 200ms
    if (millis() - lastUpdate < 200) return;
    
    switch (currentPattern) {
        case 1: // Binary counter
            expander.write(counter);
            counter++;
            break;
            
        case 2: // Walking bit
            expander.write(1 << (counter % 8));
            counter++;
            break;
            
        case 3: // Alternating pattern
            if (counter % 2 == 0) {
                expander.write(0xAA); // 10101010
            } else {
                expander.write(0x55); // 01010101
            }
            counter++;
            break;
            
        case 4: // Individual bit control
            {
                static bool bitState = true;
                int bitPos = counter % 8;
                
                if (bitPos == 0) {
                    expander.clear(); // Start fresh
                    bitState = !bitState;
                }
                
                expander.writeBit(bitPos, bitState);
                counter++;
            }
            break;
    }
    
    lastUpdate = millis();
}

// Demonstrate all basic functions
void demonstrateAllFunctions() {
    Serial.println("\n=== Function Demonstration ===");
    
    // Test individual bits
    Serial.println("Setting individual bits...");
    expander.clear();
    delay(500);
    
    for (int i = 0; i < 8; i++) {
        while (expander.isBusy()) expander.update();
        expander.setBit(i);
        Serial.print("Set bit ");
        Serial.println(i);
        delay(300);
    }
    
    delay(1000);
    
    // Clear individual bits
    Serial.println("Clearing individual bits...");
    for (int i = 0; i < 8; i++) {
        while (expander.isBusy()) expander.update();
        expander.clearBit(i);
        Serial.print("Cleared bit ");
        Serial.println(i);
        delay(300);
    }
    
    delay(500);
    
    // Toggle bits
    Serial.println("Toggling bits...");
    for (int cycle = 0; cycle < 3; cycle++) {
        for (int i = 0; i < 8; i++) {
            while (expander.isBusy()) expander.update();
            expander.toggleBit(i);
            delay(100);
        }
    }
    
    // Final clear
    while (expander.isBusy()) expander.update();
    expander.clear();
    Serial.println("Demonstration complete!");
}