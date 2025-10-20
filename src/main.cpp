#include <Arduino.h>

// ===== Segment Mapping =====
struct SegmentPair {
    int highPin;
    int lowPin;
};

// Charlieplexed segments
SegmentPair segments[] = {
    {5, 3},   // Segment 0 (A)
    {2, 5},   // Segment 1 (B)
    {3, 2},   // Segment 2 (C)
    {2, 3},   // Segment 3 (D)
    {4, 2},   // Segment 4 (E)
    {4, 3},   // Segment 5 (F)
    {7, 2},   // Segment 6 (G)
    {6, 2},   // Segment 7 (H)
    {5, 2},   // Segment 8 (Z)
    {6, 3},   // Segment 9 (I)
    {6, 4},   // Segment 10 (J)
    {7, 3},   // Segment 11 (K)
    {7, 4},   // Segment 12 (L)
    {5, 4},   // Segment 13 (M)
    {3, 4},   // Segment 14 (N)
    {2, 4},   // Segment 15 (O)
    {3, 5},   // Segment 16 Lightning
    {4, 5}    // Segment 17 Percent
};

// ===== Static segments (ALWAYS ON) =====
const int LIGHTNING = 16;
const int PERCENT = 17;

// ===== Left digit (1) =====
// If this mapping is wrong for your physical left-most "1", update it to the two segment indices
// that correspond to that digit on your board.
const int LEFT_DIGIT_1[] = {0, 1};
const int LEFT_DIGIT_1_LEN = sizeof(LEFT_DIGIT_1) / sizeof(LEFT_DIGIT_1[0]);

// ===== Middle and Right digits =====
// Provide exact counts so trailing zeros are NOT treated as valid segment indices.
const int MIDDLE_DIGITS[10][7] = {
    {2,3,4,6,7,8, -1},       // 0 (6 segments)
    {4,8, -1},               // 1 (2 segments)
    {2,4,5,6,7, -1},         // 2 (5 segments)
    {2,4,5,8,7, -1},         // 3 (5 segments)
    {3,5,4,8, -1},           // 4 (4 segments)
    {2,3,5,8,7, -1},         // 5 (5 segments)
    {2,3,5,8,7,6, -1},       // 6 (6 segments)
    {2,4,8, -1},             // 7 (3 segments)
    {2,3,4,5,6,8,7},         // 8 (7 segments)
    {2,3,4,5,8,7, -1}        // 9 (6 segments)
};
const int MIDDLE_LEN[10] = {6,2,5,5,4,5,6,3,7,6};

const int RIGHT_DIGITS[10][7] = {
    {9,10,11,13,14,15, -1},        // 0 (6)
    {11,15, -1},                   // 1 (2)
    {9,11,12,13,14, -1},           // 2 (5)
    {9,11,12,14,15, -1},           // 3 (5)
    {10,11,12,15, -1},             // 4 (4)
    {9,10,12,14,15, -1},           // 5 (5)
    {9,10,12,13,14,15, -1},        // 6 (6)
    {9,11,15, -1},                 // 7 (3)
    {9,10,11,12,13,14,15},         // 8 (7)
    {9,10,11,12,14,15, -1}         // 9 (6)
};
const int RIGHT_LEN[10] = {6,2,5,5,4,5,6,3,7,6};

// ===== Drive a single segment =====
void driveSegment(SegmentPair s, unsigned int pulse_us = 400) {
    // Turn off everything first (avoid ghosting)
    for (int i = 2; i <= 7; i++) pinMode(i, INPUT);

    pinMode(s.highPin, OUTPUT);
    digitalWrite(s.highPin, HIGH);
    pinMode(s.lowPin, OUTPUT);
    digitalWrite(s.lowPin, LOW);

    delayMicroseconds(pulse_us);

    // Turn off immediately after
    pinMode(s.highPin, INPUT);
    pinMode(s.lowPin, INPUT);
}

// ===== Refresh cycle: collect active segments and scan them quickly =====
void refreshDisplay(int left, int middle, int right, bool lightningOn, bool percentOn, unsigned long frame_ms = 10) {
    // Collect active segment indices
    int active[24];
    int count = 0;

    if (left >= 0) {
        for (int i = 0; i < LEFT_DIGIT_1_LEN; ++i) {
            active[count++] = LEFT_DIGIT_1[i];
        }
    }

    // middle
    if (middle >= 0 && middle < 10) {
        for (int i = 0; i < MIDDLE_LEN[middle]; ++i) {
            int seg = MIDDLE_DIGITS[middle][i];
            if (seg >= 0) active[count++] = seg;
        }
    }

    // right
    if (right >= 0 && right < 10) {
        for (int i = 0; i < RIGHT_LEN[right]; ++i) {
            int seg = RIGHT_DIGITS[right][i];
            if (seg >= 0) active[count++] = seg;
        }
    }

    // static icons
    if (lightningOn) active[count++] = LIGHTNING;
    if (percentOn)   active[count++] = PERCENT;

    if (count == 0) return;

    unsigned long start = millis();
    // cycle repeatedly for frame_ms total, scanning each active segment once per small pass
    while (millis() - start < frame_ms) {
        for (int i = 0; i < count; ++i) {
            driveSegment(segments[active[i]], 400); // 400us pulse per segment
        }
    }
}

// ===== Main setup =====
void setup() {
    Serial.begin(9600);
    Serial.println("Charlieplex counter 1–100 with constant lightning and percent (refactored)");
}

// ===== Main loop =====
void loop() {
    for (int num = 1; num <= 100; num++) {
        int left = -1;   // left off by default
        int middle = 0;
        int right = 0;

        if (num < 10) {
            middle = 0;
            right = num;
        } else if (num < 100) {
            middle = num / 10;
            right = num % 10;
        } else { // num == 100
            left = 1;
            middle = 0;
            right = 0;
        }

        Serial.print("Displaying ");
        Serial.println(num);

        unsigned long start = millis();
        while (millis() - start < 250) { // ~0.25s per number
            // each refresh call will scan all active segments (including icons) quickly
            refreshDisplay(left, middle, right, true, true, 8); // ~8ms frames repeated until 250ms done
        }
    }
}
