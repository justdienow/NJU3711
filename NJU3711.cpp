/*
 * NJU3711.cpp - Non-Blocking Arduino Driver Implementation
 * 8-bit Serial to Parallel Converter
 * 
 * This library provides a non-blocking interface to control the NJU3711
 * serial-to-parallel converter IC from an Arduino.
 * 
 * Author: justdienow
 * Version: 2.0 (Non-blocking)
 */

#include "NJU3711.h"

// Constructors
NJU3711::NJU3711(uint8_t dataPin, uint8_t clockPin, uint8_t strobePin, uint8_t clearPin) {
    _dataPin = dataPin;
    _clockPin = clockPin;
    _strobePin = strobePin;
    _clearPin = clearPin;
    _currentData = 0;
    _state = NJU3711_IDLE;
    _stepDelay = 1; // 1 microsecond default (well within 5MH spec)
    _clockState = false;
    _queueHead = 0;
    _queueTail = 0;
    _queueSize = 0;
    _testPatternStep = 0;
    _testPatternType = 0;
    _testPatternDelay = 500000; // 500ms default
}

// Constructor for hardware-strapped CLR pin (always HIGH)
NJU3711::NJU3711(uint8_t dataPin, uint8_t clockPin, uint8_t strobePin) {
    _dataPin = dataPin;
    _clockPin = clockPin;
    _strobePin = strobePin;
    _clearPin = 255; // Special value indicating CLR is hardware strapped
    _currentData = 0;
    _state = NJU3711_IDLE;
    _stepDelay = 1; // 1 microsecond default (well within 5MH spec)
    _clockState = false;
    _queueHead = 0;
    _queueTail = 0;
    _queueSize = 0;
    _testPatternStep = 0;
    _testPatternType = 0;
    _testPatternDelay = 500000; // 500ms default
}

// Initialie the NJU3711
void NJU3711::begin() {
    // Set pin modes
    pinMode(_dataPin, OUTPUT);
    pinMode(_clockPin, OUTPUT);
    pinMode(_strobePin, OUTPUT);
    if (_clearPin != 255) {
        pinMode(_clearPin, OUTPUT);
    }
    
    // Set initial states
    digitalWrite(_dataPin, LOW);
    digitalWrite(_clockPin, LOW);
    digitalWrite(_strobePin, HIGH);  // STB high for shifting
    if (_clearPin != 255) {
        digitalWrite(_clearPin, HIGH);   // CLR high for normal operation
    }
    
    _lastUpdateTime = micros();
    _state = NJU3711_IDLE;
    
    // Queue a clear operation to start clean
    clear();
}

// Must be called regularly in main loop
void NJU3711::update() {
    processStateMachine();
}

// Check if device is busy
bool NJU3711::isBusy() {
    return (_state != NJU3711_IDLE) || (_queueSize > 0);
}

// Set timing delay between operations
void NJU3711::setStepDelay(unsigned long delayMicros) {
    _stepDelay = delayMicros;
}

// Check if enough time has passed for next operation
bool NJU3711::isTimingMet() {
    return (micros() - _lastUpdateTime) >= _stepDelay;
}

// Update clock state
void NJU3711::updateClock(bool state) {
    if (_clockState != state) {
        digitalWrite(_clockPin, state ? HIGH : LOW);
        _clockState = state;
        _lastUpdateTime = micros();
    }
}

// Process the state machine
void NJU3711::processStateMachine() {
    if (!isTimingMet()) return;
    
    switch (_state) {
        case NJU3711_IDLE: {
            // Check if there are queued operations
            NJU3711_Operation nextOp;
            uint8_t nextData;
            if (dequeueOperation(nextOp, nextData)) {
                startOperation(nextOp, nextData);
            }
            break;
        }
        
        case NJU3711_SHIFTING: {
            // Shift out bits one at a time
            if (_clockState == false) {
                // Set data bit for current position
                bool bitValue = (_shiftData >> _bitIndex) & 0x01;
                digitalWrite(_dataPin, bitValue ? HIGH : LOW);
                updateClock(true); // Rising edge shifts data
            } else {
                updateClock(false); // Return clock to low
                _bitIndex--;
                
                if (_bitIndex < 0) {
                    // Finished shifting all bits
                    if (_currentOperation == NJU3711_OP_WRITE) {
                        _state = NJU3711_LATCHING;
                    } else {
                        _state = NJU3711_IDLE; // Shift only operation
                    }
                }
            }
            break;
        }
        
        case NJU3711_LATCHING: {
            // Pulse STB low to latch data
            static bool latchStep = false;
            if (!latchStep) {
                digitalWrite(_strobePin, LOW);
                latchStep = true;
                _lastUpdateTime = micros();
            } else {
                digitalWrite(_strobePin, HIGH);
                latchStep = false;
                _state = NJU3711_IDLE;
            }
            break;
        }
        
        case NJU3711_CLEARING: {
            // Hardware clear: Pulse CLR low to clear outputs
            // Software clear: Write 0x00 to all outputs
            if (_clearPin != 255) {
                // Hardware clear available
                static bool clearStep = false;
                if (!clearStep) {
                    digitalWrite(_clearPin, LOW);
                    clearStep = true;
                    _lastUpdateTime = micros();
                } else {
                    digitalWrite(_clearPin, HIGH);
                    clearStep = false;
                    _currentData = 0;
                    _state = NJU3711_IDLE;
                }
            } else {
                // CLR pin strapped HIGH - use software clear (write 0x00)
                // Redirect to write operation
                if (enqueueOperation(NJU3711_OP_WRITE, 0x00)) {
                    _currentData = 0;
                }
                _state = NJU3711_IDLE;
            }
            break;
        }
        
        case NJU3711_TEST_PATTERN: {
            // Handle test patterns
            if ((micros() - _lastUpdateTime) >= _testPatternDelay) {
                switch (_testPatternType) {
                    case 1: // All on/off alternating
                        write(_testPatternStep ? 0xFF : 0x00);
                        _testPatternStep = !_testPatternStep;
                        break;
                    case 2: // Alternating bits
                        write(_testPatternStep ? 0xAA : 0x55);
                        _testPatternStep = !_testPatternStep;
                        break;
                    case 3: // Walking bit
                        write(1 << (_testPatternStep % 8));
                        _testPatternStep++;
                        break;
                    case 4: // Binary counter
                        write(_testPatternStep);
                        _testPatternStep++;
                        break;
                }
                _lastUpdateTime = micros();
            }
            break;
        }
    }
}

// Queue management
bool NJU3711::enqueueOperation(NJU3711_Operation op, uint8_t data) {
    if (_queueSize >= 8) return false; // Queue full
    
    _operationQueue[_queueTail].operation = op;
    _operationQueue[_queueTail].data = data;
    _operationQueue[_queueTail].valid = true;
    
    _queueTail = (_queueTail + 1) % 8;
    _queueSize++;
    return true;
}

bool NJU3711::dequeueOperation(NJU3711_Operation& op, uint8_t& data) {
    if (_queueSize == 0) return false;
    
    op = _operationQueue[_queueHead].operation;
    data = _operationQueue[_queueHead].data;
    _operationQueue[_queueHead].valid = false;
    
    _queueHead = (_queueHead + 1) % 8;
    _queueSize--;
    return true;
}

void NJU3711::startOperation(NJU3711_Operation op, uint8_t data) {
    _currentOperation = op;
    
    switch (op) {
        case NJU3711_OP_WRITE:
        case NJU3711_OP_SHIFT_ONLY:
            _shiftData = data;
            _currentData = data;
            _bitIndex = 7;
            _state = NJU3711_SHIFTING;
            digitalWrite(_strobePin, HIGH); // Ensure STB is high for shifting
            break;
            
        case NJU3711_OP_LATCH_ONLY:
            _state = NJU3711_LATCHING;
            break;
            
        case NJU3711_OP_CLEAR:
            _state = NJU3711_CLEARING;
            break;
    }
}

// Public interface methods
bool NJU3711::write(uint8_t data) {
    if (_state == NJU3711_TEST_PATTERN) stopTestPattern();
    return enqueueOperation(NJU3711_OP_WRITE, data);
}

bool NJU3711::writeImmediate(uint8_t data) {
    return write(data);
}

bool NJU3711::shift(uint8_t data) {
    if (_state == NJU3711_TEST_PATTERN) stopTestPattern();
    return enqueueOperation(NJU3711_OP_SHIFT_ONLY, data);
}

bool NJU3711::latch() {
    if (_state == NJU3711_TEST_PATTERN) stopTestPattern();
    return enqueueOperation(NJU3711_OP_LATCH_ONLY);
}

bool NJU3711::clear() {
    if (_state == NJU3711_TEST_PATTERN) stopTestPattern();
    return enqueueOperation(NJU3711_OP_CLEAR);
}

bool NJU3711::setBit(uint8_t bitPosition) {
    if (bitPosition > 7) return false;
    if (_state == NJU3711_TEST_PATTERN) stopTestPattern();
    uint8_t newData = _currentData | (1 << bitPosition);
    return write(newData);
}

bool NJU3711::clearBit(uint8_t bitPosition) {
    if (bitPosition > 7) return false;
    if (_state == NJU3711_TEST_PATTERN) stopTestPattern();
    uint8_t newData = _currentData & ~(1 << bitPosition);
    return write(newData);
}

bool NJU3711::toggleBit(uint8_t bitPosition) {
    if (bitPosition > 7) return false;
    if (_state == NJU3711_TEST_PATTERN) stopTestPattern();
    uint8_t newData = _currentData ^ (1 << bitPosition);
    return write(newData);
}

bool NJU3711::writeBit(uint8_t bitPosition, bool value) {
    if (value) {
        return setBit(bitPosition);
    } else {
        return clearBit(bitPosition);
    }
}

uint8_t NJU3711::getCurrentData() {
    return _currentData;
}

bool NJU3711::startTestPattern(uint8_t patternType, unsigned long patternDelay) {
    if (isBusy()) return false;
    
    _testPatternType = patternType;
    _testPatternDelay = patternDelay;
    _testPatternStep = 0;
    _state = NJU3711_TEST_PATTERN;
    _lastUpdateTime = micros();
    return true;
}

void NJU3711::stopTestPattern() {
    if (_state == NJU3711_TEST_PATTERN) {
        _state = NJU3711_IDLE;
    }
}

uint8_t NJU3711::getQueueSize() {
    return _queueSize;
}

void NJU3711::clearQueue() {
    _queueHead = 0;
    _queueTail = 0;
    _queueSize = 0;
    for (int i = 0; i < 8; i++) {
        _operationQueue[i].valid = false;
    }
}