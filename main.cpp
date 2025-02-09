#include <Arduino.h>
#include <Adafruit_CircuitPlayground.h>

// =============== INSTRUCTIONS =========================
// With USB port pointing towards user, press right button to record locking gesture
// After recording is done, red LED will turn on signifying that the system is locked
// While system is locked, you are unable to record a new gesture
// Use the left button to start the unlocking gesture
// If red LED turns off, system is successfully unlocked
// If red LED stays on, system is still locked, try gesture again
// After 3 incorrect attempts, system stays locked for period of time
// Force unlock the board by flipping the slide switch in both directions
// Slide switch must be on negative (-) side for proper functionality


#define SEQUENCE_LENGTH 50
#define MAX_ATTEMPTS 3
#define LOCKOUT_DURATION 300000  // 5 minutes in milliseconds

float storedSequence[SEQUENCE_LENGTH][3];  // [ax,ay,az]
int storedLength = 0;
bool isRecording = false;
bool isChecking = false;
bool systemLocked = false;
bool inLockout = false;

// Attempt tracking
int failedAttempts = 0;
unsigned long lockoutStartTime = 0;

void checkSequence();
void recordSequence();
float compareSequences(float* recorded, float* stored, int length);
void enterLockout();
void checkLockoutStatus();
void clearAllPixels();  // New helper function

void setup() {
    CircuitPlayground.begin();
    Serial.begin(9600);
    CircuitPlayground.redLED(false);
    systemLocked = false;
    clearAllPixels();
}

// New helper function to ensure all pixels are cleared
void clearAllPixels() {
    for(int i = 0; i < 10; i++) {
        CircuitPlayground.setPixelColor(i, 0, 0, 0);
    }
}

void loop() {
    // Check if we're in lockout and handle timeout
    checkLockoutStatus();

    // Left button for recording gesture and locking
    if (CircuitPlayground.leftButton()) {
        if (!isRecording && !isChecking && !inLockout && !systemLocked) {  // Added !systemLocked check
            recordSequence();
            systemLocked = true;
            failedAttempts = 0;
            CircuitPlayground.redLED(true);
            Serial.println("System Locked with new gesture");
        } else if (systemLocked) {
            // Provide feedback that system is already locked
            Serial.println("System already locked - cannot record new gesture");
            // Visual feedback - quick red flash
            for(int i = 0; i < 3; i++) {
                CircuitPlayground.setPixelColor(0, 255, 0, 0);
                delay(100);
                CircuitPlayground.setPixelColor(0, 0, 0, 0);
                delay(100);
            }
        }
    }

    // Right button for checking gesture and unlocking
    if (CircuitPlayground.rightButton()) {
        if (systemLocked && !isRecording && !isChecking && storedLength > 0) {
            if (!inLockout) {
                checkSequence();
            } else {
                // Show lockout status
                Serial.print("System is locked out for ");
                Serial.print((LOCKOUT_DURATION - (millis() - lockoutStartTime)) / 1000);
                Serial.println(" more seconds");
                
                // Visual feedback for lockout
                for(int i = 0; i < 3; i++) {
                    CircuitPlayground.setPixelColor(0, 255, 0, 0);
                    delay(100);
                    CircuitPlayground.setPixelColor(0, 0, 0, 0);
                    delay(100);
                }
            }
        }
    }

    // Override with slide switch
    if (CircuitPlayground.slideSwitch()) {
        CircuitPlayground.redLED(false);
        systemLocked = false;
        inLockout = false;
        failedAttempts = 0;
        clearAllPixels();
    }
}

void checkLockoutStatus() {
    if (inLockout) {
        // Check if lockout period is over
        if (millis() - lockoutStartTime >= LOCKOUT_DURATION) {
            inLockout = false;
            failedAttempts = 0;
            Serial.println("Lockout period ended. System ready for new attempts.");
            clearAllPixels();
        } else {
            // Pulse red LED during lockout
            int pulseValue = (sin(millis() / 500.0) + 1) * 127;
            CircuitPlayground.setPixelColor(0, pulseValue, 0, 0);
        }
    }
}

void enterLockout() {
    inLockout = true;
    lockoutStartTime = millis();
    Serial.println("Too many failed attempts. System locked for 5 minutes.");
    
    // Visual indication of lockout
    for(int i = 0; i < 10; i++) {
        CircuitPlayground.setPixelColor(i, 255, 0, 0);
    }
}

void recordSequence() {
    Serial.println("Recording started - 5 second gesture");
    isRecording = true;
    int sampleCount = 0;
    unsigned long startTime = millis();
    
    clearAllPixels();
    // Start recording indicator
    CircuitPlayground.setPixelColor(0, 255, 165, 0);
    
    while ((millis() - startTime) < 5000 && sampleCount < SEQUENCE_LENGTH) {
        storedSequence[sampleCount][0] = CircuitPlayground.motionX();
        storedSequence[sampleCount][1] = CircuitPlayground.motionY();
        storedSequence[sampleCount][2] = CircuitPlayground.motionZ();
        
        // Visual feedback - light up pixels based on motion
        int intensity = abs(storedSequence[sampleCount][0]) * 255;
        CircuitPlayground.setPixelColor(sampleCount % 10, intensity, 0, intensity);
        
        Serial.print("Sample "); 
        Serial.print(sampleCount);
        Serial.print(": X=");
        Serial.print(storedSequence[sampleCount][0], 2);
        Serial.print(" Y=");
        Serial.print(storedSequence[sampleCount][1], 2);
        Serial.print(" Z=");
        Serial.println(storedSequence[sampleCount][2], 2);
        
        sampleCount++;
        delay(20);
    }
    
    storedLength = sampleCount;
    isRecording = false;
    
    clearAllPixels();
    
    // Completion animation - Green blink
    for(int i = 0; i < 2; i++) {
        CircuitPlayground.setPixelColor(0, 0, 255, 0);
        delay(200);
        CircuitPlayground.setPixelColor(0, 0, 0, 0);
        delay(200);
    }
    
    Serial.print("Recording complete. Collected ");
    Serial.print(sampleCount);
    Serial.println(" samples");
}

void checkSequence() {
    Serial.println("Checking gesture - perform the same motion");
    Serial.print("Attempt ");
    Serial.print(failedAttempts + 1);
    Serial.print(" of ");
    Serial.println(MAX_ATTEMPTS);
    
    isChecking = true;
    float currentSequence[SEQUENCE_LENGTH][3];
    int sampleCount = 0;
    unsigned long startTime = millis();
    
    clearAllPixels();
    // Start checking indicator - Purple
    CircuitPlayground.setPixelColor(0, 255, 0, 255);
    
    while ((millis() - startTime) < 5000 && sampleCount < storedLength) {
        currentSequence[sampleCount][0] = CircuitPlayground.motionX();
        currentSequence[sampleCount][1] = CircuitPlayground.motionY();
        currentSequence[sampleCount][2] = CircuitPlayground.motionZ();
        
        // Visual feedback
        int intensity = abs(currentSequence[sampleCount][0]) * 255;
        CircuitPlayground.setPixelColor(sampleCount % 10, intensity, 0, intensity);
        
        Serial.print("Check Sample "); 
        Serial.print(sampleCount);
        Serial.print(": X=");
        Serial.print(currentSequence[sampleCount][0], 2);
        Serial.print(" Y=");
        Serial.print(currentSequence[sampleCount][1], 2);
        Serial.print(" Z=");
        Serial.println(currentSequence[sampleCount][2], 2);
        
        sampleCount++;
        delay(20);
    }
    
    float similarity = compareSequences((float*)currentSequence, (float*)storedSequence, storedLength);
    Serial.print("Gesture match: ");
    Serial.print(similarity * 100);
    Serial.println("%");
    
    clearAllPixels();
    
    if (similarity > 0.85) {  // 85% match threshold
        Serial.println("Gesture Matched! System Unlocked");
        systemLocked = false;
        failedAttempts = 0;
        CircuitPlayground.redLED(false);
        // Success animation - green spiral
        for(int i = 0; i < 10; i++) {
            CircuitPlayground.setPixelColor(i, 0, 255, 0);
            delay(50);
        }
        delay(500);
        clearAllPixels();
    } else {
        failedAttempts++;
        Serial.print("Gesture Did Not Match - ");
        Serial.print(MAX_ATTEMPTS - failedAttempts);
        Serial.println(" attempts remaining");
        
        if (failedAttempts >= MAX_ATTEMPTS) {
            enterLockout();
        } else {
            // Failure animation - red flash
            for(int i = 0; i < 3; i++) {
                for(int j = 0; j < 10; j++) {
                    CircuitPlayground.setPixelColor(j, 255, 0, 0);
                }
                delay(100);
                clearAllPixels();
                delay(100);
            }
        }
    }
    
    clearAllPixels();
    isChecking = false;
}

float compareSequences(float* recorded, float* stored, int length) {
    float matchCount = 0;
    float tolerance = 0.3;  // Tolerance for matching
    
    // Normalize the sequences first
    float maxRecorded = 0;
    float maxStored = 0;
    
    // Use normalization method
    for(int i = 0; i < length * 3; i++) {
        maxRecorded = max(maxRecorded, abs(recorded[i]));
        maxStored = max(maxStored, abs(stored[i]));
    }
    
    for(int i = 0; i < length * 3; i++) {
        float normalizedRecorded = recorded[i] / maxRecorded;
        float normalizedStored = stored[i] / maxStored;
        
        if(fabs(normalizedRecorded - normalizedStored) < tolerance) {
            matchCount++;
        }
    }
    
    return matchCount / (length * 3);
}