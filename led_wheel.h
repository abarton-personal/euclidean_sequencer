#ifndef FRETBOARD_H
#define FRETBOARD_H

#include "globals.h"
#include <FastLED.h>

void init_leds();
void led_tasks();

void leds_show_beats(bool (*beatsarray)[MAX_BEATS], int chan);
void leds_show_measure_counter(int beatnum);

#endif