// input_listener.cpp
#include "input_listener.h"
#include "globals.h"
#include "AiEsp32RotaryEncoder.h"
#include "Arduino.h"

#define DEBOUNCE_TIME 50
#define ENCODER_READ_INTERVAL 50

#define ROTARY_ENCODER_A_PIN ENCODER_A
#define ROTARY_ENCODER_B_PIN ENCODER_B
#define ROTARY_ENCODER_BUTTON_PIN CENTER_BUTTON
#define ROTARY_ENCODER_VCC_PIN -1 
#define ROTARY_ENCODER_STEPS 4

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);


// ENCODER STUFF
void IRAM_ATTR readEncoderISR()
{
	rotaryEncoder.readEncoder_ISR();
}


void rotary_encoder_init(){
    rotaryEncoder.begin();
	rotaryEncoder.setup(readEncoderISR);
    bool circleValues = true;
	rotaryEncoder.setBoundaries(0, 16, circleValues); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)
    rotaryEncoder.setAcceleration(0);
}


void rotary_loop()
{
	if (rotaryEncoder.encoderChanged())
	{
		Serial.print("Value: ");
		Serial.println(rotaryEncoder.readEncoder());
	}
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
                    if (button.callback) {
                        button.callback();
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

