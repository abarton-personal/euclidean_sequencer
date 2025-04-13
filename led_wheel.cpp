#include "led_wheel.h"
#include "globals.h"


CRGB rgbleds[NUM_LEDS];

CRGB chan_colors[MAX_MAX_CHANNEL] = {
    CRGB::Red,
    CRGB::Blue,
    CRGB::Green,
    CRGB::Salmon,
    CRGB::Cyan,
    CRGB::Olive,
    CRGB::Lavender,
    CRGB::Peru
};

// private function declarations
void do_a_barrel_roll();
void reset_leds();


// public function definitions

void init_leds(){
    FastLED.addLeds<NEOPIXEL, LEDS_DATA_OUT>(rgbleds, NUM_LEDS);  // GRB ordering is assumed
    FastLED.setBrightness(10);
    reset_leds();
    do_a_barrel_roll();
}


void leds_show_playback(bool (*beatsarray)[MAX_BEATS], int chan, int beatnum){
    // light up selected beats, turn off unselected
    for (int i=0; i<NUM_LEDS; i++){
        if (beatsarray[chan][i]){
            rgbleds[i] = chan_colors[chan];
        } else {
            rgbleds[i] = CRGB::Black;
        }
    }
    // also light up the current beat in the measure
    if (beatnum >= 0 && beatnum < MAX_BEATS)
        rgbleds[beatnum] = CRGB::White;
    FastLED.show();
}

// private function definitions

void reset_leds(){
    for (int i=0; i<NUM_LEDS; i++){
        rgbleds[i] = CRGB::Black;
    }
}


void do_a_barrel_roll(){
    int prev_led = 0;
    for (int j=0; j<3; j++)
        for (int i=0; i<NUM_LEDS; i++){
            rgbleds[i] = chan_colors(j);
            if (i == 0) prev_led = NUM_LEDS-1;
            rgbleds[prev_led] = CRGB::Black;
            delay(80);
        }

    reset_leds();
}