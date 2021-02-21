#include <Arduino.h>
#include "MIDIUSB.h"

// enums
enum SOFTBUTTON_STATUS{
        OFF,
        ON,
        FADING_DOWN,
        FADING_UP
};

// settings
uint32_t DEFAULT_FADE_TIME = 3000; // ms
uint32_t MONITOR_UPDATE_INTERVAL = 100; // ms
const float ANALOG_IN_MAX_VALUE = 1023;
const float ANALOG_IN_MIN_VALUE = 0;
const float SOFTBUTTON_MAX_VALUE = 1023;
const float SOFTBUTTON_MIN_VALUE = 0;

// pinout
// pin 0, 1, 2 are used by the Control Chain shield
const uint8_t PIN_BUTTON_0 = 5;
const uint8_t PIN_BUTTON_1 = 3;
const uint8_t PIN_BUTTON_2 = 4;
const uint8_t PIN_BUTTON_3 = 7;
const uint8_t PIN_BUTTON_4 = 8;
const uint8_t PIN_BUTTON_5 = 12;
const uint8_t PIN_BUTTON_1_LED = 6;
const uint8_t PIN_BUTTON_2_LED = 9;
const uint8_t PIN_BUTTON_3_LED = 10;
const uint8_t PIN_BUTTON_4_LED = 11;
const uint8_t PIN_BUTTON_5_LED = 13;
const uint8_t PIN_POTI_0 = A0;
const uint8_t PIN_POTI_1 = A1;
const uint8_t PIN_POTI_2 = A2;
const uint8_t PIN_POTI_3 = A3;
const uint8_t PIN_POTI_4 = A4;
const uint8_t PIN_POTI_5 = A5;

// pinout collecitions
const uint8_t NUMBER_OF_BUTTONS = 4;
const uint8_t PIN_BUTTON[NUMBER_OF_BUTTONS] = {PIN_BUTTON_1, PIN_BUTTON_2, PIN_BUTTON_3, PIN_BUTTON_4};
const uint8_t PIN_BUTTON_LED[NUMBER_OF_BUTTONS] = {PIN_BUTTON_1_LED, PIN_BUTTON_2_LED, PIN_BUTTON_3_LED, PIN_BUTTON_4_LED};
const uint8_t NUMBER_OF_POTIS = 4;
const uint8_t PIN_POTI[NUMBER_OF_POTIS] = {PIN_POTI_0, PIN_POTI_1, PIN_POTI_2, PIN_POTI_3};

// vars
float value_softbutton[NUMBER_OF_BUTTONS];
uint8_t softbutton_status[NUMBER_OF_BUTTONS];
uint32_t softbutton_fade_timestamp[NUMBER_OF_BUTTONS];
float value_potis[NUMBER_OF_POTIS];

void noteOn(uint8_t channel, uint8_t pitch, uint8_t velocity) {
    midiEventPacket_t noteOn = {0x09, (uint8_t)(0x90 | channel), pitch, velocity};
    MidiUSB.sendMIDI(noteOn);
}

void noteOff(uint8_t channel, uint8_t pitch, uint8_t velocity) {
    midiEventPacket_t noteOff = {0x08, (uint8_t)(0x80 | channel), pitch, velocity};
    MidiUSB.sendMIDI(noteOff);
}

void controlChange(uint8_t channel, uint8_t control, uint8_t value) {
    midiEventPacket_t event = {0x0B, (uint8_t)(0xB0 | channel), control, value};
    MidiUSB.sendMIDI(event);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
void loop() {
//    noteOn(0, 1, 64);
    controlChange(1, 1, 100);
    MidiUSB.flush();
    delay(1000);
//    noteOff(0, 1, 64);
    controlChange(1, 1, 50);
    MidiUSB.flush();
    delay(1000);
    return;

    // set softbutton states
    for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
        if (softbutton_status[i] == OFF){
            if (digitalRead(PIN_BUTTON[i]) == false) {
                softbutton_status[i] = FADING_UP;
                softbutton_fade_timestamp[i] = millis();
            }
        } else if (softbutton_status[i] == FADING_UP) {
            if (digitalRead(PIN_BUTTON[i]) == true) {
                // button released
                softbutton_status[i] = ON;
                noteOn(0, i, 64);
            }
        } else if (softbutton_status[i] == ON) {
            if (digitalRead(PIN_BUTTON[i]) == false) {
                softbutton_status[i] = FADING_DOWN;
                softbutton_fade_timestamp[i] = millis();
            }
        } else if (softbutton_status[i] == FADING_DOWN) {
            if (digitalRead(PIN_BUTTON[i]) == true) {
                // button released
                softbutton_status[i] = OFF;
                noteOff(0, i, 64);
            }
        }
    }

    // update softbutton values and leds
    for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
        if (softbutton_status[i] == OFF){
            value_softbutton[i] = SOFTBUTTON_MIN_VALUE;
        } else if (softbutton_status[i] == FADING_UP) {
            if (value_softbutton[i] < SOFTBUTTON_MAX_VALUE) {
                value_softbutton[i] = (float) (millis() - softbutton_fade_timestamp[i]) / (float) DEFAULT_FADE_TIME * (SOFTBUTTON_MAX_VALUE - SOFTBUTTON_MIN_VALUE);
            } else {
                value_softbutton[i] = SOFTBUTTON_MAX_VALUE;
            }
        } else if (softbutton_status[i] == ON) {
            value_softbutton[i] = SOFTBUTTON_MAX_VALUE;
        } else if (softbutton_status[i] == FADING_DOWN) {
            if (value_softbutton[i] > SOFTBUTTON_MIN_VALUE) {
                value_softbutton[i] = (float) (softbutton_fade_timestamp[i] + DEFAULT_FADE_TIME - millis()) / (float) DEFAULT_FADE_TIME * (SOFTBUTTON_MAX_VALUE - SOFTBUTTON_MIN_VALUE);
            } else {
                value_softbutton[i] = SOFTBUTTON_MIN_VALUE;
            }
        }

        // update led
        analogWrite(PIN_BUTTON_LED[i], (uint16_t) value_softbutton[i] / 4); // must be converted to 0 .. 255
    }

    // update potis
    for (int i = 0; i < NUMBER_OF_POTIS; i++) {
        value_potis[i] = (float) analogRead(PIN_POTI[i]);
    }

    // TODO update midi
}

void setup() {
    Serial.begin(115200);

    // pinmode
    for (unsigned char i : PIN_BUTTON) {
        pinMode(i, INPUT_PULLUP);
    }

    // configure softbutton actuator
    for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
        // TODO
    }

    // configure poti actuator
    for (int i = 0; i < NUMBER_OF_POTIS; i++) {
        // TODO
    }
}
#pragma clang diagnostic pop
