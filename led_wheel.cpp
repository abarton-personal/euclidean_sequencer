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

void init_leds(){
    FastLED.addLeds<NEOPIXEL, LEDS_DATA_OUT>(rgbleds, NUM_LEDS);  // GRB ordering is assumed
    FastLED.setBrightness(100); 
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