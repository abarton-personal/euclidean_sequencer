#ifndef FRETBOARD_H
#define FRETBOARD_H

#include <FastLED.h>

void init_leds();

void leds_show_playback(bool *beatsarray, int beatnum);

#endif