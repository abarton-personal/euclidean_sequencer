// input_listener.cpp
#include "input_listener.h"
#include "globals.h"
#include "AiEsp32RotaryEncoder.h"
#include "Arduino.h"

#define DEBOUNCE_TIME               50

#define ROTARY_ENCODER_A_PIN        ENCODER_A
#define ROTARY_ENCODER_B_PIN        ENCODER_B
#define ROTARY_ENCODER_BUTTON_PIN   CENTER_BUTTON //doesn't matter, it's controlled like the other buttons, but the encoder init expects a value
#define ROTARY_ENCODER_VCC_PIN      -1 
#define ROTARY_ENCODER_STEPS        4
#define MAX_ENC_VAL                 15
#define CIRCLE_ENC_VALUES           true

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

encoderCallback enc_callback = NULL;


/*************************************************************************** */
/* FUNCTION DEFINITIONS                                                      */
/*************************************************************************** */
// ISR for encoder 
void IRAM_ATTR readEncoderISR()
{
	rotaryEncoder.readEncoder_ISR();
}

// initialize encoder configs
void rotary_encoder_init(){
    rotaryEncoder.begin();
	rotaryEncoder.setup(readEncoderISR);
	// rotaryEncoder.setBoundaries(0, MAX_ENC_VAL, CIRCLE_ENC_VALUES); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)
    rotaryEncoder.setAcceleration(0);
}

// repeatedly check encoder for changes 
void rotary_loop()
{
    int16_t encoderDelta = rotaryEncoder.encoderChanged();
    if (encoderDelta == 0)
        return;
    if (encoderDelta > 0){
        if (enc_callback != NULL) {
            enc_callback(DIRECTION_UP);
        }
    }
    if (encoderDelta < 0){
        if (enc_callback != NULL) {
            enc_callback(DIRECTION_DOWN);
        }
    }

}

void registerEncTurnCallback(encoderCallback callback) {
    enc_callback = callback;
}


// BUTTON STUFF
void updateButton(Button& button) {
    unsigned long currentTime = millis();
    switch (button.state) {
        case OPEN:
            if (digitalRead(button.pin) == LOW) {
                button.state = PRESS_DEBOUNCE;
                button.timer = currentTime;
            }
            break;
        case PRESS_DEBOUNCE:
            if (currentTime - button.timer >= DEBOUNCE_TIME) {
                if (digitalRead(button.pin) == LOW) {
                    button.state = CLOSED;
                    if (button.pressCallback) {
                        button.pressCallback();
                    }
                } else {
                    button.state = OPEN;
                }
            }
            break;
        case CLOSED:
            if (digitalRead(button.pin) == HIGH) {
                button.state = RELEASE_DEBOUNCE;
                button.timer = currentTime;
            }
            break;
        case RELEASE_DEBOUNCE:
            if (currentTime - button.timer >= DEBOUNCE_TIME) {
                if (digitalRead(button.pin) == HIGH) {
                    button.state = OPEN;
                    if (button.releaseCallback) {
                        button.releaseCallback();
                    }
                } else {
                    button.state = CLOSED;
                }
            }
            break;
    }
}

void updateButtons(Button* buttons, size_t numButtons) {
    for (size_t i = 0; i < numButtons; ++i) {
        updateButton(buttons[i]);
    }
}

