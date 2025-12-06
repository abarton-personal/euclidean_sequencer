#include <Arduino.h>
#include <BLEMidi.h>
#include "seven_seg.h"
#include "input_listener.h"
#include "globals.h"
#include "led_wheel.h"
#include <vector>
#include <array>
using namespace std;

/*************************************************************************** */
/* Global Variables                                                         */
/*************************************************************************** */
// the currently active channel
static uint8_t channel = 0;
// max number of channels to cycle through. This can be adjusted up to MAX_MAX_CHANNELS
static uint8_t max_channels = 4;
// if channel button is held down to increase max channel number, 
// we don't want it to also rotate channels back to 0 after that
static bool dont_rotate_channels = false;
// currently active mode
volatile static modes device_mode = EUCLIDEAN;
// certain actions depend on whether the center button is held down or not
volatile static ButtState center_button_pos = OPEN;
volatile static ButtState channel_button_pos = OPEN;
// flags for handling sync signals outside of interrupt callbacks
volatile static bool receiving_sync = false;
volatile static bool received_sync_pulse = false;
volatile static bool received_clock_stop = false;
volatile static bool received_clock_start = false;

// tempo for manual playback. This is overridden by MIDI sync clicks from DAW
static uint8_t custom_tempo = 120;
// how much to swing. 0 is straight 16ths / no swing
static int16_t swing_offset = 0;
// period between beats, calculated from BPM
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
vector<array<bool,MAX_BEATS>> beats;

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

// fill beats vector with 4 bool arrays of size 16, all false
void init_beats_array(){
    for (int i = 0; i < max_channels; i++){
        beats.push_back({});
    }
}

// debug print the array beats[chan]
void print_beats(uint8_t chan){
    leds_show_beats(beats[channel], chan);
    printf("beats for channel %d: [", channel);
    for(int i=0; i<(MAX_BEATS-1); i++){
        printf("%d, ", beats[channel][i]);
    }
    printf("%d]\n", beats[channel][MAX_BEATS-1]);
}

// increment or decrement the number of beats for the current channel
void inc_dec_beats(bool up){
    if(up){
        if (num_beats[channel] < MAX_BEATS){
            num_beats[channel]++;
        }
    }
    else {
        if (num_beats[channel] > 0){
            num_beats[channel]--;
        }
    }
    euclidean(num_beats[channel], beats[channel]);
    print_beats(channel);
}

void inc_dec_max_channel(bool up){
    if (up){
        if (channel < MAX_MAX_CHANNEL){
            beats.push_back({});     
            max_channels++;
        }
    } else {
        if (channel > 1){
            beats.pop_back();
            max_channels--;
        }
    }
    // switch immediately to the new highest channel
    channel = max_channels - 1;
    sev_seg_show_digit(channel);
    print_beats(channel);
    dont_rotate_channels = true;
}

// calculates how to space out the beats
void euclidean(int num_points, array<bool,MAX_BEATS>& chanbeats) {
    // First clear the array
    for (auto& beat : chanbeats) {
        beat = false;
    }
    // If num_points is 0 or greater than size, nothing to do
    if (num_points <= 0 || num_points > MAX_BEATS) {
        return;
    }
    // Special case for 1 point
    if (num_points == 1) {
        chanbeats[0] = true;
        return;
    }
    // Calculate the spacing between num_points
    float spacing = (float)MAX_BEATS / num_points;
    float position = 0;

    // Place each point
    for (int i = 0; i < num_points; i++) {
        // Round to nearest integer position
        int index = (int)(position + 0.5);
        if (index >= MAX_BEATS) index = 0;  // Wrap around if needed

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


void start_playback(){
    start_playback_fl = true;
    Serial.printf("starting playback\n");
}

void stop_playback(){
    stop_playback_fl = true;
    Serial.printf("stopping playback\n");
}


void keep_time(){
    // manage the playback state when controlled from the device
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
            leds_show_beats(beats[channel], channel);
            leds_show_measure_counter(measure_counter);
            pbs = PLAYBACK_IDLE;
            break;

        case PLAYBACK_OVERRIDE:
            // do nothing - clock is controlled by MIDI callbacks.
            break;
    }
}

// manages tasks related to incoming sync messages
void handle_sync_flags(){
    // clock started
    if(received_clock_start){
        receiving_sync = true;
        // if it's already playing using internal clock, stop that and use the sync messages instead
        if (get_playback_state() != PLAYBACK_IDLE){
            stop_playback();
            // let the state machine finish
            while(get_playback_state() != PLAYBACK_IDLE)
                keep_time();
        }
        received_clock_start = false;
        // Play the first beat now. Subsequent beats will be triggered by the sync pulse callbacks
        received_sync_pulse = true;
    }

    if(received_clock_stop){
        // hide measure counter light
        leds_show_measure_counter(BEAT_NONE);
        leds_show_beats(beats[channel], channel);
        receiving_sync = false;
        received_clock_stop = false;
    }

    if(received_sync_pulse){
        next_pulse();
        received_sync_pulse = false;
    }

}

void setMeasureCounter(uint16_t position){
    if (get_playback_state() == PLAYBACK_IDLE && !receiving_sync){
        measure_counter = position;
        sync_pulses = (position * 6) % PPQN;
        leds_show_beats(beats[channel], channel);
        leds_show_measure_counter(measure_counter);
    }
}

void next_pulse(){

    // Serial.printf("beat (%d)\n", sync_pulses);
    // downbeat and 8th notes
    if (sync_pulses == 0 || sync_pulses == (PPQN/2) || sync_pulses == PPQN){
        leds_show_beats(beats[channel], channel);
        leds_show_measure_counter(measure_counter);
        send_midi_notes(measure_counter);
        // Serial.printf("measure_counter (%d)\n", measure_counter);
        measure_counter++;
    }

    // off beats
    else if (sync_pulses == (PPQN/4)+swing_offset || sync_pulses == (PPQN*3/4)+swing_offset ){
        // Serial.printf("beat (%d)\n", sync_pulses);
        leds_show_beats(beats[channel], channel);
        leds_show_measure_counter(measure_counter);
        send_midi_notes(measure_counter);
        // Serial.printf("measure_counter (%d)\n", measure_counter);
        measure_counter++;
    }

    sync_pulses++;
    if(sync_pulses >= PPQN){
        sync_pulses -= PPQN;
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
    if (controller == 0x7B){
        Serial.printf("PANIC\n");
    }
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
void onPosition(uint16_t position, uint16_t timestamp)
{
    Serial.printf("Position pointer : position %d (timestamp %dms)\n", position, timestamp);
    setMeasureCounter(position % MAX_BEATS);
}
void onClock(uint16_t timestamp)
{
    received_sync_pulse = true;
}
void onClockStart(uint16_t timestamp)
{
    Serial.printf("Clock start: (timestamp %dms)\n",timestamp);
    received_clock_start = true;
}
void onClockContinue(uint16_t timestamp)
{
    Serial.printf("Clock continue: (timestamp %dms)\n",timestamp);
    received_clock_start = true;
}
void onClockStop(uint16_t timestamp)
{
    Serial.printf("Clock stop: (timestamp %dms)\n",timestamp);
    received_clock_stop = true;
}

/*************************************************************************** */
/* BUTTON/ENCODER CALLBACKS                                                  */
/*************************************************************************** */

void onStartButtonRelease() {
    // if receiving click track, start/stop button is disabled
    if (receiving_sync){
        sev_seg_display_word(SEG_SYNC);
        Serial.printf("Can't start - syncing\n");
        return;
    }

    if (get_playback_state() == PLAYBACK_IDLE){
        start_playback();
    }
    else if (get_playback_state() == PLAYBACK_PLAYING){
        stop_playback();
    }
}

void onModeButtonRelease() {
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

void onChannelButtonPress() {
    channel_button_pos = CLOSED;
}

void onChannelButtonRelease() {
    channel_button_pos = OPEN;
    if (dont_rotate_channels){
        dont_rotate_channels = false;
        return;
    }
    // rotate to the next channel
    if(channel >= (max_channels-1)){
      channel = 0;
    } else {
      channel++;
    }
    sev_seg_show_digit(channel);
    print_beats(channel);
}

void onCenterButtonPress(){
    center_button_pos = CLOSED;
}

void onCenterButtonRelease() {
    center_button_pos = OPEN;
}

void onEncoderUpDown(bool direction){
    switch(device_mode){
        case EUCLIDEAN:
            if (center_button_pos == CLOSED)
                rotate_beats(direction);
            else if (channel_button_pos == CLOSED)
                inc_dec_max_channel(direction);
            else
                inc_dec_beats(direction);
            break;
        case MANUAL_VELOCITY:
            break;
        case TEMPO:
            increase_tempo(direction ? 1 : -1);
            break;
        case SWING:
            increase_swing(direction ? 1 : -1);
            break;
        default:
            Serial.printf("Error: invalid mode\n");
    }
}


/*************************************************************************** */
/* BEGIN CODE                                                                */
/*************************************************************************** */

void setup() {

    init_beats_array();
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
    BLEMidiServer.setMidiPositionCallback(onPosition);
    BLEMidiServer.setMidiClockCallback(onClock);
    BLEMidiServer.setMidiStartCallback(onClockStart);
    BLEMidiServer.setMidiContinueCallback(onClockContinue);
    BLEMidiServer.setMidiStopCallback(onClockStop);
    //   BLEMidiServer.enableDebugging();

    // initialize buttons
    buttons_init();
    registerButtonCallbacks(CHANNEL_BUTTON,     onChannelButtonPress, onChannelButtonRelease);
    registerButtonCallbacks(START_STOP_BUTTON,  NULL,                 onStartButtonRelease);
    registerButtonCallbacks(MODE_BUTTON,        NULL,                 onModeButtonRelease);
    registerButtonCallbacks(CENTER_BUTTON,      onCenterButtonPress,  onCenterButtonRelease);
    // initialize encoder
    rotary_encoder_init();
    registerEncTurnCallback(onEncoderUpDown);

    // initialize seven segment display
    delay(100);
    sev_seg_power(false);
    delay(100);
    sev_seg_power(true);

    // initialize LED wheel
    init_leds();

}


void loop() {
    updateButtons();
    rotary_encoder_loop();
    keep_time();
    handle_sync_flags();
    led_tasks();
}





// TODO:
// 1. clean up a little. MIDI and clock stuff can maybe go in its own file. standardize naming conventions.
// 3. beats could be int velocity instead of on/off bools
// 4. manual velocity mode ?
// 7. Store multiple measures?
// 10. multi channel or multi note toggle