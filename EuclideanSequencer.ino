#include <Arduino.h>
#include <BLEMidi.h>
#include "seven_seg.h"
#include "input_listener.h"
#include "globals.h"


/*************************************************************************** */
/* Global Variables                                                         */
/*************************************************************************** */
static uint8_t channel = 0;
static uint8_t max_channels = 4;
volatile static modes device_mode = EUCLIDEAN;
volatile static ButtState center_button_pos = OPEN;

static uint8_t custom_tempo = 120;
static uint32_t time_per_beat_ms = BPM_TO_MS(custom_tempo);
static uint8_t measure_counter = 0;
static playback_states pbs = PLAYBACK_IDLE;
static bool start_playback_fl = false;
static bool stop_playback_fl = false;

static uint8_t num_beats[MAX_MAX_CHANNEL] = {0};
static bool beats[MAX_MAX_CHANNEL][16] = {{false}};


/*************************************************************************** */
/* Private Functions                                                         */
/*************************************************************************** */

void euclidean(int num_points, bool* chanbeats, int size);
void rotate_beats(bool up);
void increase_tempo(int mod);
void send_midi_notes(uint8_t which_beat);
playback_states get_playback_state();

// cycle to the next mode when mode button pressed
void cycle_device_mode(){
  device_mode = static_cast<modes>((device_mode + 1) % NUM_MODES);
}

// debug print the array beats[chan]
void print_beats(uint8_t chan){
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
    sev_seg_show_digit(custom_tempo);
}


void send_midi_notes(uint8_t which_beat){
    // sev_seg_show_digit(which_beat);
    if (BLEMidiServer.isConnected()){
        for (int chan=0; chan<max_channels; chan++){
            // first send MIDI OFF for previous beat if it was on
            int prev_beat = (which_beat > 0 ? which_beat : (MAX_BEATS-1));
            if(beats[chan][prev_beat]){
                BLEMidiServer.noteOff(0, (36+chan), 127);
            }
            // then send MIDI ON for this note
            if(beats[chan][which_beat]){
                // midi channel 0, note 24 (C2), velocity 127.
                // TODO: make these adjustable somehow
                BLEMidiServer.noteOn(0, (36+chan), 127);
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
            send_midi_notes(measure_counter++);
            pbs = PLAYBACK_PLAYING;
            break;
        case PLAYBACK_PLAYING:
            if (millis() - last_downbeat_time >= time_per_beat_ms){
                // if it's time, play the next set of notes
                last_downbeat_time += time_per_beat_ms;
                send_midi_notes(measure_counter++);
                // loop back to beginning of measure
                if(measure_counter >= MAX_BEATS){
                    measure_counter = 0;
                }
            }
            if (stop_playback_fl){
                pbs = PLAYBACK_STOP;
                stop_playback_fl = false;
            }
            break;
        case PLAYBACK_STOP:
            terminate_all_midi();
            pbs = PLAYBACK_IDLE;
            break;
        case PLAYBACK_OVERRIDE:
            // do nothing - clock is controlled by MIDI callbacks.
            break;
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
    static uint16_t beat = 0;
    beat++;
    if(beat > 96){
        beat -= 96;
        Serial.printf("Clock beat %d\n", beat);
    }
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
  // BLEMidiServer.enableDebugging();

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
  sev_seg_power(true);
  
}



void loop() {
//   if(BLEMidiServer.isConnected()) {             // If we've got a connection, we send an A4 during one second, at full velocity (127)
//       BLEMidiServer.noteOn(0, 69, 127);
//       delay(1000);
//       BLEMidiServer.noteOff(0, 69, 127);        // Then we stop the note and make a delay of one second before returning to the beginning of the loop
//       delay(1000);
//   }
  updateButtons(buttons, NUM_BUTTONS);
  rotary_loop();
  keep_time();
}





