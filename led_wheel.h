#ifndef FRETBOARD_H
#define FRETBOARD_H

#include "globals.h"
#include <FastLED.h>

void init_leds();

void leds_show_playback(bool (*beatsarray)[MAX_BEATS], int chan, int beatnum);

#endif