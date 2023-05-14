// Test Program for FRMIDIConBoard1

// This program checks that the potentiometers, buttons, etc.
// on the board are connected correctly.
// After compiling and uploading this program, open the Arduino
// IDE's serial monitor (set to 115200 bps) and check its output
// resulting from operating the knobs and buttons on the board.
// This program does not use any MIDI library. It simply outputs
// to the USB serial port.

// Board: Sparkfun Pro Micro (5V/16MHz)
// See the following tutorial on setting up the Arduino IDE
// https://learn.sparkfun.com/tutorials/pro-micro--fio-v3-hookup-guide/introduction

#include <Arduino.h>

// #define MEASURE_ITERATION

// Potentiometers
#define NPOTS 8
const uint8_t pot_pin[] = { A6, A3, A2, A1, A0, A7, A8, A9 };
uint8_t pot_v[NPOTS];        // current values
uint8_t pot_v1[NPOTS];       // previous values
uint8_t pot_rv1[NPOTS];      // previous raw values

// Buttons
#define NBTNS 3
const uint8_t btn_pin[] = { 15, 16, 14 };
uint8_t btn_v[NBTNS];        // current values
uint8_t btn_v1[NBTNS];       // previous values
uint8_t btn_rv1[NBTNS];      // previous raw values

// Rotary Encoder
const uint8_t encA_pin = 5;
const uint8_t encB_pin = 7;
uint8_t encA_v, encB_v;      // current values
uint8_t encA_v1, encB_v1;    // previous values
uint8_t encA_rv1, encB_rv1;  // previous raw values

// LED
const uint8_t led_pin = 10;

#if defined(MEASURE_ITERATION)
unsigned long t1;
unsigned long count;
#endif

void setup() {
    Serial.begin(115200);
    while (!Serial);

    // initialize pot values
    for (int i = 0; i < NPOTS; i++) {
        uint8_t v = (uint8_t)(analogRead(pot_pin[i]) / 8);
        pot_v[i] = pot_v1[i] = pot_rv1[i] = v;
    }
    // initialize button values
    for (int i = 0; i < NBTNS; i++) {
        pinMode(btn_pin[i], INPUT_PULLUP);
        btn_v[i] = btn_v1[i] = btn_rv1[i] = digitalRead(btn_pin[i]);
    }
    // initialize rotary encoder values
    pinMode(encA_pin, INPUT_PULLUP);
    pinMode(encB_pin, INPUT_PULLUP);
    encA_v = encA_v1 = encA_rv1 = digitalRead(encA_pin);
    encB_v = encB_v1 = encB_rv1 = digitalRead(encB_pin);

    pinMode(led_pin, OUTPUT);

#if defined(MEASURE_ITERATION)
    t1 = millis();
    count = 0;
#endif
}

void loop() {
    static int bcount = 0;

    // read pot values
    for (int i = 0; i < NPOTS; i++) {
        uint8_t rv = (uint8_t)(analogRead(pot_pin[i]) / 8);
        if (rv == pot_rv1[i]) {
            pot_v[i] = rv;
        }
        pot_rv1[i] = rv;
    }
    // read button values
    for (int i = 0; i < NBTNS; i++) {
        uint8_t rv = (uint8_t)digitalRead(btn_pin[i]);
        if (rv == btn_rv1[i]) {
            btn_v[i] = rv;
        }
        btn_rv1[i] = rv;
    }
    // read encoder
    {
        uint8_t encA_rv = digitalRead(encA_pin);
        uint8_t encB_rv = digitalRead(encB_pin);
        if (encA_rv == encA_rv1) {
            encA_v = encA_rv;
        }
        if (encB_rv == encB_rv1) {
            encB_v = encB_rv;
        }
        encA_rv1 = encA_rv;
        encB_rv1 = encB_rv;
    }

    // display changes of pot values
    for (int i = 0; i < NPOTS; i++) {
        if (pot_v[i] != pot_v1[i]) {
            Serial.print("Pot");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(pot_v[i]);
        }
    }

    // display changes of buttons
    for (int i = 0; i < NBTNS; i++) {
        if (btn_v1[i] && !btn_v[i]) {
            Serial.print("Button");
            Serial.print(i);
            Serial.println(": Pressed");
            bcount++;
            // digitalWrite(led_pin, HIGH);
        }
        else if (!btn_v1[i] && btn_v[i]) {
            Serial.print("Button");
            Serial.print(i);
            Serial.println(": Released");
            bcount--;
            // digitalWrite(led_pin, LOW);
        }
    }

    // display the rotation of the encoder
    if (encA_v1 && !encA_v && encB_v1 && encB_v) {
        Serial.println("Encoder: CW");
    }
    else if (encA_v1 && encA_v && encB_v1 && !encB_v) {
        Serial.println("Encoder: CCW");
    }

    if (bcount == 0) {
        digitalWrite(led_pin, LOW);
    }
    else if (bcount > 0 && bcount <= NBTNS) {
        digitalWrite(led_pin, HIGH);
    }
    else {
        Serial.println("Invalid button state");
    }
    
    // update previous values
    for (int i = 0; i < NPOTS; i++) {
        pot_v1[i] = pot_v[i];        
    }
    for (int i = 0; i < NBTNS; i++) {
        btn_v1[i] = btn_v[i];
    }
    encA_v1 = encA_v;
    encB_v1 = encB_v;

#if defined(MEASURE_ITERATION)
    if (++count >= 1000) {
        unsigned long t = millis();
        Serial.print(t - t1);
        Serial.println("us/iter");
        t1 = t;
        count = 0;
    }
#endif
}
