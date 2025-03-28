#ifndef GLOBALS_H
#define GLOBALS_H

/*************************************************************************** */
/* PIN DEFINITIONS                                                           */
/*************************************************************************** */
#define NUM_BUTTONS         4
#define CHANNEL_BUTTON      27 //brown
#define START_STOP_BUTTON   26 //orange
#define MODE_BUTTON         32 //blue
#define CENTER_BUTTON       23

#define ENCODER_A           16
#define ENCODER_B           17

#define SEV_SEG_CLK         22
#define SEV_SEG_DATA        21

#define LEDS_DATA_OUT       14

/*************************************************************************** */
/* UTILITIES                                                                 */
/*************************************************************************** */

#define DIRECTION_UP        true
#define DIRECTION_DOWN      false
#define INCREMENT           true
#define DECREMENT           false
#define START               true
#define STOP                false            

typedef enum modes {
    EUCLIDEAN,
    MANUAL_VELOCITY,
    TEMPO,
    SWING,
    NUM_MODES   // sentinal value, not an actual mode
};

typedef enum playback_states {
    PLAYBACK_IDLE,
    PLAYBACK_START,
    PLAYBACK_PLAYING,
    PLAYBACK_STOP,
    PLAYBACK_OVERRIDE,
};

#define MAX_MAX_CHANNEL 8 //at some point we have to run out of memory or something.
#define MAX_BEATS       16
#define MAX_TEMPO       220
#define MIN_TEMPO       40
#define NUM_LEDS        MAX_BEATS
#define BEAT_NONE       -1

#define BPM_TO_MS(X)    (15000 / X)  // period = (60 s/min) * (1000 ms/s) / (tempo in bpm) / (4 subdivisions/beat)



#endif