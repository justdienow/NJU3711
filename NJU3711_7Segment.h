/*
 * NJU3711_7Segment.h - 7-Segment Display Extension
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

#ifndef NJU3711_7SEGMENT_H
#define NJU3711_7SEGMENT_H

#include "NJU3711.h"

// Segment bit positions (matches your specific pin mapping)
#define SEG_A   7  // P8
#define SEG_B   5  // P6
#define SEG_C   2  // P3
#define SEG_D   1  // P2
#define SEG_E   3  // P4
#define SEG_F   6  // P7
#define SEG_G   4  // P5
#define SEG_DP  0  // P1

// Display modes
enum DisplayMode {
    ACTIVE_LOW,        // LEDs light when output is LOW (your case)
    ACTIVE_HIGH        // LEDs light when output is HIGH
};

// Animation types for effects
enum AnimationType {
    ANIM_ROTATE_CW,     // Clockwise rotation
    ANIM_ROTATE_CCW,    // Counter-clockwise rotation
    ANIM_BLINK,         // Blinking digit
    ANIM_FADE,          // Fade in/out effect
    ANIM_CHASE,         // Chase segments
    ANIM_LOADING        // Loading bar effect
};

class NJU3711_7Segment : public NJU3711 {

private:
    DisplayMode _displayMode;
    bool _decimalPointState;
    uint8_t _brightness;        // For future PWM brightness control
    
    // Animation state variables
    bool _animationActive;
    AnimationType _currentAnimation;
    unsigned long _animationDelay;
    unsigned long _lastAnimationUpdate;
    uint8_t _animationStep;
    uint8_t _animationValue;    // Current displayed value during animation
    
    // Internal helper methods
    void processAnimation();

protected:
    // Helper methods that derived classes can use
    uint8_t applyDisplayMode(uint8_t segmentData);
    uint8_t getDigitPattern(uint8_t digit);
    uint8_t getHexPattern(uint8_t hexValue);
    uint8_t getCharPattern(char character);
    DisplayMode getDisplayMode() const { return _displayMode; }
    
public:
    // Constructors
    NJU3711_7Segment(uint8_t dataPin, uint8_t clockPin, uint8_t strobePin, 
                     DisplayMode mode = ACTIVE_LOW);
    NJU3711_7Segment(uint8_t dataPin, uint8_t clockPin, uint8_t strobePin, uint8_t clearPin,
                     DisplayMode mode = ACTIVE_LOW);
    
    // Update method to handle animations (calls parent update + animation processing)
    void update();
    
    // Basic display functions
    bool displayDigit(uint8_t digit, bool showDP = false);
    bool displayHex(uint8_t hexValue, bool showDP = false);
    bool displayChar(char character, bool showDP = false);
    bool displayRaw(uint8_t segmentMask, bool showDP = false);
    
    // Individual segment control
    bool setSegment(uint8_t segment, bool state);
    bool clearSegment(uint8_t segment);
    bool toggleSegment(uint8_t segment);
    
    // Decimal point control
    bool setDecimalPoint(bool state);
    bool toggleDecimalPoint();
    bool getDecimalPointState();
    
    // Display mode control
    void setDisplayMode(DisplayMode mode);
    DisplayMode getDisplayMode();
    
    // Special display functions
    bool displayBlank();           // Turn off all segments
    bool displayAll();             // Turn on all segments (test pattern)
    bool displayMinus();           // Display minus sign
    bool displayUnderscore();      // Display underscore
    bool displayDegree();          // Display degree symbol
    
    // Multi-character patterns (using animation system)
    bool displayWord(const char* word, unsigned long charDelay = 500000); // Display scrolling text
    bool displayNumber(int number, unsigned long digitDelay = 300000);    // Display multi-digit number
    
    // Animation functions
    bool startAnimation(AnimationType type, unsigned long animDelay = 100000);
    void stopAnimation();
    bool isAnimating();
    
    // Utility functions
    bool test();                   // Run test pattern
    bool countdown(uint8_t from, uint8_t to = 0, unsigned long stepDelay = 1000000);
    bool countup(uint8_t from = 0, uint8_t to = 9, unsigned long stepDelay = 1000000);
    
    // Error display
    bool displayError();           // Display "E" for error
    bool displayOff();             // Display "OFF"
    bool displayOn();              // Display "ON" (uses animation)
};

#endif // NJU3711_7SEGMENT_H