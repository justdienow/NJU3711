/*
 * NJU3711_7Segment_Multi.cpp - 3-Digit 7-Segment Display Implementation
 * 
 * This class extends the NJU3711_7Segment driver to support three
 * 7-segment displays with high-side transistor switching.
 * 
 * Author: justdienow
 * Version: 1.0
 */

#include "NJU3711_7Segment_Multi.h"

// Constructor for 3-digit display
NJU3711_7Segment_Multi::NJU3711_7Segment_Multi(uint8_t dataPin, uint8_t clockPin, uint8_t strobePin,
                                               uint8_t digit1Pin, uint8_t digit2Pin, uint8_t digit3Pin,
                                               DisplayMode mode)
    : NJU3711_7Segment(dataPin, clockPin, strobePin, mode) {
    
    _digitPins[0] = digit1Pin;
    _digitPins[1] = digit2Pin;
    _digitPins[2] = digit3Pin;
    
    // Initialize display state
    _mplexState = MPLEX_IDLE;
    _currentDigit = 0;
    _nextDigit = 0;
    _lastMultiplexTime = 0;
    _lastStateTime = 0;
    _multiplexDelay = 2000;  // 2ms default
    _blankingTime = 50;     // 50us default blanking
    _multiplexEnabled = true;
    _displayValue = 0;
    _leadingZeros = false;
    _blankOnZero = false;
    
    // Clear digit data
    for (int i = 0; i < 3; i++) {
        _digitData[i] = 0;
        _digitDP[i] = false;
        _digitEnabled[i] = true;
    }
}

// Constructor for 3-digit with CLR pin
NJU3711_7Segment_Multi::NJU3711_7Segment_Multi(uint8_t dataPin, uint8_t clockPin, uint8_t strobePin, uint8_t clearPin,
                                               uint8_t digit1Pin, uint8_t digit2Pin, uint8_t digit3Pin,
                                               DisplayMode mode)
    : NJU3711_7Segment(dataPin, clockPin, strobePin, clearPin, mode) {
    
    _digitPins[0] = digit1Pin;
    _digitPins[1] = digit2Pin;
    _digitPins[2] = digit3Pin;
    
    // Initialize display state
    _mplexState = MPLEX_IDLE;
    _currentDigit = 0;
    _nextDigit = 0;
    _lastMultiplexTime = 0;
    _lastStateTime = 0;
    _multiplexDelay = 2000;  // 2ms default
    _blankingTime = 50;     // 50us default blanking
    _multiplexEnabled = true;
    _displayValue = 0;
    _leadingZeros = false;
    _blankOnZero = false;
    
    // Clear digit data
    for (int i = 0; i < 3; i++) {
        _digitData[i] = 0;
        _digitDP[i] = false;
        _digitEnabled[i] = true;
    }
}

// Initialize
void NJU3711_7Segment_Multi::begin() {
    // Call parent begin
    NJU3711_7Segment::begin();
    
    // Set up digit control pins
    for (int i = 0; i < 3; i++) {
        pinMode(_digitPins[i], OUTPUT);
        digitalWrite(_digitPins[i], HIGH);  // HIGH = OFF for PNP transistor
    }
    
    _lastMultiplexTime = micros();
}

// Update - must be called regularly
void NJU3711_7Segment_Multi::update() {
    // Call parent update
    NJU3711_7Segment::update();
    
    // Handle multiplexing if enabled
    if (_multiplexEnabled && !isBusy()) {
        multiplexDisplay();
    }
}

// Multiplex the display with proper state machine
void NJU3711_7Segment_Multi::multiplexDisplay() {
    unsigned long currentTime = micros();
    
    switch (_mplexState) {
        case MPLEX_IDLE:
            // Check if it's time for next digit
            if ((currentTime - _lastMultiplexTime) >= _multiplexDelay) {
                _mplexState = MPLEX_TURN_OFF_DIGITS;
                _lastStateTime = currentTime;
            }
            break;
            
        case MPLEX_TURN_OFF_DIGITS:
            // Turn off all digits to prevent ghosting
            deselectAllDigits();
            _mplexState = MPLEX_WAIT_BLANKING;
            _lastStateTime = currentTime;
            break;
            
        case MPLEX_WAIT_BLANKING:
            // Wait for blanking period
            if ((currentTime - _lastStateTime) >= _blankingTime) {
                // Always move to next digit in sequence (0->1->2->0...)
                _nextDigit = (_currentDigit + 1) % 3;
                _mplexState = MPLEX_WRITE_DATA;
            }
            break;
            
        case MPLEX_WRITE_DATA:
            // Only proceed if NJU3711 is not busy
            if (!isBusy()) {
                uint8_t pattern;
                
                // Check if this digit should display something
                if (_digitEnabled[_nextDigit]) {
                    // Prepare segment data
                    pattern = _digitData[_nextDigit];
                    if (_digitDP[_nextDigit]) {
                        pattern |= (1 << 0);  // DP is bit 0
                    }
                } else {
                    // Digit is disabled - show blank
                    pattern = 0x00;
                }
                
                // Apply display mode (invert for active low)
                pattern = (getDisplayMode() == ACTIVE_LOW) ? ~pattern : pattern;
                
                // Write to NJU3711
                NJU3711::write(pattern);
                _mplexState = MPLEX_WAIT_DATA;
                _lastStateTime = currentTime;
            }
            break;
            
        case MPLEX_WAIT_DATA:
            // Wait for data to be written and latched
            if (!isBusy()) {
                // Add small delay to ensure data is stable
                if ((currentTime - _lastStateTime) >= 10) {
                    _mplexState = MPLEX_TURN_ON_DIGIT;
                }
            }
            break;
            
        case MPLEX_TURN_ON_DIGIT:
            // Always turn on the digit, even if blank
            // This maintains constant timing
            selectDigit(_nextDigit);
            _currentDigit = _nextDigit;
            _mplexState = MPLEX_DISPLAY_DIGIT;
            _lastMultiplexTime = currentTime;
            break;
            
        case MPLEX_DISPLAY_DIGIT:
            // Display this digit for the multiplex delay period
            _mplexState = MPLEX_IDLE;
            break;
    }
}

// Select a digit (turn on its transistor)
void NJU3711_7Segment_Multi::selectDigit(uint8_t digit) {
    if (digit < 3) {
        digitalWrite(_digitPins[digit], LOW);  // LOW = ON for PNP transistor
    }
}

// Deselect all digits
void NJU3711_7Segment_Multi::deselectAllDigits() {
    for (int i = 0; i < 3; i++) {
        digitalWrite(_digitPins[i], HIGH);  // HIGH = OFF for PNP transistor
    }
}

// Display a number (0-999)
bool NJU3711_7Segment_Multi::displayNumber(uint16_t number) {
    if (number > 999) number = 999;
    
    _displayValue = number;
    updateDigitData();
    return true;
}

// Display number with decimal point
bool NJU3711_7Segment_Multi::displayNumber(uint16_t number, uint8_t decimalPosition) {
    displayNumber(number);
    
    // Clear all decimal points first
    clearAllDecimalPoints();
    
    // Set decimal point if position is valid
    if (decimalPosition > 0 && decimalPosition <= 3) {
        _digitDP[decimalPosition - 1] = true;
    }
    
    return true;
}

// Update digit data based on current display value
void NJU3711_7Segment_Multi::updateDigitData() {
    uint16_t value = _displayValue;
    
    // Handle blank on zero
    if (_blankOnZero && value == 0) {
        for (int i = 0; i < 3; i++) {
            _digitData[i] = 0;  // Blank
            _digitEnabled[i] = false;
        }
        return;
    }
    
    // Extract digits (rightmost = ones, leftmost = hundreds)
    uint8_t digits[3];
    digits[0] = value % 10;         // Ones (rightmost)
    digits[1] = (value / 10) % 10;  // Tens (middle)
    digits[2] = (value / 100) % 10; // Hundreds (leftmost)
    
    // Map to physical positions
    // Position 0 = rightmost digit (ones)
    // Position 1 = middle digit (tens)
    // Position 2 = leftmost digit (hundreds)
    
    // Determine which digits to show
    bool showDigit[3] = {false, false, false};
    
    if (_leadingZeros) {
        // Show all digits with leading zeros
        showDigit[0] = true;
        showDigit[1] = true;
        showDigit[2] = true;
    } else {
        // Determine which digits to show based on value
        if (value >= 100) {
            showDigit[0] = true;  // ones
            showDigit[1] = true;  // tens
            showDigit[2] = true;  // hundreds
        } else if (value >= 10) {
            showDigit[0] = true;  // ones
            showDigit[1] = true;  // tens
            showDigit[2] = false; // no hundreds
        } else {
            showDigit[0] = true;  // ones always shown
            showDigit[1] = false; // no tens
            showDigit[2] = false; // no hundreds
        }
    }
    
    // Update digit data
    for (int i = 0; i < 3; i++) {
        if (showDigit[i]) {
            _digitData[i] = getDigitPattern(digits[i]);
            _digitEnabled[i] = true;
        } else {
            _digitData[i] = 0;  // Blank pattern
            _digitEnabled[i] = false;
        }
    }
}

// Set individual digit
bool NJU3711_7Segment_Multi::setDigit(uint8_t position, uint8_t value, bool showDP) {
    if (position >= 3) return false;
    
    _digitData[position] = getDigitPattern(value);
    _digitDP[position] = showDP;
    _digitEnabled[position] = true;
    return true;
}

// Set digit with character
bool NJU3711_7Segment_Multi::setDigitChar(uint8_t position, char character, bool showDP) {
    if (position >= 3) return false;
    
    _digitData[position] = getCharPattern(character);
    _digitDP[position] = showDP;
    _digitEnabled[position] = true;
    return true;
}

// Set digit with raw segments
bool NJU3711_7Segment_Multi::setDigitRaw(uint8_t position, uint8_t segments, bool showDP) {
    if (position >= 3) return false;
    
    _digitData[position] = segments;
    _digitDP[position] = showDP;
    _digitEnabled[position] = true;
    return true;
}

// Enable/disable digits
bool NJU3711_7Segment_Multi::enableDigit(uint8_t position, bool enable) {
    if (position >= 3) return false;
    _digitEnabled[position] = enable;
    return true;
}

bool NJU3711_7Segment_Multi::disableDigit(uint8_t position) {
    return enableDigit(position, false);
}

void NJU3711_7Segment_Multi::enableAllDigits() {
    for (int i = 0; i < 3; i++) {
        _digitEnabled[i] = true;
    }
}

void NJU3711_7Segment_Multi::disableAllDigits() {
    for (int i = 0; i < 3; i++) {
        _digitEnabled[i] = false;
    }
}

// Display control
void NJU3711_7Segment_Multi::setLeadingZeros(bool enable) {
    _leadingZeros = enable;
    updateDigitData();
}

void NJU3711_7Segment_Multi::setBlankOnZero(bool enable) {
    _blankOnZero = enable;
    updateDigitData();
}

void NJU3711_7Segment_Multi::clearDisplay() {
    disableAllDigits();
    deselectAllDigits();
}

void NJU3711_7Segment_Multi::displayAll() {
    for (int i = 0; i < 3; i++) {
        _digitData[i] = 0xFF;  // All segments on
        _digitDP[i] = true;
        _digitEnabled[i] = true;
    }
}

// Multiplexing control
void NJU3711_7Segment_Multi::setMultiplexDelay(unsigned long delayMicros) {
    _multiplexDelay = delayMicros;
}

void NJU3711_7Segment_Multi::setBlankingTime(unsigned long blankingMicros) {
    _blankingTime = blankingMicros;
}

void NJU3711_7Segment_Multi::enableMultiplex(bool enable) {
    _multiplexEnabled = enable;
    if (!enable) {
        deselectAllDigits();
    }
}

void NJU3711_7Segment_Multi::disableMultiplex() {
    enableMultiplex(false);
}

bool NJU3711_7Segment_Multi::isMultiplexing() {
    return _multiplexEnabled;
}

// Special displays
bool NJU3711_7Segment_Multi::displayError() {
    setDigitChar(2, 'E', false);  // Leftmost
    setDigitChar(1, 'r', false);
    setDigitChar(0, 'r', false);  // Rightmost
    return true;
}

bool NJU3711_7Segment_Multi::displayDashes() {
    for (int i = 0; i < 3; i++) {
        setDigitChar(i, '-', false);
    }
    return true;
}

bool NJU3711_7Segment_Multi::displayTemperature(int16_t temp, bool celsius) {
    if (temp < -99) temp = -99;
    if (temp > 999) temp = 999;
    
    if (temp < 0) {
        // Negative temperature
        temp = -temp;
        setDigitChar(2, '-', false);      // Minus sign
        setDigit(1, (temp / 10) % 10, false);
        setDigit(0, temp % 10, false);
    } else {
        // Positive temperature
        displayNumber(temp);
    }
    
    return true;
}

// Decimal point control
bool NJU3711_7Segment_Multi::setDecimalPoint(uint8_t position, bool state) {
    if (position >= 3) return false;
    _digitDP[position] = state;
    return true;
}

void NJU3711_7Segment_Multi::clearAllDecimalPoints() {
    for (int i = 0; i < 3; i++) {
        _digitDP[i] = false;
    }
}

// Get current value
uint16_t NJU3711_7Segment_Multi::getCurrentValue() {
    return _displayValue;
}