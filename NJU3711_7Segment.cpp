/*
 * NJU3711_7Segment.cpp - 7-Segment Display Extension Implementation
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
 * 
 * Author: justdienow
 * Version: 1.0
 * 
 */

#include "NJU3711_7Segment.h"

// 7-segment patterns for digits 0-9 
// Using your specific pin mapping: A=P8(bit7), B=P6(bit5), C=P3(bit2), D=P2(bit1), E=P4(bit3), F=P7(bit6), G=P5(bit4), DP=P1(bit0)
const uint8_t DIGIT_PATTERNS[] = {
    0b11101110, // 0: ABCDEF
    0b00100100, // 1: BC
    0b10111010, // 2: ABDEG
    0b10110110, // 3: ABCDG
    0b01110100, // 4: BCFG
    0b11010110, // 5: ACDFG
    0b11011110, // 6: ACDEFG
    0b10100100, // 7: ABC
    0b11111110, // 8: ABCDEFG
    0b11110110  // 9: ABCDFG
};

// Hexadecimal patterns for A-F (using your pin mapping)
const uint8_t HEX_PATTERNS[] = {
    0b11111100, // A: ABCEFG
    0b01011110, // b: CDEFG  
    0b11001010, // C: ADEF
    0b00111110, // d: BCDEG
    0b11011010, // E: ADEFG
    0b11011000  // F: AEFG
};

// Character patterns for common letters and symbols
const uint8_t CHAR_PATTERNS[] = {
    // Letters that can be displayed on 7-segment
    0b11111100, // A
    0b01011110, // b
    0b11001010, // C
    0b00111110, // d
    0b11011010, // E
    0b11011000, // F
    0b00010000, // G
    0b00010000, // H
    0b00010000, // I
    0b00010000, // J
    0b00010000, // K (same as H)
    0b00010000, // L
    0b00010000, // M (approximation)
    0b00010000, // n
    0b00010000, // o
    0b00010000, // P
    0b00010000, // q
    0b00011000, // r
    0b00010000, // S
    0b00010000, // t
    0b00010000, // U
    0b00010000, // V (same as U)
    0b00010000, // W (same as U)
    0b00010000, // X (same as H)
    0b00010000, // y
    0b00010000  // Z (same as 2)
};

// Special character patterns (using your pin mapping)
#define PATTERN_MINUS      0b00010000  // G segment only (bit 4)
#define PATTERN_UNDERSCORE 0b00000010  // D segment only (bit 1) 
#define PATTERN_DEGREE     0b11110000  // ABFG segments
#define PATTERN_BLANK      0b00000000  // All segments off
#define PATTERN_ALL        0b11111111  // All segments on
#define PATTERN_ERROR      0b11011010  // E pattern
#define PATTERN_OFF_O      0b00011110  // o pattern for "OFF"
#define PATTERN_OFF_F      0b11011000  // F pattern for "OFF"

// Constructors
NJU3711_7Segment::NJU3711_7Segment(uint8_t dataPin, uint8_t clockPin, uint8_t strobePin, DisplayMode mode)
    : NJU3711(dataPin, clockPin, strobePin) {
    _displayMode = mode;
    _decimalPointState = false;
    _brightness = 255;
    _animationActive = false;
    _currentAnimation = ANIM_ROTATE_CW;
    _animationDelay = 100000;
    _lastAnimationUpdate = 0;
    _animationStep = 0;
    _animationValue = 0;
}

NJU3711_7Segment::NJU3711_7Segment(uint8_t dataPin, uint8_t clockPin, uint8_t strobePin, uint8_t clearPin, DisplayMode mode)
    : NJU3711(dataPin, clockPin, strobePin, clearPin) {
    _displayMode = mode;
    _decimalPointState = false;
    _brightness = 255;
    _animationActive = false;
    _currentAnimation = ANIM_ROTATE_CW;
    _animationDelay = 100000;
    _lastAnimationUpdate = 0;
    _animationStep = 0;
    _animationValue = 0;
}

// Update method to handle animations (calls parent update + animation processing)
void NJU3711_7Segment::update() {
    // Call parent update first
    NJU3711::update();
    
    // Handle animations
    if (_animationActive) {
        processAnimation();
    }
}

// Apply display mode (active low vs active high)
uint8_t NJU3711_7Segment::applyDisplayMode(uint8_t segmentData) {
    if (_displayMode == ACTIVE_LOW) {
        return ~segmentData; // Invert for active low (your case: LOW = LED on)
    }
    return segmentData; // Active high: HIGH = LED on
}

// Get digit pattern (0-9)
uint8_t NJU3711_7Segment::getDigitPattern(uint8_t digit) {
    if (digit > 9) return PATTERN_BLANK;
    return DIGIT_PATTERNS[digit];
}

// Get hex pattern (0-F)
uint8_t NJU3711_7Segment::getHexPattern(uint8_t hexValue) {
    if (hexValue <= 9) {
        return DIGIT_PATTERNS[hexValue];
    } else if (hexValue <= 15) {
        return HEX_PATTERNS[hexValue - 10];
    }
    return PATTERN_BLANK;
}

// Get character pattern
uint8_t NJU3711_7Segment::getCharPattern(char character) {
    // Convert to uppercase
    if (character >= 'a' && character <= 'z') {
        character = character - 'a' + 'A';
    }
    
    // Handle digits
    if (character >= '0' && character <= '9') {
        return getDigitPattern(character - '0');
    }
    
    // Handle letters A-Z
    if (character >= 'A' && character <= 'Z') {
        return CHAR_PATTERNS[character - 'A'];
    }
    
    // Handle special characters
    switch (character) {
        case '-': return PATTERN_MINUS;
        case '_': return PATTERN_UNDERSCORE;
        case ' ': return PATTERN_BLANK;
        default: return PATTERN_BLANK;
    }
}

// Display digit (0-9)
bool NJU3711_7Segment::displayDigit(uint8_t digit, bool showDP) {
    if (_animationActive) stopAnimation();
    
    uint8_t pattern = getDigitPattern(digit);
    if (showDP) pattern |= (1 << SEG_DP);
    
    _decimalPointState = showDP;
    return write(applyDisplayMode(pattern));
}

// Display hex value (0-F)
bool NJU3711_7Segment::displayHex(uint8_t hexValue, bool showDP) {
    if (_animationActive) stopAnimation();
    
    uint8_t pattern = getHexPattern(hexValue);
    if (showDP) pattern |= (1 << SEG_DP);
    
    _decimalPointState = showDP;
    return write(applyDisplayMode(pattern));
}

// Display character
bool NJU3711_7Segment::displayChar(char character, bool showDP) {
    if (_animationActive) stopAnimation();
    
    uint8_t pattern = getCharPattern(character);
    if (showDP) pattern |= (1 << SEG_DP);
    
    _decimalPointState = showDP;
    return write(applyDisplayMode(pattern));
}

// Display raw segment pattern
bool NJU3711_7Segment::displayRaw(uint8_t segmentMask, bool showDP) {
    if (_animationActive) stopAnimation();
    
    if (showDP) segmentMask |= (1 << SEG_DP);
    _decimalPointState = showDP;
    return write(applyDisplayMode(segmentMask));
}

// Set individual segment
bool NJU3711_7Segment::setSegment(uint8_t segment, bool state) {
    if (segment > 7) return false;
    if (_animationActive) stopAnimation();
    
    if (segment == SEG_DP) {
        _decimalPointState = state;
    }
    
    return writeBit(segment, _displayMode == ACTIVE_LOW ? !state : state);
}

// Clear individual segment
bool NJU3711_7Segment::clearSegment(uint8_t segment) {
    return setSegment(segment, false);
}

// Toggle individual segment
bool NJU3711_7Segment::toggleSegment(uint8_t segment) {
    if (segment > 7) return false;
    if (_animationActive) stopAnimation();
    
    if (segment == SEG_DP) {
        _decimalPointState = !_decimalPointState;
    }
    
    return toggleBit(segment);
}

// Decimal point control
bool NJU3711_7Segment::setDecimalPoint(bool state) {
    return setSegment(SEG_DP, state);
}

bool NJU3711_7Segment::toggleDecimalPoint() {
    return toggleSegment(SEG_DP);
}

bool NJU3711_7Segment::getDecimalPointState() {
    return _decimalPointState;
}

// Display mode control
void NJU3711_7Segment::setDisplayMode(DisplayMode mode) {
    _displayMode = mode;
}

DisplayMode NJU3711_7Segment::getDisplayMode() {
    return _displayMode;
}

// Special display functions
bool NJU3711_7Segment::displayBlank() {
    if (_animationActive) stopAnimation();
    _decimalPointState = false;
    return write(applyDisplayMode(PATTERN_BLANK));
}

bool NJU3711_7Segment::displayAll() {
    if (_animationActive) stopAnimation();
    _decimalPointState = true;
    return write(applyDisplayMode(PATTERN_ALL));
}

bool NJU3711_7Segment::displayMinus() {
    if (_animationActive) stopAnimation();
    _decimalPointState = false;
    return write(applyDisplayMode(PATTERN_MINUS));
}

bool NJU3711_7Segment::displayUnderscore() {
    if (_animationActive) stopAnimation();
    _decimalPointState = false;
    return write(applyDisplayMode(PATTERN_UNDERSCORE));
}

bool NJU3711_7Segment::displayDegree() {
    if (_animationActive) stopAnimation();
    _decimalPointState = false;
    return write(applyDisplayMode(PATTERN_DEGREE));
}

bool NJU3711_7Segment::displayError() {
    if (_animationActive) stopAnimation();
    _decimalPointState = false;
    return write(applyDisplayMode(PATTERN_ERROR));
}

bool NJU3711_7Segment::displayOff() {
    // This would require multiple characters, so we'll just display 'F'
    return displayChar('F');
}

bool NJU3711_7Segment::displayOn() {
    // This would require multiple characters, so we'll just display 'O'
    return displayChar('O');
}

// Animation functions
bool NJU3711_7Segment::startAnimation(AnimationType type, unsigned long animDelay) {
    if (isBusy()) return false;
    
    _currentAnimation = type;
    _animationDelay = animDelay;
    _animationStep = 0;
    _animationActive = true;
    _lastAnimationUpdate = micros();
    return true;
}

void NJU3711_7Segment::stopAnimation() {
    _animationActive = false;
}

bool NJU3711_7Segment::isAnimating() {
    return _animationActive;
}

// Process animations
void NJU3711_7Segment::processAnimation() {
    if (!_animationActive) return;
    
    unsigned long currentTime = micros();
    if ((currentTime - _lastAnimationUpdate) < _animationDelay) return;
    
    switch (_currentAnimation) {
        case ANIM_ROTATE_CW: {
            // Rotate single segment clockwise
            uint8_t segments[] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F};
            uint8_t pattern = (1 << segments[_animationStep % 6]);
            write(applyDisplayMode(pattern));
            _animationStep++;
            break;
        }
        
        case ANIM_ROTATE_CCW: {
            // Rotate single segment counter-clockwise
            uint8_t segments[] = {SEG_A, SEG_F, SEG_E, SEG_D, SEG_C, SEG_B};
            uint8_t pattern = (1 << segments[_animationStep % 6]);
            write(applyDisplayMode(pattern));
            _animationStep++;
            break;
        }
        
        case ANIM_BLINK: {
            // Blink the current value
            if (_animationStep % 2 == 0) {
                displayDigit(_animationValue);
            } else {
                displayBlank();
            }
            _animationStep++;
            break;
        }
        
        case ANIM_CHASE: {
            // Chase segments around
            uint8_t pattern = 0;
            for (int i = 0; i < 3; i++) {
                int seg = (_animationStep + i) % 6;
                uint8_t segments[] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F};
                pattern |= (1 << segments[seg]);
            }
            write(applyDisplayMode(pattern));
            _animationStep++;
            break;
        }
        
        case ANIM_LOADING: {
            // Loading bar effect
            uint8_t pattern = 0;
            uint8_t segments[] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F};
            for (int i = 0; i <= (_animationStep % 7); i++) {
                if (i < 6) pattern |= (1 << segments[i]);
            }
            write(applyDisplayMode(pattern));
            _animationStep++;
            if (_animationStep >= 12) _animationStep = 0; // Reset after full cycle
            break;
        }
    }
    
    _lastAnimationUpdate = currentTime;
}

// Test function
bool NJU3711_7Segment::test() {
    if (isBusy()) return false;
    
    return startAnimation(ANIM_LOADING, 200000); // 200ms loading animation
}

// Countdown function
bool NJU3711_7Segment::countdown(uint8_t from, uint8_t to, unsigned long stepDelay) {
    // This is a simplified version - in a full implementation, you'd use the animation system
    // For now, just display the starting number
    return displayDigit(from);
}

// Count up function
bool NJU3711_7Segment::countup(uint8_t from, uint8_t to, unsigned long stepDelay) {
    // This is a simplified version - in a full implementation, you'd use the animation system
    // For now, just display the starting number
    return displayDigit(from);
}