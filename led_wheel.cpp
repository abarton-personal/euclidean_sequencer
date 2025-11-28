#include "led_wheel.h"
#include "globals.h"
#include <Arduino.h>
#include <array>

#define MIN_UPDATE_INTERVAL 30

CRGB rgbleds[NUM_LEDS];

CRGB chan_colors[MAX_MAX_CHANNEL] = {
    CRGB::Red,
    CRGB::Blue,
    CRGB::Green,
    CRGB::Pink,
    CRGB::Cyan,
    CRGB::Olive,
    CRGB::Lavender,
    CRGB::Peru
};

static bool change_requested = false;
static unsigned long update_time;


// private function declarations
void do_a_barrel_roll();
void reset_leds();


// public function definitions

void init_leds(){
    FastLED.addLeds<NEOPIXEL, LEDS_DATA_OUT>(rgbleds, NUM_LEDS);  // GRB ordering is assumed
    FastLED.setBrightness(10);
    reset_leds();
    do_a_barrel_roll();
    update_time = millis();
}

void led_tasks(){
    if (millis() - update_time > MIN_UPDATE_INTERVAL){
        if (change_requested){
            FastLED.show();
            update_time = millis();
            change_requested = false;
        }
    }
}

void leds_show_beats(const std::array<bool,MAX_BEATS>& beatsarray, int chan){
    // light up selected beats, turn off unselected
    for (int i=0; i<NUM_LEDS; i++){
        if (beatsarray[i]){
            rgbleds[i] = chan_colors[chan];
        } else {
            rgbleds[i] = CRGB::Black;
        }
    }
    change_requested = true;
}

void leds_show_measure_counter(int beatnum){
    // light up the current beat in the measure
    if (beatnum >= 0 && beatnum < MAX_BEATS)
        rgbleds[beatnum] = CRGB::White;
    change_requested = true;
}

// private function definitions

void reset_leds(){
    for (int i=0; i<NUM_LEDS; i++){
        rgbleds[i] = CRGB::Black;
    }
    FastLED.show();
}


void do_a_barrel_roll(){
    int prev_led = 0;
    for (int j=0; j<3; j++)
        for (int i=0; i<NUM_LEDS; i++){
            rgbleds[i] = chan_colors[j];
            if (i == 0) prev_led = NUM_LEDS-1;
            else prev_led = i-1;
            rgbleds[prev_led] = CRGB::Black;
            FastLED.show();
            delay(50);
        }

    reset_leds();
}

