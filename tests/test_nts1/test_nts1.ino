// Test Program for FRMIDIConBoard1 + NTS-1

// Target: KORG NTS-1 (Synthesizer Kit)
// https://www.korg.com/products/dj/nts_1/

// Configuration
// +--------------+ MIDI    MIDI +-----------+
// |              | OUT       IN |           |
// | FRMIDICon    +------------->+   NTS-1   |
// |      Board1  |    3.5mm     |           |
// +--------------+  TRS cable   +-----------+
// 

// Board: Sparkfun Pro Micro (5V/16MHz)
// See the following tutorial on setting up the Arduino IDE
// https://learn.sparkfun.com/tutorials/pro-micro--fio-v3-hookup-guide/introduction

// Required Library:
// MIDI Library by Francois Best, lathoub

#include <Arduino.h>
#include <MIDI.h>

// #define MEASURE_ITERATION
// #define DEBUG

// Potentiometers
const uint8_t pot_pin[] = { A6, A3, A2, A1, A0, A7, A8, A9 };
#define NPOTS ((int)(sizeof(pot_pin) / sizeof(uint8_t)))
uint8_t pot_v[NPOTS];        // current values
uint8_t pot_v1[NPOTS];       // previous values
uint8_t pot_rv1[NPOTS];      // previous raw values

// Buttons
const uint8_t btn_pin[] = { 15, 16, 14 };
#define NBTNS ((int)(sizeof(btn_pin) / sizeof(uint8_t)))
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

// NTS-1 Control Change
const uint8_t cc[] = {
    0x36,  // OSCILLATOR SHAPE
    0x2B,  // FILTER CUTOFF
    0x2C,  // FILTER RESONANCE
    0x2D,  // FILTER SWEEP DEPTH
    0x2E,  // FILTER SWEEP RATE
    0x10,  // EG ATTACK
    0x13,  // EG RELEASE
    0x1D   // MOD FX DEPTH
};

// "On the Run" sequence
const uint8_t note[] = {
    40, // E2
    43, // G2
    45, // A2
    43, // G2
    50, // D3
    48, // C3
    50, // D3
    52  // E3
};
#define NNOTES ((int)(sizeof(note) / sizeof(uint8_t)))

int bpm = 165;
unsigned long clk_us;
unsigned long t_next;

const uint8_t synth_ch = 1;

MIDI_CREATE_DEFAULT_INSTANCE();

void sendNoteOn(uint8_t note, uint8_t vel, uint8_t ch) {
#if defined(DEBUG)
    Serial.print("noteOn ");
    Serial.print(note, 16);
    Serial.print(" ");
    Serial.println(vel);
#endif
    MIDI.sendNoteOn(note, vel, ch);
}

void sendNoteOff(uint8_t note, uint8_t vel, uint8_t ch) {
#if defined(DEBUG)
    Serial.print("noteOff ");
    Serial.print(note, 16);
    Serial.print(" ");
    Serial.println(vel);
#endif
    MIDI.sendNoteOff(note, vel, ch);
}

void sendCC(uint8_t cc, uint8_t arg, uint8_t ch) {
#if defined(DEBUG)
    Serial.print("cc ");
    Serial.print(cc, 16);
    Serial.print(" ");
    Serial.println(arg, 16);
#endif
    MIDI.sendControlChange(cc, arg, ch);
}

void setup_NTS1(uint8_t ch) {
    // ALL NOTES OFF
    for (int i = 0; i < NNOTES; i++) {
        sendNoteOff(note[i], 0, ch);
    }

    // oscillator type: square wave
    sendCC(0x35, 0x32, ch);
    // oscillator shape
    sendCC(0x36, 0x20, ch);
    // oscillator alt
    sendCC(0x37, 0x00, ch);
    // oscillator LFO rate
    sendCC(0x18, 0x00, ch);
    // oscillator LFO depth (pitch 0x3F-sd0x00, shape 0x41-0x7F)
    sendCC(0x1A, 0x40, ch);

    // filter type: low-pass pole 2
    sendCC(0x2A, 0x00, ch);
    // filter cutoff
    sendCC(0x2B, 0x20, ch);
    // filter resonance
    sendCC(0x2C, 0x15, ch);
    // filter sweep depth
    sendCC(0x2D, 0x40, ch);
    // filter sweep rate
    sendCC(0x2E, 0x00, ch);

    // EG TYPE (AHR)
    sendCC(0x0E, 0x00, ch);
    // EG ATTACK
    sendCC(0x10, 0x20, ch);
    // EG RELEASE
    sendCC(0x13, 0x40, ch);
    // TREMOLO RATE
    // sendCC(0x14, 0x40, ch);
    // TREMOLO DEPTH
    sendCC(0x15, 0x00, ch);

    // MOD FX TYPE
    sendCC(0x58, 0x7F, ch);  // FLANGER
    // MOD FX TIME
    sendCC(0x1C, 0x24, ch);
    // MOD FX DEPTH
    sendCC(0x1D, 0x20, ch);

    // DELAY FX TYPE
    sendCC(0x59, 0x00, ch);  // OFF

    // REVERB FX TYPE
    sendCC(0x5A, 0x15, ch);  // HALL
    // REVERB FX TIME
    sendCC(0x22, 0x30, ch);
    // REVERB FX DEPTH
    sendCC(0x23, 0x40, ch);
    // REVERB FX MIX
    sendCC(0x24, 0x20, ch);
}

void setup() {
#if defined(DEBUG) || defined(MEASURE_ITERATION)
    Serial.begin(115200);
    // while (!Serial);
#endif

    ADCSRA &= 0xf8;
    ADCSRA |= 0x04;

    MIDI.begin(4);

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

    setup_NTS1(synth_ch);

    clk_us = 2500000 / bpm;
    t_next = micros() + clk_us;

#if defined(MEASURE_ITERATION)
    t1 = millis();
    count = 0;
#endif
}

void loop() {
    static bool startreq = false, stopreq = false, 
                playing = false, noteon = false;
    if (startreq) {
        playing = true;
        startreq = false;
    }

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

    // send control changes
    for (int i = 0; i < NPOTS; i++) {
        if (pot_v[i] != pot_v1[i]) {
            sendCC(cc[i], pot_v[i], synth_ch);
        }
    }

    // start/stop playing & reset
    for (int i = 0; i < NBTNS; i++) {
        if (btn_v1[i] && !btn_v[i]) {
            if (i == 0) {  // start playing
                startreq = true;
            }
            else if (i == 1 && playing) {  // stop playing
                stopreq = true;
            }
            else if (i == 2) {  // reset
                setup_NTS1(synth_ch);
                startreq = stopreq = playing = false;
            }
        }
        else if (!btn_v1[i] && btn_v[i]) {
        }
    }

    // change BPM
    if (encA_v1 && !encA_v && encB_v1 && encB_v) {
        if (bpm < 240) {
            bpm += 1;
            clk_us = 2500000 / bpm;
        }
#if defined(DEBUG)
        Serial.print("BPM: ");
        Serial.println(bpm);
#endif
    }
    else if (encA_v1 && encA_v && encB_v1 && !encB_v) {
        if (bpm > 1) {
            bpm -= 1;
            clk_us = 2500000 / bpm;
        }
#if defined(DEBUG)
        Serial.print("BPM: ");
        Serial.println(bpm);
#endif
    }

    // play sequence of notes
    unsigned long t_curr = micros();
    if (t_curr >= t_next) {
        static int bc = 0;
        static int i = 0;
        if (bc == 0) {
            digitalWrite(led_pin, HIGH);
        }
        else if (bc == 5) {
            digitalWrite(led_pin, LOW);
        }
        if (playing && bc % 6 == 0) {
            sendNoteOn(note[i], 127, 1);
            noteon = true;
        }
        else if (playing && noteon && bc % 6 == 4) {
            sendNoteOff(note[i], 127, 1);
            noteon = false;
            i = (i + 1) % NNOTES;
            if (stopreq) {
                playing = false;
                stopreq = false;
            }
        }
        bc = (bc + 1) % 24;
        t_next = t_curr + clk_us;
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
