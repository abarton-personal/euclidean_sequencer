#include <Arduino.h>
#include <BLEMidi.h>
#include "seven_seg.h"
#include "input_listener.h"
#include "globals.h"
#include "led_wheel.h"


/*************************************************************************** */
/* Global Variables                                                         */
/*************************************************************************** */
// the currently active channel
static uint8_t channel = 0;
// max number of channels to cycle through. This can be adjusted up to MAX_MAX_CHANNELS
static uint8_t max_channels = 4;
// currently active mode
volatile static modes device_mode = EUCLIDEAN;
// certain actions depend on whether the center button is held down or not
volatile static ButtState center_button_pos = OPEN;

// tempo that is used. This is overridden by MIDI sync clicks from DAW
static uint8_t custom_tempo = 120;
// how much to swing. 0 is straight 16ths / no swing
static int16_t swing_offset = 0;
// period between beats, calculated form BPM
static uint32_t time_per_beat_ms = BPM_TO_MS(custom_tempo);
static uint32_t time_per_sync_pulse = time_per_beat_ms / 6;
// which of the 16 beats is being played right now
static uint8_t measure_counter = BEAT_NONE;
static uint8_t sync_pulses = 0;
// playback state machine state
static playback_states pbs = PLAYBACK_IDLE;
// flags to start and stop playback
static bool start_playback_fl = false;
static bool stop_playback_fl = false;

// how many beats are currently "on" for each channel
static uint8_t num_beats[MAX_MAX_CHANNEL] = {0};
// 2D array to hold the on and off beats for each channel
static bool beats[MAX_MAX_CHANNEL][MAX_BEATS] = {{false}};

// whether each channel is a different MIDI channel, or dif
static midi_chan_split_type midi_chan_split = NOTE_PER_CHAN;
// MIDI note sent by channel 1 when sending NOTE_PER_CHAN
// 36 = C2 which is usually the kick drum on a drum pad
static int base_note = 36;

/*************************************************************************** */
/* Private Function Declarations                                             */
/*************************************************************************** */

void euclidean(int num_points, bool* chanbeats, int size);
void rotate_beats(bool up);
void increase_tempo(int mod);
void increase_swing(int mod);
void send_midi_notes(uint8_t which_beat);
void toggle_midi_chan_split();
void next_pulse();
playback_states get_playback_state();


/*************************************************************************** */
/* Private Function Definitions                                              */
/*************************************************************************** */

// cycle to the next mode when mode button pressed
void cycle_device_mode(){
  device_mode = static_cast<modes>((device_mode + 1) % NUM_MODES);
}

// debug print the array beats[chan]
void print_beats(uint8_t chan){
    leds_show_playback(beats, chan, measure_counter);
    printf("beats for channel %d: [", channel);
    for(int i=0; i<15; i++){
        printf("%d, ", beats[channel][i]);
    }
    printf("%d]\n", beats[channel][15]);
}

// increment or decrement the number of beats for the current channel
void inc_dec_beats(bool up){
    if(up==true){
        if (num_beats[channel] < MAX_BEATS){
            num_beats[channel]++;
        }
    }
    else {
        if (num_beats[channel] > 0){
            num_beats[channel]--;
        }
    }
    euclidean(num_beats[channel], beats[channel], MAX_BEATS);
    print_beats(channel);
}

void euclidean(int num_points, bool* chanbeats, int size) {
    // First clear the array
    for (int i = 0; i < size; i++) {
        chanbeats[i] = false;
    }
    // If num_points is 0 or greater than size, nothing to do
    if (num_points <= 0 || num_points > size) {
        return;
    }
    // Special case for 1 point
    if (num_points == 1) {
        chanbeats[0] = true;
        return;
    }
    // Calculate the spacing between num_points
    float spacing = (float)size / num_points;
    float position = 0;
    
    // Place each point
    for (int i = 0; i < num_points; i++) {
        // Round to nearest integer position
        int index = (int)(position + 0.5);
        if (index >= size) index = 0;  // Wrap around if needed
        
        chanbeats[index] = true;
        position += spacing;
    }
}


void rotate_beats(bool up){
    // rotate clockwise
    if(up){
        bool tmp = beats[channel][MAX_BEATS-1];
        for(int i=MAX_BEATS-1; i>=1; i--){
            beats[channel][i] = beats[channel][i-1];
        }
        beats[channel][0] = tmp;
    }

    // rotate counter-clockwise
    else{
        bool tmp = beats[channel][0];
        for(int i=0; i<=MAX_BEATS-2; i++){
            beats[channel][i] = beats[channel][i+1];
        }
        beats[channel][MAX_BEATS-1] = tmp;
    }
    print_beats(channel);
}


void increase_tempo(int mod){
    custom_tempo += mod;
    if (custom_tempo > MAX_TEMPO) custom_tempo = MAX_TEMPO;
    if (custom_tempo < MIN_TEMPO) custom_tempo = MIN_TEMPO;
    time_per_beat_ms = BPM_TO_MS(custom_tempo);
    time_per_sync_pulse = time_per_beat_ms / 6;
    sev_seg_show_digit(custom_tempo);
}


void increase_swing(int mod){
    swing_offset += mod;
    if (swing_offset < -4) swing_offset = -4;
    if (swing_offset > 4) swing_offset = 4;
    sev_seg_show_digit(swing_offset);
}


void send_midi_notes(uint8_t which_beat){
    // sev_seg_show_digit(which_beat);
    if (BLEMidiServer.isConnected()){
        for (int chan=0; chan<max_channels; chan++){
            // first send MIDI OFF for previous beat if it was on
            int prev_beat = (which_beat > 0 ? which_beat : (MAX_BEATS-1));
            if(beats[chan][prev_beat]){
                BLEMidiServer.noteOff(0, (base_note+chan), 127);
            }
            // then send MIDI ON for this note
            if(beats[chan][which_beat]){
                // midi channel 0, note 24 (C2), velocity 127.
                BLEMidiServer.noteOn(0, (base_note+chan), 127);
            }
        }
    }
}


void terminate_all_midi(){
    if (BLEMidiServer.isConnected()){
        for (int chan=0; chan<max_channels; chan++){
            BLEMidiServer.noteOff(0, (24+chan), 127);
        }
    }
}


void start_stop_playback(bool start){
    if (start){
        start_playback_fl = true;
        Serial.printf("starting playback\n");
    } 
    else {
        stop_playback_fl = true;
        Serial.printf("stopping playback\n");
    }
    
}


void keep_time(){
    unsigned long now;
    static unsigned long last_downbeat_time;

    switch (pbs){
        case PLAYBACK_IDLE:
            if (start_playback_fl){
                pbs = PLAYBACK_START;
                start_playback_fl = false;
            }
            break;
        case PLAYBACK_START:
            // play downbeat and start internal clock
            last_downbeat_time = millis();
            measure_counter = 0;
            sync_pulses = 0;
            next_pulse();
            pbs = PLAYBACK_PLAYING;
            break;
        case PLAYBACK_PLAYING:
            now = millis();
            if (now - last_downbeat_time >= time_per_sync_pulse){
                next_pulse();
                last_downbeat_time = now;
            }
            if (stop_playback_fl){
                pbs = PLAYBACK_STOP;
                stop_playback_fl = false;
            }
            break;
        case PLAYBACK_STOP:
            terminate_all_midi();
            measure_counter = BEAT_NONE;
            sync_pulses = 0;
            leds_show_playback(beats, channel, measure_counter);
            pbs = PLAYBACK_IDLE;
            break;
        case PLAYBACK_OVERRIDE:
            // do nothing - clock is controlled by MIDI callbacks.
            break;
    }
}

void next_pulse(){

    // Serial.printf("beat (%d)\n", sync_pulses);
    // downbeat and 8th notes
    if (sync_pulses == 0 || sync_pulses == 12 || sync_pulses == 24){
        leds_show_playback(beats, channel, measure_counter);
        send_midi_notes(measure_counter);
        // Serial.printf("measure_counter (%d)\n", measure_counter);
        measure_counter++;
    }

    // off beats
    else if (sync_pulses == 6+swing_offset || sync_pulses == 18+swing_offset ){
        // Serial.printf("beat (%d)\n", sync_pulses);
        leds_show_playback(beats, channel, measure_counter);
        send_midi_notes(measure_counter);
        // Serial.printf("measure_counter (%d)\n", measure_counter);
        measure_counter++;
    }

    sync_pulses++;
    if(sync_pulses >= 24){
        sync_pulses -= 24;
        if(measure_counter >= MAX_BEATS-1){
            measure_counter = 0;
        }
    }
}


playback_states get_playback_state(){
    return pbs;
}


/*************************************************************************** */
/* MIDI CALLBACKS                                                            */
/*************************************************************************** */

void onNoteOn(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp)
{
  Serial.printf("Note on : channel %d, note %d, velocity %d (timestamp %dms)\n", channel, note, velocity, timestamp);
}

void onNoteOff(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp)
{
  Serial.printf("Note off : channel %d, note %d, velocity %d (timestamp %dms)\n", channel, note, velocity, timestamp);
}

void onAfterTouchPoly(uint8_t channel, uint8_t note, uint8_t pressure, uint16_t timestamp)
{
  Serial.printf("Polyphonic after touch : channel %d, note %d, pressure %d (timestamp %dms)\n", channel, note, pressure, timestamp);
}

void onControlChange(uint8_t channel, uint8_t controller, uint8_t value, uint16_t timestamp)
{
    Serial.printf("Control change : channel %d, controller %d, value %d (timestamp %dms)\n", channel, controller, value, timestamp);
}

void onProgramChange(uint8_t channel, uint8_t program, uint16_t timestamp)
{
    Serial.printf("Program change : channel %d, program %d (timestamp %dms)\n", channel, program, timestamp);
}

void onAfterTouch(uint8_t channel, uint8_t pressure, uint16_t timestamp)
{
    Serial.printf("After touch : channel %d, pressure %d (timestamp %dms)\n", channel, pressure, timestamp);
}

void onPitchbend(uint8_t channel, uint16_t value, uint16_t timestamp)
{
    Serial.printf("Pitch bend : channel %d, value %d (timestamp %dms)\n", channel, value, timestamp);
}
void onClock(uint16_t timestamp)
{
    next_pulse();
}
void onClockStart(uint16_t timestamp)
{
    Serial.printf("Clock start (timestamp %dms)\n", timestamp);
    measure_counter = BEAT_NONE;
    sync_pulses = 0;
    next_pulse();
}

void onClockStop(uint16_t timestamp)
{
    Serial.printf("Clock stop (timestamp %dms)\n", timestamp);
    measure_counter = BEAT_NONE;
    sync_pulses = 0;
}

/*************************************************************************** */
/* BUTTON CALLBACKS                                                          */
/*************************************************************************** */

void onStartButtonRelease() {

    if (get_playback_state() == PLAYBACK_IDLE){
        start_stop_playback(START);
        Serial.printf("Start (orange)\n");
    }
    else if (get_playback_state() == PLAYBACK_PLAYING){
        start_stop_playback(STOP);
        Serial.printf("Stop (orange)\n");
    }
}

void onModeButtonRelease() {
    Serial.printf("Mode (blue)\n");
    cycle_device_mode();
    switch(device_mode){
        case EUCLIDEAN:
            sev_seg_display_word(SEG_EUCL);
            break;
        case MANUAL_VELOCITY:
            sev_seg_display_word(SEG_VOL);
            break;
        case TEMPO:
            sev_seg_display_word(SEG_RATE);
            break;
        case SWING:
            sev_seg_display_word(SEG_SHUF);
            break;
        default:
            Serial.printf("Error: invalid mode\n");
    }
}

void onChannelButtonRelease() {
    Serial.printf("Channel (brown)\n");
    if(channel >= (max_channels-1)){
      channel = 0;
    } else {
      channel++;
    }
    sev_seg_show_digit(channel);
    print_beats(channel);
}

void onCenterButtonPress(){
    Serial.printf("Center (press)\n");
    center_button_pos = CLOSED;
}

void onCenterButtonRelease() {
    Serial.printf("Center (release)\n");
    center_button_pos = OPEN;
}

/*************************************************************************** */
/* ENCODER MOVEMENT CALLBACKS                                                */
/*************************************************************************** */

void onEncoderUp(){
    switch(device_mode){
        case EUCLIDEAN:
            if (center_button_pos == CLOSED)
                rotate_beats(INCREMENT);
            else 
                inc_dec_beats(INCREMENT);
            break;
        case MANUAL_VELOCITY:
            break;
        case TEMPO:
            increase_tempo(1);
            break;
        case SWING:
            increase_swing(1);
            break;
        default:
            Serial.printf("Error: invalid mode\n");
    }
}

void onEncoderDown(){
    switch(device_mode){
        case EUCLIDEAN:
            if (center_button_pos == CLOSED)
                rotate_beats(DECREMENT);
            else 
                inc_dec_beats(DECREMENT);
            break;
        case MANUAL_VELOCITY:
            break;
        case TEMPO:
            increase_tempo(-1);
            break;
        case SWING:
            increase_swing(-1);
            break;
        default:
            Serial.printf("Error: invalid mode\n");
    }
}


/*************************************************************************** */
/* BUTTONS (TODO: this should just go in input_listener, except the callbacks) */
/*************************************************************************** */

Button buttons[NUM_BUTTONS] = {
    {CHANNEL_BUTTON,    OPEN, 0, NULL,                  onChannelButtonRelease},
    {START_STOP_BUTTON, OPEN, 0, NULL,                  onStartButtonRelease},
    {MODE_BUTTON,       OPEN, 0, NULL,                  onModeButtonRelease},
    {CENTER_BUTTON,     OPEN, 0, onCenterButtonPress,   onCenterButtonRelease}
};


/*************************************************************************** */
/* BEGIN CODE                                                                */
/*************************************************************************** */

void setup() {
  // Serial
  Serial.begin(115200);

  // initialize MIDI listener
  BLEMidiServer.begin("Euclidean Sequencer");
  BLEMidiServer.setOnConnectCallback([]() {
    Serial.println("Connected");
  });
  BLEMidiServer.setOnDisconnectCallback([]() {
    Serial.println("Disconnected");
  });
  BLEMidiServer.setNoteOnCallback(onNoteOn);
  BLEMidiServer.setNoteOffCallback(onNoteOff);
  BLEMidiServer.setAfterTouchPolyCallback(onAfterTouchPoly);
  BLEMidiServer.setControlChangeCallback(onControlChange);
  BLEMidiServer.setProgramChangeCallback(onProgramChange);
  BLEMidiServer.setAfterTouchCallback(onAfterTouch);
  BLEMidiServer.setPitchBendCallback(onPitchbend);
  BLEMidiServer.setMidiClockCallback(onClock);
  BLEMidiServer.setMidiStartCallback(onClockStart);
  BLEMidiServer.setMidiStopCallback(onClockStop);
  
//   BLEMidiServer.enableDebugging();

  // initialize buttons
  for (Button& button : buttons) {
      pinMode(button.pin, INPUT_PULLUP);
  }
  pinMode(CHANNEL_BUTTON, INPUT);
  pinMode(START_STOP_BUTTON, INPUT);
  pinMode(MODE_BUTTON, INPUT);
  pinMode(CENTER_BUTTON, INPUT);
  // initialize encoder
  rotary_encoder_init();
  registerEncTurnCallback(onEncoderUp, DIRECTION_UP);
  registerEncTurnCallback(onEncoderDown, DIRECTION_DOWN);

  // initialize seven segment display
  delay(100);
  sev_seg_power(false);
  delay(100);
  sev_seg_power(true);

  init_leds();
  
}



void loop() {
  updateButtons(buttons, NUM_BUTTONS);
  rotary_loop();
  keep_time();
}





// TODO:
// 1. clean up a little. MIDI and clock stuff can maybe go in its own file. standardize naming conventions.
// 2. sync with clock in
// 3. beats should be int velocity instead of on/off bools
// 4. manual velocity mode
// 5. test a USB MIDI library - can't find one. BLE it is.
// 6. shuffle mode
// 7. Store multiple measures?
// 8. wire and test leds - different color per channel, beat tracer
// 9. at startup clear LEDs and do a little dance 
// 10. multi channel or multi note toggle