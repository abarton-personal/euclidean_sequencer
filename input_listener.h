// input_listener.h
#ifndef INPUT_LISTENER_H
#define INPUT_LISTENER_H

#include <Arduino.h>
#include "globals.h"

enum ButtState {
    OPEN,
    PRESS_DEBOUNCE,
    CLOSED,
    RELEASE_DEBOUNCE
};

typedef void (*ButtonCallback)();

struct Button {
    uint8_t pin;
    ButtState state;
    unsigned long timer;
    ButtonCallback pressCallback;
    ButtonCallback releaseCallback;
};

void buttons_init();
void updateButtons();

typedef void (*encoderCallback)(bool);
void registerEncTurnCallback(encoderCallback callback);
void registerButtonCallbacks(uint8_t pin, ButtonCallback onPress, ButtonCallback onRelease);

void rotary_encoder_init();
void rotary_loop();

#endif // INPUT_LISTENER_H
