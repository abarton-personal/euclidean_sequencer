
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

#define SEV_SEG_CLK         14
#define SEV_SEG_DATA        12

#define LEDS_DATA_OUT       7//5

/*************************************************************************** */
/* UTILITIES                                                                 */
/*************************************************************************** */

#define DIRECTION_UP        true
#define DIRECTION_DOWN      false

typedef enum modes {
    EUCLIDEAN,
    MANUAL_VELOCITY,
    TEMPO,
    SWING,
    NUM_MODES   // sentinal value, not an actual mode
};

#define MAX_MAX_CHANNEL 16 //at some point we have to run out of memory or something.