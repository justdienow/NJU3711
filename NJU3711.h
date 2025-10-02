/*
 * NJU3711.h - Non-Blocking Arduino Driver Header
 * 8-bit Serial to Parallel Converter
 * 
 * This library provides a non-blocking interface to control the NJU3711
 * serial-to-parallel converter IC from an Arduino.
 * 
 * Author: justdienow
 * Version: 2.0 (Non-blocking)
 */

#ifndef NJU3711_H
#define NJU3711_H

#include <Arduino.h>

// Operation states for the state machine
enum NJU3711_State {
    NJU3711_IDLE,
    NJU3711_SHIFTING,
    NJU3711_LATCHING,
    NJU3711_CLEARING,
    NJU3711_TEST_PATTERN
};

// Operation types
enum NJU3711_Operation {
    NJU3711_OP_WRITE,
    NJU3711_OP_SHIFT_ONLY,
    NJU3711_OP_LATCH_ONLY,
    NJU3711_OP_CLEAR,
    NJU3711_OP_TEST_PATTERN
};

class NJU3711 {
private:
    uint8_t _dataPin;       // DATA pin
    uint8_t _clockPin;      // CLK pin
    uint8_t _strobePin;     // STB pin
    uint8_t _clearPin;      // CLR pin (255 if hardware strapped HIGH)
    uint8_t _currentData;   // Current data stored in the device
    
    // State machine variables
    NJU3711_State _state;
    NJU3711_Operation _currentOperation;
    uint8_t _shiftData;     // Data being shifted
    int8_t _bitIndex;       // Current bit being shifted (7 to 0)
    unsigned long _lastUpdateTime;
    unsigned long _stepDelay;   // Minimum delay between steps (microseconds)
    bool _clockState;       // Current clock state
    
    // Test pattern variables
    uint8_t _testPatternStep;
    uint8_t _testPatternType;
    unsigned long _testPatternDelay;
    
    // Queue for operations
    struct OperationQueue {
        NJU3711_Operation operation;
        uint8_t data;
        bool valid;
    } _operationQueue[8];  // Small queue for operations
    uint8_t _queueHead;
    uint8_t _queueTail;
    uint8_t _queueSize;
    
    // Internal methods
    void processStateMachine();
    bool enqueueOperation(NJU3711_Operation op, uint8_t data = 0);
    bool dequeueOperation(NJU3711_Operation& op, uint8_t& data);
    void startOperation(NJU3711_Operation op, uint8_t data = 0);
    bool isTimingMet();
    void updateClock(bool state);

public:
    // Constructors
    NJU3711(uint8_t dataPin, uint8_t clockPin, uint8_t strobePin, uint8_t clearPin);
    NJU3711(uint8_t dataPin, uint8_t clockPin, uint8_t strobePin); // CLR strapped HIGH
    
    // Initialize the NJU3711
    void begin();
    
    // Must be called regularly in main loop
    void update();
    
    // Check if device is busy
    bool isBusy();
    
    // Non-blocking write operations (return false if busy, true if queued)
    bool write(uint8_t data);
    bool writeImmediate(uint8_t data);
    
    // Non-blocking bit operations
    bool setBit(uint8_t bitPosition);
    bool clearBit(uint8_t bitPosition);
    bool toggleBit(uint8_t bitPosition);
    bool writeBit(uint8_t bitPosition, bool value);
    
    // Non-blocking utility operations
    bool shift(uint8_t data);   // Shift without latching
    bool latch();               // Latch current register
    bool clear();               // Software clear (writes 0x00) - hardware clear not available if CLR strapped
    
    // Get current data value (immediate)
    uint8_t getCurrentData();
    
    // Non-blocking test patterns
    bool startTestPattern(uint8_t patternType, unsigned long patternDelay = 500000); // delay in microseconds
    void stopTestPattern();
    
    // Set timing (microseconds between operations)
    void setStepDelay(unsigned long delayMicros);
    
    // Queue management
    uint8_t getQueueSize();
    void clearQueue();
};

#endif // NJU3711_H