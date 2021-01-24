#include <Arduino.h>
#include "ControlChain.h"
#include "control_chain.h"

// enums
enum SOFTBUTTON_STATUS{
        OFF,
        ON,
        FADING_DOWN,
        FADING_UP
};

// settings
boolean DEBUG = true;
uint32_t DEFAULT_FADE_TIME = 3000; // ms
uint32_t MONITOR_UPDATE_INTERVAL = 100; // ms

// pinout
// pin 0, 1, 2 are used by the Control Chain shield
//const uint8_t PIN_BUTTON_0 = 2;
const uint8_t PIN_BUTTON_1 = 3;
const uint8_t PIN_BUTTON_2 = 4;
const uint8_t PIN_BUTTON_3 = 7;
const uint8_t PIN_BUTTON_4 = 8;
const uint8_t PIN_BUTTON_5 = 12;
//const uint8_t PIN_BUTTON_0_LED = 5;
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
const uint8_t NUMBER_OF_BUTTONS = 5;
const uint8_t PIN_BUTTON[NUMBER_OF_BUTTONS] = {PIN_BUTTON_1, PIN_BUTTON_2, PIN_BUTTON_3, PIN_BUTTON_4, PIN_BUTTON_5};
const uint8_t PIN_BUTTON_LED[NUMBER_OF_BUTTONS] = {PIN_BUTTON_1_LED, PIN_BUTTON_2_LED, PIN_BUTTON_3_LED, PIN_BUTTON_4_LED, PIN_BUTTON_5_LED};
const uint8_t NUMBER_OF_POTIS = 6;
const uint8_t PIN_POTI[NUMBER_OF_POTIS] = {PIN_POTI_0, PIN_POTI_1, PIN_POTI_2, PIN_POTI_3, PIN_POTI_4, PIN_POTI_5};

// instances
ControlChain cc;

// vars
float value_softbutton[NUMBER_OF_BUTTONS];
uint8_t softbutton_status[NUMBER_OF_BUTTONS];
uint32_t softbutton_fade_timestamp[NUMBER_OF_BUTTONS];
float value_potis[NUMBER_OF_POTIS];
uint32_t monitor_update_timestamp;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
void loop() {
    // set softbutton states
    for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
        if (softbutton_status[i] == OFF){
            if (digitalRead(PIN_BUTTON[i]) == false) {
                softbutton_status[i] = FADING_UP;
                softbutton_fade_timestamp[i] = millis();
                if (DEBUG) {
                    Serial.print("Button now fading up: ");
                    Serial.println(i);
                }
            }
        } else if (softbutton_status[i] == FADING_UP) {
            if (digitalRead(PIN_BUTTON[i]) == true) {
                softbutton_status[i] = ON;
                if (DEBUG) {
                    Serial.print("Button now on: ");
                    Serial.println(i);
                }
            }
        } else if (softbutton_status[i] == ON) {
            if (digitalRead(PIN_BUTTON[i]) == false) {
                softbutton_status[i] = FADING_DOWN;
                softbutton_fade_timestamp[i] = millis();
                if (DEBUG) {
                    Serial.print("Button now fading down: ");
                    Serial.println(i);
                }
            }
        } else if (softbutton_status[i] == FADING_DOWN) {
            if (digitalRead(PIN_BUTTON[i]) == true) {
                softbutton_status[i] = OFF;
                if (DEBUG) {
                    Serial.print("Button now off: ");
                    Serial.println(i);
                }
            }
        }
    }

    // update softbutton values and leds
    for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
        if (softbutton_status[i] == OFF){
            value_softbutton[i] = 0;
        } else if (softbutton_status[i] == FADING_UP) {
            value_softbutton[i] = (float) (millis() - softbutton_fade_timestamp[i]) / (float) DEFAULT_FADE_TIME * 1023;
        } else if (softbutton_status[i] == ON) {
            value_softbutton[i] = 1023;
        } else if (softbutton_status[i] == FADING_DOWN) {
            value_softbutton[i] = (float) (softbutton_fade_timestamp[i] + DEFAULT_FADE_TIME - millis()) / (float) DEFAULT_FADE_TIME * 1023;
        }

        // update led
        analogWrite(PIN_BUTTON_LED[i], (uint16_t) value_softbutton[i] / 4); // must be converted to 0 .. 255
    }

    // update potis
    for (int i = 0; i < NUMBER_OF_POTIS; i++) {
        value_potis[i] = (float) analogRead(PIN_POTI[i]);
    }

    // print values from time to time
    if (DEBUG && millis() - monitor_update_timestamp > MONITOR_UPDATE_INTERVAL) {
        Serial.print("Current softbutton states: ");
        for (float v : value_softbutton) {
            Serial.print(v);
            Serial.print(" ");
        }
        Serial.println();
    }

    cc.run();
}

void setup() {
    if (DEBUG) {
        Serial.begin(115200);
        uint32_t timestamp = millis();
        while (!Serial || timestamp + 2000 > millis()) {
            Serial.println("arduino is up...");
        }
    }

    // pinmode
    for (unsigned char i : PIN_BUTTON) {
        pinMode(i, INPUT_PULLUP);
    }

    // initialize control chain
    // note that control chain requires the Serial 0 and pin 2, which means
    // these peripherals won't be available to be used in your program
    Serial.print("Initializing Control Chain...");
    cc.begin();
    Serial.println(" OK");

    // define device name (1st parameter) and its URI (2nd parameter)
    // the URI must be an unique identifier for your device. A good practice
    // is to use a URL pointing to your project's code or documentation
    Serial.print("Setting Control Chain names...");
    const char *uri = "https://github.com/stephanbrunner/MOD-controller";
    cc_device_t *device = cc.newDevice("MOD-controller", uri);
    Serial.println(" OK");

    // configure softbutton actuator
    cc_actuator_config_t softbutton_actuator_config[NUMBER_OF_BUTTONS];
    for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
        String name = "softbutton ";
        name.concat(i);
        softbutton_actuator_config[i].type = CC_ACTUATOR_CONTINUOUS;
        softbutton_actuator_config[i].name = name.c_str();
        softbutton_actuator_config[i].value = &value_softbutton[i];
        softbutton_actuator_config[i].min = 0.0;
        softbutton_actuator_config[i].max = 1023.0;
        softbutton_actuator_config[i].supported_modes = CC_MODE_REAL | CC_MODE_INTEGER;
        softbutton_actuator_config[i].max_assignments = 1;

        // create and add actuator to device
        cc_actuator_t *actuator;
        actuator = cc.newActuator(&softbutton_actuator_config[i]);
        cc.addActuator(device, actuator);
    }

    // configure poti actuator
//    cc_actuator_config_t poti_actuator_config[NUMBER_OF_BUTTONS];
//    for (int i = 0; i < NUMBER_OF_POTIS; i++) {
//        String name = "potentiometer ";
//        name.concat(i);
//        poti_actuator_config[i].type = CC_ACTUATOR_CONTINUOUS;
//        poti_actuator_config[i].name = name.c_str();
//        poti_actuator_config[i].value = &value_potis[i];
//        poti_actuator_config[i].min = 0.0;
//        poti_actuator_config[i].max = 1023.0;
//        poti_actuator_config[i].supported_modes = CC_MODE_REAL | CC_MODE_INTEGER;
//        poti_actuator_config[i].max_assignments = 1;
//
//        // create and add actuator to device
//        cc_actuator_t *actuator;
//        actuator = cc.newActuator(&poti_actuator_config[i]);
//        cc.addActuator(device, actuator);
//    }
}
#pragma clang diagnostic pop
