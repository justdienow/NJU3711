/*
 * NJU3711_7Segment_Cascade_Clock_Demo.ino - Cascaded 6-Digit Clock Demo
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
 * Arduino Pin 5  -> Digit 1 select (leftmost - Hour tens)
 * Arduino Pin 6  -> Digit 2 select (Hour ones)
 * Arduino Pin 7  -> Digit 3 select (Minute tens)
 * Arduino Pin 8  -> Digit 4 select (Minute ones)
 * Arduino Pin 9  -> Digit 5 select (Second tens)
 * Arduino Pin 10 -> Digit 6 select (rightmost - Second ones)
 * 
 * Display Configuration:
 * [H] [H] [M] [M] [S] [S]
 *  ^PM dot  ^blink ^blink
 * 
 * The clock displays HH.MM.SS format with:
 * - Far left DP = PM indicator (12-hour mode only)
 * - Middle DPs = Colon separator (blinks every second)
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
#define DIGIT_1   5  // Hour tens
#define DIGIT_2   6  // Hour ones
#define DIGIT_3   7  // Minute tens
#define DIGIT_4   8  // Minute ones
#define DIGIT_5   9  // Second tens
#define DIGIT_6   10 // Second ones

// Create display driver instance
NJU3711_7Segment display(DATA_PIN, CLK_PIN, STB_PIN, ACTIVE_LOW);

// Array to hold digit select pins
const uint8_t digitPins[6] = {DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4, DIGIT_5, DIGIT_6};

// Display buffer - holds the 6 digits to display
uint8_t displayBuffer[6] = {0};
bool displayDP[6] = {false}; // Decimal point state for each digit

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

// Clock variables
uint8_t hours = 12;
uint8_t minutes = 0;
uint8_t seconds = 0;
bool is24HourMode = false;
bool colonBlinkState = false;
unsigned long lastSecondUpdate = 0;
unsigned long lastBlinkUpdate = 0;
const unsigned long BLINK_INTERVAL = 500000; // 500ms blink rate

void setup() {
    Serial.begin(9600);
    Serial.println("NJU3711 6-Digit Clock Demo");
    Serial.println("==========================");
    
    // Initialize the display driver
    display.begin();
    display.setStepDelay(1); // Fast timing
    
    // Setup digit select pins
    for (int i = 0; i < 6; i++) {
        pinMode(digitPins[i], OUTPUT);
        digitalWrite(digitPins[i], HIGH); // All digits off initially (active LOW)
    }
    
    // Initialize clock display
    updateClockDisplay();
    
    Serial.println("Commands:");
    Serial.println("HHMMSS - Set time (e.g., 013055 = 01:30:55)");
    Serial.println("h      - Toggle 12/24 hour format");
    Serial.println("r      - Set refresh interval (microseconds)");
    Serial.println("i      - Show current refresh info");
    Serial.println("t      - Show current time");
    Serial.println();
    Serial.print("Current time: ");
    printTime();
    Serial.print(" (");
    Serial.print(is24HourMode ? "24" : "12");
    Serial.println("-hour mode)");
}

void loop() {
    // CRITICAL: Must call update() regularly for non-blocking operation
    display.update();
    
    // Handle multiplexing refresh
    refreshDisplay();
    
    // Update clock (increment seconds)
    updateClock();
    
    // Handle colon blinking
    updateColonBlink();
    
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
                // Write segment data for this digit with its decimal point
                display.displayDigit(displayBuffer[nextDigit], displayDP[nextDigit]);
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

void updateClock() {
    unsigned long currentTime = millis();
    
    // Update every second
    if (currentTime - lastSecondUpdate >= 1000) {
        seconds++;
        
        if (seconds >= 60) {
            seconds = 0;
            minutes++;
            
            if (minutes >= 60) {
                minutes = 0;
                hours++;
                
                if (is24HourMode) {
                    if (hours >= 24) {
                        hours = 0;
                    }
                } else {
                    if (hours > 12) {
                        hours = 1;
                    }
                }
            }
        }
        
        updateClockDisplay();
        lastSecondUpdate = currentTime;
    }
}

void updateColonBlink() {
    unsigned long currentTime = micros();
    
    // Blink the colon separators every 500ms
    if (currentTime - lastBlinkUpdate >= BLINK_INTERVAL) {
        colonBlinkState = !colonBlinkState;
        
        // Update the decimal points for the colon
        displayDP[2] = colonBlinkState; // After hours
        displayDP[4] = colonBlinkState; // After minutes
        
        lastBlinkUpdate = currentTime;
    }
}

void updateClockDisplay() {
    uint8_t displayHours = hours;
    
    // Handle 12-hour mode
    if (!is24HourMode) {
        // Set PM indicator (far left DP)
        if (hours >= 12) {
            displayDP[0] = true; // PM
        } else {
            displayDP[0] = false; // AM
        }
        
        // Convert to 12-hour format
        if (hours == 0) {
            displayHours = 12;
        } else if (hours > 12) {
            displayHours = hours - 12;
        }
    } else {
        displayDP[0] = false; // No PM indicator in 24-hour mode
    }
    
    // Extract individual digits
    displayBuffer[0] = displayHours / 10;      // Hour tens
    displayBuffer[1] = displayHours % 10;      // Hour ones
    displayBuffer[2] = minutes / 10;           // Minute tens
    displayBuffer[3] = minutes % 10;           // Minute ones
    displayBuffer[4] = seconds / 10;           // Second tens
    displayBuffer[5] = seconds % 10;           // Second ones
    
    // Handle leading zero for hours in 12-hour mode
    if (!is24HourMode && displayBuffer[0] == 0) {
        // We could blank it, but for clock clarity, we'll keep the zero
        // Uncomment the next line to blank the leading zero:
        // displayBuffer[0] = 10; // Use 10 as blank marker
    }
}

void setTime(uint8_t h, uint8_t m, uint8_t s) {
    // Validate and constrain values
    if (is24HourMode) {
        hours = constrain(h, 0, 23);
    } else {
        hours = constrain(h, 1, 12);
    }
    
    minutes = constrain(m, 0, 59);
    seconds = constrain(s, 0, 59);
    
    updateClockDisplay();
    lastSecondUpdate = millis(); // Reset second counter
}

void toggle12_24Hour() {
    is24HourMode = !is24HourMode;
    
    // Convert current time if switching modes
    if (!is24HourMode) {
        // Switching to 12-hour mode
        if (hours == 0) {
            hours = 12;
        } else if (hours > 12) {
            // Keep the hour value, PM indicator will show it's PM
        }
    } else {
        // Switching to 24-hour mode
        if (hours == 12 && !displayDP[0]) {
            // 12 AM becomes 00
            hours = 0;
        } else if (hours < 12 && displayDP[0]) {
            // PM times need 12 added
            hours += 12;
        }
    }
    
    updateClockDisplay();
    
    Serial.print("Switched to ");
    Serial.print(is24HourMode ? "24" : "12");
    Serial.println("-hour mode");
    Serial.print("Current time: ");
    printTime();
    Serial.println();
}

void printTime() {
    uint8_t displayHours = hours;
    
    if (!is24HourMode) {
        if (hours > 12) {
            displayHours = hours - 12;
        } else if (hours == 0) {
            displayHours = 12;
        }
    }
    
    if (displayHours < 10) Serial.print('0');
    Serial.print(displayHours);
    Serial.print(':');
    if (minutes < 10) Serial.print('0');
    Serial.print(minutes);
    Serial.print(':');
    if (seconds < 10) Serial.print('0');
    Serial.print(seconds);
    
    if (!is24HourMode) {
        Serial.print(displayDP[0] ? " PM" : " AM");
    }
}

void handleSerialInput() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        
        if (input.length() == 0) return;
        
        char cmd = input.charAt(0);
        
        // Check if it's a 6-digit time input (HHMMSS)
        if (input.length() == 6 && isDigit(cmd)) {
            uint8_t h = (input.substring(0, 2)).toInt();
            uint8_t m = (input.substring(2, 4)).toInt();
            uint8_t s = (input.substring(4, 6)).toInt();
            
            setTime(h, m, s);
            Serial.print("Time set to: ");
            printTime();
            Serial.println();
        }
        else if (cmd == 'h' || cmd == 'H') {
            toggle12_24Hour();
        }
        else if (cmd == 'r' || cmd == 'R') {
            setRefreshInterval();
        }
        else if (cmd == 'i' || cmd == 'I') {
            showRefreshInfo();
        }
        else if (cmd == 't' || cmd == 'T') {
            Serial.print("Current time: ");
            printTime();
            Serial.print(" (");
            Serial.print(is24HourMode ? "24" : "12");
            Serial.println("-hour mode)");
        }
    }
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
        updateClock();
        updateColonBlink();
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