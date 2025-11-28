#ifndef FRETBOARD_H
#define FRETBOARD_H

#include "globals.h"
#include <FastLED.h>
#include <array>

void init_leds();
void led_tasks();

void leds_show_beats(const std::array<bool,MAX_BEATS>& beatsarray, int chan);
void leds_show_measure_counter(int beatnum);

#endif