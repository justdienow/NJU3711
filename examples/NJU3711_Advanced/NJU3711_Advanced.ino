/*
 * NJU3711_Advanced.ino - Advanced Example
 * 
 * This example demonstrates advanced features including:
 * - Multiple NJU3711 instances
 * - Queue management  
 * - Timing control
 * - Complex patterns
 * - Performance monitoring
 * 
 * Author: justdienow
 * Version: 1.0
 * 
 */

#include <NJU3711.h>

// Create two NJU3711 instances for demonstration
NJU3711 expander1(2, 3, 4);    // First device
NJU3711 expander2(5, 6, 7);    // Second device (different pins)

// Performance monitoring
unsigned long operationCount = 0;
unsigned long startTime = 0;

void setup() {
    Serial.begin(9600);
    Serial.println("NJU3711 Advanced Example");
    Serial.println("========================");
    
    // Initialize both expanders
    expander1.begin();
    expander2.begin();
    
    // Set different timing for demonstration
    expander1.setStepDelay(1);    // Fast timing (1μs)
    expander2.setStepDelay(10);   // Slower timing (10μs)
    
    Serial.println("Two NJU3711 devices initialized");
    Serial.println("Device 1: Fast timing (1μs)");
    Serial.println("Device 2: Slow timing (10μs)");
    Serial.println();
    
    Serial.println("Commands:");
    Serial.println("1 - Synchronized patterns");
    Serial.println("2 - Independent patterns"); 
    Serial.println("3 - Queue stress test");
    Serial.println("4 - Performance benchmark");
    Serial.println("5 - Complex wave patterns");
    Serial.println("c - Clear both devices");
    Serial.println("s - Show status");
    Serial.println();
    
    startTime = millis();
}

void loop() {
    // CRITICAL: Update both devices
    expander1.update();
    expander2.update();
    
    // Handle commands
    handleAdvancedCommands();
    
    // Monitor performance
    monitorPerformance();
}

void handleAdvancedCommands() {
    if (Serial.available()) {
        char cmd = Serial.read();
        
        switch (cmd) {
            case '1':
                runSynchronizedPatterns();
                break;
                
            case '2':
                runIndependentPatterns();
                break;
                
            case '3':
                runQueueStressTest();
                break;
                
            case '4':
                runPerformanceBenchmark();
                break;
                
            case '5':
                runComplexWavePatterns();
                break;
                
            case 'c':
            case 'C':
                expander1.clear();
                expander2.clear();
                Serial.println("Both devices cleared");
                break;
                
            case 's':
            case 'S':
                showDetailedStatus();
                break;
        }
        
        while (Serial.available()) Serial.read();
    }
}

void runSynchronizedPatterns() {
    Serial.println("Running synchronized patterns...");
    
    for (int i = 0; i < 256; i++) {
        // Wait for both devices to be ready
        while (expander1.isBusy() || expander2.isBusy()) {
            expander1.update();
            expander2.update();
        }
        
        // Send complementary patterns
        expander1.write(i);
        expander2.write(~i);  // Inverted pattern
        
        operationCount += 2;
        delay(50);
    }
    
    Serial.println("Synchronized patterns complete");
}

void runIndependentPatterns() {
    Serial.println("Running independent patterns for 10 seconds...");
    
    unsigned long endTime = millis() + 10000; // Run for 10 seconds
    uint8_t counter1 = 0, counter2 = 0;
    unsigned long lastUpdate1 = 0, lastUpdate2 = 0;
    
    while (millis() < endTime) {
        expander1.update();
        expander2.update();
        
        // Device 1: Fast binary counter (every 100ms)
        if (!expander1.isBusy() && (millis() - lastUpdate1) > 100) {
            expander1.write(counter1++);
            lastUpdate1 = millis();
            operationCount++;
        }
        
        // Device 2: Slow walking bit (every 300ms)  
        if (!expander2.isBusy() && (millis() - lastUpdate2) > 300) {
            expander2.write(1 << (counter2 % 8));
            counter2++;
            lastUpdate2 = millis();
            operationCount++;
        }
    }
    
    Serial.println("Independent patterns complete");
}

void runQueueStressTest() {
    Serial.println("Running queue stress test...");
    
    // Fill queues rapidly
    unsigned long startOps = operationCount;
    
    for (int burst = 0; burst < 5; burst++) {
        Serial.print("Burst ");
        Serial.print(burst + 1);
        Serial.print(": ");
        
        // Try to queue many operations quickly
        int queued1 = 0, queued2 = 0;
        
        for (int i = 0; i < 20; i++) {
            if (expander1.write(random(256))) queued1++;
            if (expander2.write(random(256))) queued2++;
        }
        
        Serial.print("Dev1 queued: ");
        Serial.print(queued1);
        Serial.print(", Dev2 queued: ");
        Serial.println(queued2);
        
        operationCount += queued1 + queued2;
        
        // Wait for queues to empty
        while (expander1.isBusy() || expander2.isBusy()) {
            expander1.update();
            expander2.update();
        }
        
        delay(500);
    }
    
    unsigned long completedOps = operationCount - startOps;
    Serial.print("Stress test complete. Operations: ");
    Serial.println(completedOps);
}

void runPerformanceBenchmark() {
    Serial.println("Running performance benchmark...");
    
    unsigned long benchmarkStart = millis();
    unsigned long benchmarkOps = operationCount;
    
    // Rapid-fire operations for 5 seconds
    unsigned long endTime = millis() + 5000;
    
    while (millis() < endTime) {
        expander1.update();
        expander2.update();
        
        if (!expander1.isBusy()) {
            expander1.write(random(256));
            operationCount++;
        }
        
        if (!expander2.isBusy()) {
            expander2.write(random(256));
            operationCount++;
        }
    }
    
    // Wait for completion
    while (expander1.isBusy() || expander2.isBusy()) {
        expander1.update();
        expander2.update();
    }
    
    unsigned long benchmarkTime = millis() - benchmarkStart;
    unsigned long benchmarkCompleted = operationCount - benchmarkOps;
    
    Serial.print("Benchmark results:");
    Serial.print(" Time: ");
    Serial.print(benchmarkTime);
    Serial.print("ms, Operations: ");
    Serial.print(benchmarkCompleted);
    Serial.print(", Rate: ");
    Serial.print((benchmarkCompleted * 1000.0) / benchmarkTime);
    Serial.println(" ops/sec");
}

void runComplexWavePatterns() {
    Serial.println("Running complex wave patterns for 15 seconds...");
    
    unsigned long endTime = millis() + 15000;
    float phase1 = 0, phase2 = 0;
    
    while (millis() < endTime) {
        expander1.update();
        expander2.update();
        
        if (!expander1.isBusy()) {
            // Sine wave pattern
            uint8_t pattern1 = 0;
            for (int bit = 0; bit < 8; bit++) {
                float angle = phase1 + (bit * PI / 4);
                if (sin(angle) > 0) {
                    pattern1 |= (1 << bit);
                }
            }
            expander1.write(pattern1);
            phase1 += 0.1;
            operationCount++;
        }
        
        if (!expander2.isBusy()) {
            // Sawtooth wave pattern
            uint8_t pattern2 = 0;
            for (int bit = 0; bit < 8; bit++) {
                float sawtooth = fmod(phase2 + (bit * 0.25), 1.0);
                if (sawtooth > 0.5) {
                    pattern2 |= (1 << bit);
                }
            }
            expander2.write(pattern2);
            phase2 += 0.05;
            operationCount++;
        }
        
        delay(20); // ~50Hz update rate
    }
    
    Serial.println("Complex wave patterns complete");
}

void showDetailedStatus() {
    Serial.println("\n=== Detailed Status ===");
    
    Serial.print("Device 1 - Busy: ");
    Serial.print(expander1.isBusy() ? "Yes" : "No");
    Serial.print(", Queue: ");
    Serial.print(expander1.getQueueSize());
    Serial.print(", Data: 0b");
    Serial.println(expander1.getCurrentData(), BIN);
    
    Serial.print("Device 2 - Busy: ");
    Serial.print(expander2.isBusy() ? "Yes" : "No");
    Serial.print(", Queue: ");
    Serial.print(expander2.getQueueSize());
    Serial.print(", Data: 0b");
    Serial.println(expander2.getCurrentData(), BIN);
    
    unsigned long runtime = millis() - startTime;
    Serial.print("Runtime: ");
    Serial.print(runtime / 1000);
    Serial.print("s, Total operations: ");
    Serial.print(operationCount);
    Serial.print(", Average rate: ");
    Serial.print((operationCount * 1000.0) / runtime);
    Serial.println(" ops/sec");
    
    Serial.print("Free RAM: ");
    Serial.print(freeRam());
    Serial.println(" bytes");
    Serial.println();
}

void monitorPerformance() {
    static unsigned long lastMonitor = 0;
    
    // Show performance stats every 30 seconds
    if (millis() - lastMonitor > 30000) {
        Serial.print("Performance update - Operations: ");
        Serial.print(operationCount);
        Serial.print(", Rate: ");
        Serial.print((operationCount * 1000.0) / (millis() - startTime));
        Serial.println(" ops/sec");
        lastMonitor = millis();
    }
}

// Utility function to check free RAM
int freeRam() {
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}