/*
 * NJU3711_7Segment_Cascade_Demo.ino - Cascaded 6-Digit Display Demo
 * 
 * Hardware Setup:
 * - Two 3-digit display PCBs
 * - Shared control bus: DATA, CLK, STB connected together
 * - 6 individual digit select pins (one per digit)
 * 
 * Pin Assignments:
 * Arduino Pin 2  -> DATA (shared)
 * Arduino Pin 3  -> CLK (shared)
 * Arduino Pin 4  -> STB (shared)
 * Arduino Pin 5  -> Digit 1 select (leftmost)
 * Arduino Pin 6  -> Digit 2 select
 * Arduino Pin 7  -> Digit 3 select
 * Arduino Pin 8  -> Digit 4 select
 * Arduino Pin 9  -> Digit 5 select
 * Arduino Pin 10 -> Digit 6 select (rightmost)
 * 
 * Display Configuration:
 * [1] [2] [3]    [4] [5] [6]
 *  Left PCB       Right PCB
 * 
 * The displays use multiplexing - only one digit is lit at a time,
 * but refreshed fast enough that they all appear lit due to persistence of vision.
 * 
 * Author: justdienow
 * Version: 1.0
 * 
 */

#include "NJU3711_7Segment.h"

// Control pins (shared bus)
#define DATA_PIN  2
#define CLK_PIN   3
#define STB_PIN   4

// Digit select pins
#define DIGIT_1   5  // Leftmost digit
#define DIGIT_2   6
#define DIGIT_3   7
#define DIGIT_4   8
#define DIGIT_5   9
#define DIGIT_6   10 // Rightmost digit

// Create display driver instance
NJU3711_7Segment display(DATA_PIN, CLK_PIN, STB_PIN, ACTIVE_LOW);

// Array to hold digit select pins
const uint8_t digitPins[6] = {DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4, DIGIT_5, DIGIT_6};

// Current number to display (0-999999)
long currentNumber = 123456;

// Display buffer - holds the 6 digits to display
uint8_t displayBuffer[6] = {0};

// Multiplexing variables
uint8_t currentDigit = 0;
unsigned long lastRefresh = 0;
unsigned long REFRESH_INTERVAL = 2000; // 2ms per digit = ~83Hz refresh rate (adjustable)

// Scanning state machine
enum ScanState {
    SCAN_IDLE,
    SCAN_TURN_OFF_DIGITS,
    SCAN_WAIT_BLANKING,
    SCAN_WRITE_SEGMENTS,
    SCAN_WAIT_DATA,
    SCAN_TURN_ON_DIGIT,
    SCAN_DISPLAY_DIGIT
};
ScanState scanState = SCAN_IDLE;

uint8_t nextDigit = 0;
unsigned long lastStateTime = 0;
const unsigned long BLANKING_TIME = 10; // 10us blanking period to prevent ghosting

void setup() {
    Serial.begin(9600);
    Serial.println("NJU3711 6-Digit Cascaded Display Demo");
    Serial.println("=====================================");
    
    // Initialize the display driver
    display.begin();
    display.setStepDelay(1); // Fast timing
    
    // Setup digit select pins (ACTIVE LOW)
    for (int i = 0; i < 6; i++) {
        pinMode(digitPins[i], OUTPUT);
        digitalWrite(digitPins[i], HIGH); // All digits off initially (active LOW)
    }
    
    // Update display buffer with initial number
    updateDisplayBuffer(currentNumber);
    
    Serial.println("Commands:");
    Serial.println("Type a number (0-999999) to display it");
    Serial.println("+ : Increment by 1");
    Serial.println("- : Decrement by 1");
    Serial.println("c : Clear display (show 000000)");
    Serial.println("t : Test pattern (count 0-999999)");
    Serial.println("r : Set refresh interval (microseconds)");
    Serial.println("i : Show current refresh info");
    Serial.println();
    Serial.print("Displaying: ");
    Serial.println(currentNumber);
}

void loop() {
    // CRITICAL: Must call update() regularly for non-blocking operation
    display.update();
    
    // Handle multiplexing refresh
    refreshDisplay();
    
    // Handle serial commands
    handleSerialInput();
}

void refreshDisplay() {
    unsigned long currentTime = micros();
    
    switch (scanState) {
        case SCAN_IDLE:
            // Check if it's time for next digit
            if ((currentTime - lastRefresh) >= REFRESH_INTERVAL) {
                scanState = SCAN_TURN_OFF_DIGITS;
                lastStateTime = currentTime;
            }
            break;
            
        case SCAN_TURN_OFF_DIGITS:
            // Turn off all digits to prevent ghosting
            for (int i = 0; i < 6; i++) {
                digitalWrite(digitPins[i], HIGH);  // HIGH = OFF for active low
            }
            scanState = SCAN_WAIT_BLANKING;
            lastStateTime = currentTime;
            break;
            
        case SCAN_WAIT_BLANKING:
            // Wait for blanking period
            if ((currentTime - lastStateTime) >= BLANKING_TIME) {
                // Always move to next digit in sequence (0->1->2->3->4->5->0...)
                nextDigit = (currentDigit + 1) % 6;
                scanState = SCAN_WRITE_SEGMENTS;
            }
            break;
            
        case SCAN_WRITE_SEGMENTS:
            // Only proceed if NJU3711 is not busy
            if (!display.isBusy()) {
                // Write segment data for this digit
                display.displayDigit(displayBuffer[nextDigit]);
                scanState = SCAN_WAIT_DATA;
                lastStateTime = currentTime;
            }
            break;
            
        case SCAN_WAIT_DATA:
            // Wait for data to be written and latched
            if (!display.isBusy()) {
                // Add small delay to ensure data is stable
                if ((currentTime - lastStateTime) >= 10) {
                    scanState = SCAN_TURN_ON_DIGIT;
                }
            }
            break;
            
        case SCAN_TURN_ON_DIGIT:
            // Always turn on the digit, even if blank
            // This maintains constant timing
            digitalWrite(digitPins[nextDigit], LOW);  // LOW = ON for active low
            currentDigit = nextDigit;
            scanState = SCAN_DISPLAY_DIGIT;
            lastRefresh = currentTime;
            break;
            
        case SCAN_DISPLAY_DIGIT:
            // Display this digit for the multiplex delay period
            scanState = SCAN_IDLE;
            break;
    }
}

void updateDisplayBuffer(long number) {
    // Constrain number to valid range
    if (number < 0) number = 0;
    if (number > 999999) number = 999999;
    
    // Extract individual digits (right to left)
    // Display buffer index 0 = leftmost digit, 5 = rightmost digit
    displayBuffer[5] = number % 10;           // Ones
    displayBuffer[4] = (number / 10) % 10;    // Tens
    displayBuffer[3] = (number / 100) % 10;   // Hundreds
    displayBuffer[2] = (number / 1000) % 10;  // Thousands
    displayBuffer[1] = (number / 10000) % 10; // Ten thousands
    displayBuffer[0] = (number / 100000) % 10; // Hundred thousands
}

void handleSerialInput() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        
        if (input.length() == 0) return;
        
        char cmd = input.charAt(0);
        
        if (cmd >= '0' && cmd <= '9') {
            // User entered a number
            currentNumber = input.toInt();
            if (currentNumber > 999999) currentNumber = 999999;
            updateDisplayBuffer(currentNumber);
            Serial.print("Displaying: ");
            Serial.println(currentNumber);
        }
        else if (cmd == '+') {
            currentNumber++;
            if (currentNumber > 999999) currentNumber = 0;
            updateDisplayBuffer(currentNumber);
            Serial.print("Displaying: ");
            Serial.println(currentNumber);
        }
        else if (cmd == '-') {
            currentNumber--;
            if (currentNumber < 0) currentNumber = 999999;
            updateDisplayBuffer(currentNumber);
            Serial.print("Displaying: ");
            Serial.println(currentNumber);
        }
        else if (cmd == 'c' || cmd == 'C') {
            currentNumber = 0;
            updateDisplayBuffer(currentNumber);
            Serial.println("Display cleared (000000)");
        }
        else if (cmd == 't' || cmd == 'T') {
            runTestPattern();
        }
        else if (cmd == 'r' || cmd == 'R') {
            setRefreshInterval();
        }
        else if (cmd == 'i' || cmd == 'I') {
            showRefreshInfo();
        }
    }
}

void runTestPattern() {
    Serial.println("Running test pattern: Counting 0-999999");
    Serial.println("Press any key to stop...");
    
    currentNumber = 0;
    
    while (!Serial.available()) {
        display.update();
        refreshDisplay();
        
        // Update counter every 10ms
        static unsigned long lastCount = 0;
        if (millis() - lastCount > 10) {
            currentNumber++;
            if (currentNumber > 999999) currentNumber = 0;
            updateDisplayBuffer(currentNumber);
            lastCount = millis();
            
            // Print progress every 1000 counts
            if (currentNumber % 1000 == 0) {
                Serial.println(currentNumber);
            }
        }
    }
    
    // Clear serial buffer
    while (Serial.available()) Serial.read();
    
    Serial.print("Test stopped at: ");
    Serial.println(currentNumber);
}

void setRefreshInterval() {
    Serial.println();
    Serial.println("=== Set Refresh Interval ===");
    Serial.print("Current interval: ");
    Serial.print(REFRESH_INTERVAL);
    Serial.println(" microseconds");
    Serial.print("Current refresh rate: ");
    Serial.print(1000000.0 / (REFRESH_INTERVAL * 6), 1);
    Serial.println(" Hz (complete display)");
    Serial.println();
    Serial.println("Enter new refresh interval in microseconds:");
    Serial.println("Examples:");
    Serial.println("  500   = 0.5ms per digit (~333Hz display refresh)");
    Serial.println("  1000  = 1ms per digit (~167Hz display refresh)");
    Serial.println("  2000  = 2ms per digit (~83Hz display refresh) [default]");
    Serial.println("  3000  = 3ms per digit (~56Hz display refresh)");
    Serial.println("  5000  = 5ms per digit (~33Hz display refresh)");
    Serial.println();
    Serial.print("> ");
    
    // Wait for input
    while (!Serial.available()) {
        display.update();
        refreshDisplay();
    }
    
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    long newInterval = input.toInt();
    
    if (newInterval < 100) {
        Serial.println("Error: Interval too short (minimum 100us)");
        return;
    }
    
    if (newInterval > 50000) {
        Serial.println("Error: Interval too long (maximum 50ms)");
        Serial.println("Note: Intervals >5ms may cause visible flicker");
        return;
    }
    
    REFRESH_INTERVAL = newInterval;
    
    Serial.println();
    Serial.print("Refresh interval set to: ");
    Serial.print(REFRESH_INTERVAL);
    Serial.println(" microseconds");
    Serial.print("Display refresh rate: ");
    Serial.print(1000000.0 / (REFRESH_INTERVAL * 6), 1);
    Serial.println(" Hz");
    
    if (REFRESH_INTERVAL > 5000) {
        Serial.println("Warning: Interval >5ms may cause visible flicker");
    }
    Serial.println();
}

void showRefreshInfo() {
    Serial.println();
    Serial.println("=== Refresh Information ===");
    Serial.print("Refresh interval: ");
    Serial.print(REFRESH_INTERVAL);
    Serial.println(" microseconds per digit");
    Serial.print("Time per complete display: ");
    Serial.print((REFRESH_INTERVAL * 6) / 1000.0, 2);
    Serial.println(" milliseconds");
    Serial.print("Display refresh rate: ");
    Serial.print(1000000.0 / (REFRESH_INTERVAL * 6), 1);
    Serial.println(" Hz");
    Serial.print("Digit refresh rate: ");
    Serial.print(1000000.0 / REFRESH_INTERVAL, 1);
    Serial.println(" Hz");
    Serial.println();
    
    // Show recommendations
    Serial.println("Recommended ranges:");
    Serial.println("  Fast:   500-1000us (good for bright displays)");
    Serial.println("  Normal: 1500-2500us (balanced, default 2000us)");
    Serial.println("  Slow:   3000-5000us (may flicker on some displays)");
    Serial.println();
}

/*
 * Alternative function: Display with leading zero suppression
 * Uncomment and use this version of updateDisplayBuffer if you want
 * leading zeros to be blank (e.g., "123" displays as "   123" instead of "000123")
 */
/*
void updateDisplayBufferWithLeadingZeroSuppression(long number) {
    // Constrain number to valid range
    if (number < 0) number = 0;
    if (number > 999999) number = 999999;
    
    // Extract individual digits
    displayBuffer[5] = number % 10;
    displayBuffer[4] = (number / 10) % 10;
    displayBuffer[3] = (number / 100) % 10;
    displayBuffer[2] = (number / 1000) % 10;
    displayBuffer[1] = (number / 10000) % 10;
    displayBuffer[0] = (number / 100000) % 10;
    
    // Suppress leading zeros by marking them as blank (use 10 as blank marker)
    bool foundNonZero = false;
    for (int i = 0; i < 5; i++) { // Don't suppress the last digit
        if (displayBuffer[i] != 0) {
            foundNonZero = true;
        }
        if (!foundNonZero) {
            displayBuffer[i] = 10; // Mark as blank
        }
    }
}

// Modified refreshDisplay() to handle blanks:
void refreshDisplayWithBlanks() {
    unsigned long currentTime = micros();
    
    switch (scanState) {
        case SCAN_IDLE:
            if ((currentTime - lastRefresh) >= REFRESH_INTERVAL) {
                scanState = SCAN_TURN_OFF_DIGITS;
                lastStateTime = currentTime;
            }
            break;
            
        case SCAN_TURN_OFF_DIGITS:
            for (int i = 0; i < 6; i++) {
                digitalWrite(digitPins[i], HIGH);  // HIGH = OFF
            }
            scanState = SCAN_WAIT_BLANKING;
            lastStateTime = currentTime;
            break;
            
        case SCAN_WAIT_BLANKING:
            if ((currentTime - lastStateTime) >= BLANKING_TIME) {
                nextDigit = (currentDigit + 1) % 6;
                scanState = SCAN_WRITE_SEGMENTS;
            }
            break;
            
        case SCAN_WRITE_SEGMENTS:
            if (!display.isBusy()) {
                // Display blank or digit
                if (displayBuffer[nextDigit] == 10) {
                    display.displayBlank();
                } else {
                    display.displayDigit(displayBuffer[nextDigit]);
                }
                scanState = SCAN_WAIT_DATA;
                lastStateTime = currentTime;
            }
            break;
            
        case SCAN_WAIT_DATA:
            if (!display.isBusy()) {
                if ((currentTime - lastStateTime) >= 10) {
                    scanState = SCAN_TURN_ON_DIGIT;
                }
            }
            break;
            
        case SCAN_TURN_ON_DIGIT:
            digitalWrite(digitPins[nextDigit], LOW);  // LOW = ON
            currentDigit = nextDigit;
            scanState = SCAN_DISPLAY_DIGIT;
            lastRefresh = currentTime;
            break;
            
        case SCAN_DISPLAY_DIGIT:
            scanState = SCAN_IDLE;
            break;
    }
}
*/