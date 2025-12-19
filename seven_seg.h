#ifndef SEVEN_SEG_H
#define SEVEN_SEG_H
#include <TM1637Display.h>

/*************************************************************************** */
/* CONSTANTS                                                                 */
/*************************************************************************** */

const uint8_t SEG_DONE[] = {
	SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
	SEG_C | SEG_E | SEG_G,                           // n
	SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
	};

const uint8_t SEG_EUCL[] = {
    SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,            // E
    SEG_C | SEG_D | SEG_E,                            // u
    SEG_A | SEG_F | SEG_E | SEG_D,                    // c
    SEG_E | SEG_F | SEG_D                             // L
};

const uint8_t SEG_VOL[] = {
    SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,            // V
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,    // O
    SEG_E | SEG_F | SEG_D,                            // L
    0
};

const uint8_t SEG_RATE[] = {
    SEG_E | SEG_G,                                    // r
    SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,    // A
    SEG_D | SEG_E | SEG_F | SEG_G,                    // t
    SEG_A | SEG_D | SEG_E | SEG_F | SEG_G             // E
};

const uint8_t SEG_SHUF[] = {
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,            // S
    SEG_C | SEG_E | SEG_F | SEG_G,                    // h
    SEG_C | SEG_D | SEG_E,                            // u
    SEG_A | SEG_E | SEG_F | SEG_G                     // F
};

const uint8_t SEG_SYNC[] = {
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,            // S
    SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,            // y
    SEG_C | SEG_E | SEG_G,                            // n
    SEG_A | SEG_F | SEG_E | SEG_D,                    // c
};

const uint8_t SEG_NO[] = {
    SEG_C | SEG_E | SEG_G,                            // n
    SEG_C | SEG_D | SEG_E | SEG_G,                    // o
    0,
    0
};

const uint8_t SEG_CH[] = {
    SEG_A | SEG_F | SEG_E | SEG_D,                    // c
    SEG_C | SEG_E | SEG_F | SEG_G,                    // h
    0,
    0
};

const uint8_t SEG_CONN[] = {
    SEG_A | SEG_F | SEG_E | SEG_D,                    // c
    SEG_C | SEG_D | SEG_E | SEG_G,                    // o
    SEG_C | SEG_E | SEG_G,                            // n
    SEG_C | SEG_E | SEG_G,                            // n
};

const uint8_t SEG_POOP[] = {
    SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
    SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,
};

/*************************************************************************** */
/* FUNCTION DEFINITIONS                                                      */
/*************************************************************************** */

void sev_seg_power(bool on);
void sev_seg_show_digit(int num);
void sev_seg_show_channel_num(int num);
void sev_seg_display_word(const uint8_t* word);
void demo_sev_seg();

#endif