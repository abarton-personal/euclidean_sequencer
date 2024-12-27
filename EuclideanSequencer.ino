#include <Arduino.h>
#include <BLEMidi.h>
#include "seven_seg.h"
#include "input_listener.h"
#include "globals.h"

#define LED_BUILTIN 2

static int myval = 0;


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

void onStartButtonRelease() {
    Serial.printf("Start (orange)\n");
    myval++;
    sev_seg_show_digit(myval);
}

void onModeButtonRelease() {
    Serial.printf("Mode (blue)\n");
    if(myval > 0) myval--;
    sev_seg_show_digit(myval);
}

void onChannelButtonRelease() {
    Serial.printf("Channel (brown)\n");
    static bool pow = true;
    if(pow) pow = false;
    else pow = true;
    sev_seg_power(pow);
    if (pow) sev_seg_show_digit(myval);
}

void onCenterButtonRelease() {
    Serial.printf("Center (enc)\n");
    sev_seg_display_done();
}

Button buttons[NUM_BUTTONS] = {
    {CHANNEL_BUTTON, OPEN, 0, onChannelButtonRelease},
    {START_STOP_BUTTON, OPEN, 0, onStartButtonRelease},
    {MODE_BUTTON, OPEN, 0, onModeButtonRelease},
    {CENTER_BUTTON, OPEN, 0, onCenterButtonRelease}
};



void setup() {
  Serial.begin(115200);
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
      // set up inputs
    for (Button& button : buttons) {
        pinMode(button.pin, INPUT_PULLUP);
    }
    pinMode(CHANNEL_BUTTON, INPUT);
    pinMode(START_STOP_BUTTON, INPUT);
    pinMode(MODE_BUTTON, INPUT);
    pinMode(CENTER_BUTTON, INPUT);

  sev_seg_power(true);
  rotary_encoder_init();
  
}

void loop() {
  if(BLEMidiServer.isConnected()) {             // If we've got a connection, we send an A4 during one second, at full velocity (127)
      BLEMidiServer.noteOn(0, 69, 127);
      delay(1000);
      BLEMidiServer.noteOff(0, 69, 127);        // Then we stop the note and make a delay of one second before returning to the beginning of the loop
      delay(1000);
  }
  updateButtons(buttons, NUM_BUTTONS);
  rotary_loop();
}





