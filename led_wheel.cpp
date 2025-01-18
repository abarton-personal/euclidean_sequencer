#include "led_wheel.h"
#include "globals.h"


CRGB rgbleds[NUM_LEDS];

void init_leds(){
    FastLED.addLeds<NEOPIXEL, LEDS_DATA_OUT>(rgbleds, NUM_LEDS);  // GRB ordering is assumed
    FastLED.setBrightness(255); 
}

void leds_show_playback(bool *beatsarray, int beatnum){
    // light up selected beats, turn off unselected
    for (int i=0; i<NUM_LEDS; i++){
        if (beatsarray[i]){
            // TODO: color depending on the channel
            rgbleds[i] = CRGB::Red;
        } else {
            rgbleds[i] = CRGB::Black;
        }
    }
    // also light up the current beat in the measure
    rgbleds[beatnum] = CRGB::Red;
    FastLED.show();
}