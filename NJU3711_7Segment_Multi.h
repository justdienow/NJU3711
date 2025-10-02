/*
 * NJU3711_7Segment_Multi.h - 3-Digit 7-Segment Display Extension
 * 
 * This class extends the NJU3711_7Segment driver to support three
 * 7-segment displays with high-side transistor switching.
 * 
 * Supports 3 digits (0-999) with automatic multiplexing.
 * 
 * Author: justdienow
 * Version: 1.0
 */

#ifndef NJU3711_7SEGMENT_MULTI_H
#define NJU3711_7SEGMENT_MULTI_H

#include "NJU3711_7Segment.h"

class NJU3711_7Segment_Multi : public NJU3711_7Segment {
private:
    // Digit control pins (connected to transistor bases)
    uint8_t _digitPins[3];
    
    // Display data
    uint8_t _digitData[3];      // Segment data for each digit
    bool _digitDP[3];           // Decimal point state for each digit
    bool _digitEnabled[3];      // Enable/disable individual digits
    
    // Multiplexing state
    enum MultiplexState {
        MPLEX_IDLE,
        MPLEX_TURN_OFF_DIGITS,
        MPLEX_WAIT_BLANKING,
        MPLEX_WRITE_DATA,
        MPLEX_WAIT_DATA,
        MPLEX_TURN_ON_DIGIT,
        MPLEX_DISPLAY_DIGIT
    };
    
    MultiplexState _mplexState;
    uint8_t _currentDigit;      // Currently active digit (0-2)
    uint8_t _nextDigit;         // Next digit to display
    unsigned long _lastMultiplexTime;
    unsigned long _lastStateTime;
    unsigned long _multiplexDelay;  // Time between digit switches (microseconds)
    unsigned long _blankingTime;    // Blanking time between digits (microseconds)
    bool _multiplexEnabled;
    
    // Display value
    uint16_t _displayValue;     // 0-999
    bool _leadingZeros;         // Show leading zeros
    bool _blankOnZero;         // Blank display when value is 0
    
    // Internal methods
    void multiplexDisplay();
    void updateDigitData();

public:
    // Constructor for 3-digit display
    NJU3711_7Segment_Multi(uint8_t dataPin, uint8_t clockPin, uint8_t strobePin,
                           uint8_t digit1Pin, uint8_t digit2Pin, uint8_t digit3Pin,
                           DisplayMode mode = ACTIVE_LOW);
    
    // Constructor for 3-digit with CLR pin
    NJU3711_7Segment_Multi(uint8_t dataPin, uint8_t clockPin, uint8_t strobePin, uint8_t clearPin,
                           uint8_t digit1Pin, uint8_t digit2Pin, uint8_t digit3Pin,
                           DisplayMode mode = ACTIVE_LOW);
    
    // Initialize (must call in setup())
    void begin();
    
    // Update method (must call regularly in loop())
    void update();
    
    // Display numeric value (0-999)
    bool displayNumber(uint16_t number);
    bool displayNumber(uint16_t number, uint8_t decimalPosition); // 0=no DP, 1=digit1, 2=digit2, 3=digit3
    
    // Display individual digits
    bool setDigit(uint8_t position, uint8_t value, bool showDP = false);
    bool setDigitChar(uint8_t position, char character, bool showDP = false);
    bool setDigitRaw(uint8_t position, uint8_t segments, bool showDP = false);
    
    // Digit control
    bool enableDigit(uint8_t position, bool enable = true);
    bool disableDigit(uint8_t position);
    void enableAllDigits();
    void disableAllDigits();
    
    // Display control
    void setLeadingZeros(bool enable);
    void setBlankOnZero(bool enable);
    void clearDisplay();
    void displayAll();  // Test pattern - all segments on all digits
    
    // Multiplexing control
    void setMultiplexDelay(unsigned long delayMicros); // Default 2000us (2ms)
    void setBlankingTime(unsigned long blankingMicros); // Default 50us
    void enableMultiplex(bool enable = true);
    void disableMultiplex();
    bool isMultiplexing();
    
    // Special displays
    bool displayError();        // Show "Err"
    bool displayDashes();       // Show "---"
    bool displayTemperature(int16_t temp, bool celsius = true); // -99 to 999
    
    // Decimal point control
    bool setDecimalPoint(uint8_t position, bool state);
    void clearAllDecimalPoints();
    
    // Get current value
    uint16_t getCurrentValue();
    
    // Direct digit control (useful for testing)
    void selectDigit(uint8_t digit);
    void deselectAllDigits();
};

#endif // NJU3711_7SEGMENT_MULTI_H